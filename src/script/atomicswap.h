// Copyright (c) 2025-2026 The Ycash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#ifndef BITCOIN_SCRIPT_ATOMICSWAP_H
#define BITCOIN_SCRIPT_ATOMICSWAP_H

#include "script/script.h"
#include "script/standard.h"
#include "primitives/transaction.h"
#include "pubkey.h"
#include "uint256.h"

#include <optional>
#include <vector>

/**
 * Atomic Swap HTLC (Hash Time-Locked Contract) Implementation
 *
 * Cross-chain atomic swaps using hash time-locked contracts. Two parties
 * exchange funds across different blockchains without requiring trust,
 * using a shared secret and timelocks.
 *
 * HTLC Script Structure:
 *
 * IF
 *   // Claim path: recipient provides preimage
 *   HASH160 <secret_hash> EQUALVERIFY DUP HASH160 <recipient_pubkey_hash>
 * ELSE
 *   // Refund path: initiator reclaims after locktime
 *   <locktime> CHECKLOCKTIMEVERIFY DROP DUP HASH160 <initiator_pubkey_hash>
 * ENDIF
 * EQUALVERIFY CHECKSIG
 *
 * Two spending paths:
 *   1. Claim: recipient redeems by revealing the preimage (secret).
 *   2. Refund: initiator reclaims after locktime expires.
 *
 * Ported from ycash-official's 2025 atomic-swap work. The script-level
 * primitives are self-contained C++ and operate purely on CScript /
 * CKeyID / uint160 / uint256 -- no wallet or chain dependency.
 */

/** Atomic swap contract parameters. */
struct AtomicSwapContract {
    uint160 secretHash;           // HASH160 of the secret (RIPEMD160(SHA256(secret)))
    CKeyID recipientPubKeyHash;   // Public key hash of the recipient (claim path)
    CKeyID initiatorPubKeyHash;   // Public key hash of the initiator (refund path)
    int64_t lockTime;             // Absolute locktime (block height or Unix timestamp)

    AtomicSwapContract() : lockTime(0) {}

    AtomicSwapContract(
        const uint160& secretHash_,
        const CKeyID& recipientPubKeyHash_,
        const CKeyID& initiatorPubKeyHash_,
        int64_t lockTime_)
        : secretHash(secretHash_),
          recipientPubKeyHash(recipientPubKeyHash_),
          initiatorPubKeyHash(initiatorPubKeyHash_),
          lockTime(lockTime_) {}

    bool IsValid() const;
};

/** Build an HTLC redeem script for atomic swaps (for use in P2SH). */
CScript BuildAtomicSwapScript(const AtomicSwapContract& contract);

/** Extract atomic swap contract data from a redeem script. */
bool ExtractAtomicSwapContract(const CScript& redeemScript, AtomicSwapContract& contractOut);

/** Build a scriptSig to claim an atomic swap (reveal secret). */
CScript BuildAtomicSwapClaimScript(
    const std::vector<unsigned char>& secret,
    const std::vector<unsigned char>& signature,
    const std::vector<unsigned char>& pubkey,
    const CScript& redeemScript);

/** Build a scriptSig to refund an atomic swap (after locktime). */
CScript BuildAtomicSwapRefundScript(
    const std::vector<unsigned char>& signature,
    const std::vector<unsigned char>& pubkey,
    const CScript& redeemScript);

/** Verify that a transaction output contains a valid atomic swap contract. */
bool VerifyAtomicSwapOutput(const CTxOut& txOut, const CScript& redeemScript);

/** Extract the secret from a claim transaction (input index variant). */
bool ExtractAtomicSwapSecret(
    const CTransaction& tx,
    uint32_t vin,
    std::vector<unsigned char>& secretOut);

/** Extract the secret directly from a scriptSig. */
bool ExtractAtomicSwapSecret(
    const CScript& scriptSig,
    std::vector<unsigned char>& secretOut);

/** Generate a cryptographically secure random 32-byte secret. */
std::vector<unsigned char> GenerateAtomicSwapSecret();

/** Validate atomic swap parameters against the current height/time. */
bool ValidateAtomicSwapContract(
    const AtomicSwapContract& contract,
    int currentHeight,
    std::string& errorOut);

#endif // BITCOIN_SCRIPT_ATOMICSWAP_H
