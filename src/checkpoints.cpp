// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#include "checkpoints.h"

#include "chainparams.h"
#include "main.h"
#include "reverse_iterator.h"
#include "uint256.h"

#include <stdint.h>


namespace Checkpoints {

    /**
     * How many times slower we expect checking transactions after the last
     * checkpoint to be (from checking signatures, which is skipped up to the
     * last checkpoint). This number is a compromise, as it can't be accurate
     * for every system. When reindexing from a fast disk with a slow CPU, it
     * can be up to 20, while when downloading from a slow network with a
     * fast multicore CPU, it won't be much higher than 1.
     */
    static const double SIGCHECK_VERIFICATION_FACTOR = 5.0;

    //! Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(const CCheckpointData& data, CBlockIndex *pindex, bool fSigchecks) {
        if (pindex==NULL)
            return 0.0;

        int64_t nNow = time(NULL);

        double fSigcheckVerificationFactor = fSigchecks ? SIGCHECK_VERIFICATION_FACTOR : 1.0;
        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkpoint, and
        // fSigcheckVerificationFactor per transaction after.

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->GetBlockTime())/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return std::min(fWorkBefore / (fWorkBefore + fWorkAfter), 1.0);
    }

    int GetTotalBlocksEstimate(const CCheckpointData& data)
    {
        const MapCheckpoints& checkpoints = data.mapCheckpoints;

        if (checkpoints.empty())
            return 0;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const CCheckpointData& data)
    {
        const MapCheckpoints& checkpoints = data.mapCheckpoints;

        for (const MapCheckpoints::value_type& i : reverse_iterate(checkpoints))
        {
            const uint256& hash = i.second;
            BlockMap::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

    bool IsAncestorOfLastCheckpoint(const CCheckpointData& data, const CBlockIndex* pindex)
    {
        if (!fImporting) {
            CBlockIndex *pindexLastCheckpoint = GetLastCheckpoint(data);
            return pindexLastCheckpoint && pindexLastCheckpoint->GetAncestor(pindex->nHeight) == pindex;
        }

        // Workaround for block import:
        // If daemon is importing blocks from file there is no forward fetching of headers and mapBlockIndex won't contain next "last checkpoint"
        // GetLastCheckpoint() would return "previous last checkpoint" and our block can't be ancestor of it, only descendant
        // This renders -ibdskiptxverification (-fastsync) unusable when importing blocks from bootstrap.dat and expensive checks enabled even at heights lower than last checkpoint
        // Therefore, let's be permissive here and return true, at least until best block reaches last defined checkpoint height
        if (!data.mapCheckpoints.empty()) {
            int last_cp_height = std::prev(data.mapCheckpoints.end())->first;
            if (pindex->nHeight <= last_cp_height)
                return true;
        }

        return false;
    }



} // namespace Checkpoints
