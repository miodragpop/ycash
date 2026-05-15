#include "assert.h"
#include "asyncrpcoperation_saplingconsolidation.h"
#include "init.h"
#include "key_io.h"
#include "policy/policy.h"
#include "rpc/protocol.h"
#include "random.h"
#include "sync.h"
#include "tinyformat.h"
#include "transaction_builder.h"
#include "util/system.h"
#include "util/moneystr.h"
#include "wallet.h"

#include <optional>
#include <variant>

CAmount fConsolidationTxFee = CONSOLIDATION_FEE_DERIVE;
bool fConsolidationMapUsed = false;
const int CONSOLIDATION_EXPIRY_DELTA = 15;

AsyncRPCOperation_saplingconsolidation::AsyncRPCOperation_saplingconsolidation(int targetHeight, uint256 saplingAnchor) :
    targetHeight_(targetHeight), saplingAnchor_(saplingAnchor) {}

AsyncRPCOperation_saplingconsolidation::~AsyncRPCOperation_saplingconsolidation() {}

void AsyncRPCOperation_saplingconsolidation::main() {
    if (isCancelled())
        return;

    set_state(OperationStatus::EXECUTING);
    start_execution_clock();

    bool success = false;

    try {
        success = main_impl();
    } catch (const UniValue& objError) {
        int code = find_value(objError, "code").get_int();
        std::string message = find_value(objError, "message").get_str();
        set_error_code(code);
        set_error_message(message);
    } catch (const runtime_error& e) {
        set_error_code(-1);
        set_error_message("runtime error: " + string(e.what()));
    } catch (const logic_error& e) {
        set_error_code(-1);
        set_error_message("logic error: " + string(e.what()));
    } catch (const exception& e) {
        set_error_code(-1);
        set_error_message("general exception: " + string(e.what()));
    } catch (...) {
        set_error_code(-2);
        set_error_message("unknown error");
    }

    stop_execution_clock();

    if (success) {
        set_state(OperationStatus::SUCCESS);
    } else {
        set_state(OperationStatus::FAILED);
    }

    std::string s = strprintf("%s: Sapling Consolidation transaction created. (status=%s", getId(), getStateAsString());
    if (success) {
        s += strprintf(", success)\n");
    } else {
        s += strprintf(", error=%s)\n", getErrorMessage());
    }

    LogPrintf("%s", s);
}

