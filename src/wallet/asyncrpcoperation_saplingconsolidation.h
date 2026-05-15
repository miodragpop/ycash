#ifndef ZCASH_WALLET_ASYNCRPCOPERATION_SAPLINGCONSOLIDATION_H
#define ZCASH_WALLET_ASYNCRPCOPERATION_SAPLINGCONSOLIDATION_H

#include "amount.h"
#include "asyncrpcoperation.h"
#include "univalue.h"
#include "zcash/Address.hpp"

// Sentinel: when fConsolidationTxFee holds this value, the operation derives
// the fee from the built transaction's per-Sapling-output floor instead of
// using a fixed amount. -consolidationtxfee overrides with an explicit value.
static const CAmount CONSOLIDATION_FEE_DERIVE = -1;

extern CAmount fConsolidationTxFee;
extern bool fConsolidationMapUsed;

class AsyncRPCOperation_saplingconsolidation : public AsyncRPCOperation
{
public:
    AsyncRPCOperation_saplingconsolidation(int targetHeight, uint256 saplingAnchor);
    virtual ~AsyncRPCOperation_saplingconsolidation();

    // We don't want to be copied or moved around
    AsyncRPCOperation_saplingconsolidation(AsyncRPCOperation_saplingconsolidation const&) = delete;            // Copy construct
    AsyncRPCOperation_saplingconsolidation(AsyncRPCOperation_saplingconsolidation&&) = delete;                 // Move construct
    AsyncRPCOperation_saplingconsolidation& operator=(AsyncRPCOperation_saplingconsolidation const&) = delete; // Copy assign
    AsyncRPCOperation_saplingconsolidation& operator=(AsyncRPCOperation_saplingconsolidation&&) = delete;      // Move assign

    virtual void main();

    virtual void cancel();

    virtual UniValue getStatus() const;

private:
    int targetHeight_;
    uint256 saplingAnchor_;

    bool main_impl();

    void setConsolidationResult(int numTxCreated, const CAmount& amountConsolidated, const std::vector<std::string>& consolidationTxIds);
};

#endif // ZCASH_WALLET_ASYNCRPCOPERATION_SAPLINGCONSOLIDATION_H
