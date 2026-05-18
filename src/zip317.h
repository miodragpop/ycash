// Copyright (c) 2023-2023 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#ifndef ZCASH_ZIP317_H
#define ZCASH_ZIP317_H

#include "amount.h"
#include "primitives/transaction.h"

#include <cstdint>
#include <cstddef>
#include <vector>

// Constants for fee calculation.
static const CAmount MARGINAL_FEE = 500;
static const size_t GRACE_ACTIONS = 2;
static const size_t P2PKH_STANDARD_INPUT_SIZE = 150;
static const size_t P2PKH_STANDARD_OUTPUT_SIZE = 34;

// Constants for block template construction.
static const int64_t WEIGHT_RATIO_SCALE = INT64_C(10000000000000000);
static const int64_t WEIGHT_RATIO_CAP = 4;

// NOTE: ZIP 317 mempool / block-template policy is intentionally OFF by default
// in Ycash. The ZIP 317 code paths (conventional fee, unpaid actions, weighted
// block construction, ZIP 401 low-fee eviction penalty) remain compiled and
// fully wired -- only the per-tx and per-block unpaid-action *limits* are
// raised to SIZE_MAX so the rejection and free-action capping become no-ops.
//
// Day-to-day shielded-output spam is handled by the much simpler Ycash
// per-Sapling-output fee floor (see `PerSaplingOutputFees` in policy/policy.cpp),
// which is ported from ycash-official and runs in AcceptToMemoryPool.
//
// To re-activate ZIP 317 in the future:
//   - At runtime (per-node opt-in, no release required):
//       ycashd -txunpaidactionlimit=0 -blockunpaidactionlimit=0
//   - As a network-wide default: change the two constants below back to 0 and
//     ship a release. ZIP 317 is *relay policy*, not consensus, so a soft
//     default change is sufficient; a UPGRADE_* activation height is not
//     required (and would be overkill).
static const size_t DEFAULT_BLOCK_UNPAID_ACTION_LIMIT = SIZE_MAX;

// Wallet fee-funding policy (node-local; selects how a no-explicit-fee
// transaction is funded). Not network policy: relay/accept stays the Ycash
// per-Sapling-output floor regardless. `PerOutput` is the Ycash v4.5.0
// contract; `ZIP317` is plain ZIP 317 at the MARGINAL_FEE above. The two are
// kept distinct -- neither mode mixes the other's floor in.
enum class FeePolicy { PerOutput, ZIP317 };
static const FeePolicy DEFAULT_FEE_POLICY = FeePolicy::PerOutput;
extern FeePolicy nFeePolicy;

/// Limit on the number of unpaid actions a transaction can have to be accepted to the mempool.
static const size_t DEFAULT_TX_UNPAID_ACTION_LIMIT = DEFAULT_BLOCK_UNPAID_ACTION_LIMIT;

/// This is the lowest the conventional fee can be in ZIP 317.
static const CAmount MINIMUM_FEE = MARGINAL_FEE * GRACE_ACTIONS;

/// Return the conventional fee for the given `logicalActionCount` calculated according to
/// <https://zips.z.cash/zip-0317#fee-calculation>.
CAmount CalculateConventionalFee(size_t logicalActionCount);

/// Return the number of logical actions calculated according to
/// <https://zips.z.cash/zip-0317#fee-calculation>.
size_t CalculateLogicalActionCount(
        const std::vector<CTxIn>& vin,
        const std::vector<CTxOut>& vout,
        unsigned int joinSplitCount,
        unsigned int saplingSpendCount,
        unsigned int saplingOutputCount,
        unsigned int orchardActionCount);

#endif // ZCASH_ZIP317_H