bool AsyncRPCOperation_saplingconsolidation::main_impl() {
    LogPrint("zrpcunsafe", "%s: Beginning AsyncRPCOperation_saplingconsolidation.\n", getId());
    const Consensus::Params& consensusParams = Params().GetConsensus();
    auto nextActivationHeight = NextActivationHeight(targetHeight_, consensusParams);
    if (nextActivationHeight && targetHeight_ + CONSOLIDATION_EXPIRY_DELTA >= nextActivationHeight.value()) {
        LogPrint("zrpcunsafe", "%s: Consolidation txs would be created before a NU activation but may expire after. Skipping this round.\n", getId());
        setConsolidationResult(0, 0, std::vector<std::string>());
        return true;
    }

    KeyIO keyIO(Params());
    std::vector<SproutNoteEntry> sproutEntries;
    std::vector<SaplingNoteEntry> saplingEntries;
    std::vector<OrchardNoteMetadata> orchardEntries;
    std::set<libzcash::SaplingPaymentAddress> addresses;
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);
        // minDepth 11: skip unconfirmed notes, mirroring the Sapling migration op.
        pwalletMain->GetFilteredNotes(sproutEntries, saplingEntries, orchardEntries, std::nullopt, std::nullopt, 11);
        if (fConsolidationMapUsed) {
            const std::vector<std::string>& v = mapMultiArgs["-consolidatesaplingaddress"];
            for (size_t i = 0; i < v.size(); i++) {
                auto zAddress = keyIO.DecodePaymentAddress(v[i]);
                if (zAddress.has_value()) {
                    if (auto saplingAddr = std::get_if<libzcash::SaplingPaymentAddress>(&zAddress.value())) {
                        addresses.insert(*saplingAddr);
                    }
                }
            }
        } else {
            pwalletMain->GetSaplingPaymentAddresses(addresses);
        }
    }

    int numTxCreated = 0;
    std::vector<std::string> consolidationTxIds;
    CAmount amountConsolidated = 0;
    CCoinsViewCache coinsView(pcoinsTip);

    for (auto addr : addresses) {
        libzcash::SaplingExtendedSpendingKey extsk;
        if (pwalletMain->GetSaplingExtendedSpendingKey(addr, extsk)) {

            std::vector<SaplingNoteEntry> fromNotes;
            CAmount amountToSend = 0;
            // Use a randomly determined number of notes between 10 and 44 so the
            // input count of consolidation txs is not a stable fingerprint.
            int maxQuantity = GetRand(35) + 10;
            for (const SaplingNoteEntry& saplingEntry : saplingEntries) {
                libzcash::SaplingIncomingViewingKey ivk;
                pwalletMain->GetSaplingIncomingViewingKey(saplingEntry.address, ivk);

                // Select only notes belonging to the address we are sweeping to.
                if (ivk == extsk.expsk.full_viewing_key().in_viewing_key()) {
                    amountToSend += CAmount(saplingEntry.note.value());
                    fromNotes.push_back(saplingEntry);
                }

                if (fromNotes.size() >= (size_t)maxQuantity)
                    break;
            }

            // Require a random minimum of 2 - 11 notes before it is worth
            // consolidating this address this round.
            int minQuantity = GetRand(10) + 2;
            if (fromNotes.size() < (size_t)minQuantity)
                continue;

            auto builder = TransactionBuilder(
                Params(),
                targetHeight_,
                std::nullopt,
                saplingAnchor_,
                pwalletMain,
                &coinsView,
                &cs_main);
            builder.SetExpiryHeight(targetHeight_ + CONSOLIDATION_EXPIRY_DELTA);

            std::vector<SaplingOutPoint> ops;
            std::vector<libzcash::SaplingNote> notes;
            for (auto fromNote : fromNotes) {
                ops.push_back(fromNote.op);
                notes.push_back(fromNote.note);
            }

            uint256 anchor;
            std::vector<std::optional<SaplingWitness>> witnesses;
            {
                LOCK2(cs_main, pwalletMain->cs_wallet);
                if (!pwalletMain->GetSaplingNoteWitnesses(ops, nAnchorConfirmations, witnesses, anchor)) {
                    LogPrint("zrpcunsafe", "%s: Insufficient Sapling witnesses for this address. Skipping.\n", getId());
                    continue;
                }
            }

            bool missingWitness = false;
            for (size_t i = 0; i < notes.size(); i++) {
                if (!witnesses[i]) {
                    LogPrint("zrpcunsafe", "%s: Missing witness for note %d. Skipping this address.\n", getId(), i);
                    missingWitness = true;
                    break;
                }
                builder.AddSaplingSpend(extsk, notes[i], witnesses[i].value());
            }
            if (missingWitness)
                continue;

            // Fee: explicit -consolidationtxfee if set, otherwise the
            // per-Sapling-output anti-spam floor. A consolidation tx has
            // exactly one Sapling output, which is within the grace range
            // (<= DEFAULT_EXEMPT_SAPLING_OUTPUTS), so the floor is a single
            // DEFAULT_PER_SAPLING_OUTPUT_FEE -- see PerSaplingOutputFees in
            // policy/policy.cpp. Anything less is rejected by our own mempool.
            CAmount fee = fConsolidationTxFee;
            if (fee == CONSOLIDATION_FEE_DERIVE) {
                fee = DEFAULT_PER_SAPLING_OUTPUT_FEE;
            }
            if (amountToSend <= fee) {
                LogPrint("zrpcunsafe", "%s: Selected amount %s does not cover fee %s. Skipping this address.\n",
                    getId(), FormatMoney(amountToSend), FormatMoney(fee));
                continue;
            }

            builder.SetFee(fee);
            builder.AddSaplingOutput(extsk.expsk.ovk, addr, amountToSend - fee, std::nullopt);
            CTransaction tx = builder.Build().GetTxOrThrow();

            if (isCancelled()) {
                LogPrint("zrpcunsafe", "%s: Canceled. Stopping.\n", getId());
                break;
            }

            pwalletMain->AddPendingSaplingConsolidationTx(tx);
            LogPrint("zrpcunsafe", "%s: Added pending consolidation transaction with txid=%s\n", getId(), tx.GetHash().ToString());
            ++numTxCreated;
            amountConsolidated += amountToSend - fee;
            consolidationTxIds.push_back(tx.GetHash().ToString());
        }
    }

    LogPrint("zrpcunsafe", "%s: Created %d transactions with total Sapling output amount=%s\n", getId(), numTxCreated, FormatMoney(amountConsolidated));
    setConsolidationResult(numTxCreated, amountConsolidated, consolidationTxIds);
    return true;
}

void AsyncRPCOperation_saplingconsolidation::setConsolidationResult(int numTxCreated, const CAmount& amountConsolidated, const std::vector<std::string>& consolidationTxIds) {
    UniValue res(UniValue::VOBJ);
    res.pushKV("num_tx_created", numTxCreated);
    res.pushKV("amount_consolidated", FormatMoney(amountConsolidated));
    UniValue txIds(UniValue::VARR);
    for (const std::string& txId : consolidationTxIds) {
        txIds.push_back(txId);
    }
    res.pushKV("consolidation_txids", txIds);
    set_result(res);
}

void AsyncRPCOperation_saplingconsolidation::cancel() {
    set_state(OperationStatus::CANCELLED);
}

UniValue AsyncRPCOperation_saplingconsolidation::getStatus() const {
    UniValue v = AsyncRPCOperation::getStatus();
    UniValue obj = v.get_obj();
    obj.pushKV("method", "saplingconsolidation");
    obj.pushKV("target_height", targetHeight_);
    return obj;
}
