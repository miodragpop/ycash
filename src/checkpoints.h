// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2017-2023 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#ifndef BITCOIN_CHECKPOINTS_H
#define BITCOIN_CHECKPOINTS_H

#include "uint256.h"

#include <map>

class CBlockIndex;
struct CCheckpointData;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{

//! Return conservative estimate of total number of blocks, 0 if unknown
int GetTotalBlocksEstimate(const CCheckpointData& data);

//! Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex* GetLastCheckpoint(const CCheckpointData& data);

double GuessVerificationProgress(const CCheckpointData& data, CBlockIndex* pindex, bool fSigchecks = true);

bool IsAncestorOfLastCheckpoint(const CCheckpointData& data, const CBlockIndex* pindex);

//! Height-based equivalent that does not depend on `mapBlockIndex` containing
//! the checkpoint blocks. Returns true iff `pindex->nHeight` is at or below the
//! highest hardcoded checkpoint height. Safe to use as a "skip expensive checks"
//! gate only in combination with checkpoint-hash enforcement at known heights
//! (see ContextualCheckBlockHeader), which guarantees that any block reaching
//! validation at a checkpoint height is on the checkpointed chain.
bool IsBelowOrAtLastCheckpointHeight(const CCheckpointData& data, const CBlockIndex* pindex);
} //namespace Checkpoints

#endif // BITCOIN_CHECKPOINTS_H
