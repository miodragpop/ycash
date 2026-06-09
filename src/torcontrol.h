// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2017-2023 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

/**
 * Functionality for communicating with Tor.
 */
#ifndef BITCOIN_TORCONTROL_H
#define BITCOIN_TORCONTROL_H

#include "scheduler.h"

extern const std::string DEFAULT_TOR_CONTROL;
// Default OFF: the implemented onion protocol is Tor v2 (NEW:RSA1024), which the
// Tor network disabled in Oct 2021, so auto-creating a hidden service by default
// only spins up a useless torcontrol thread + reconnect loop. Operators can still
// opt in with -listenonion=1. TODO: port Tor v3 (ED25519-V3) + i2p from Bitcoin.
static const bool DEFAULT_LISTEN_ONION = false;

void StartTorControl(boost::thread_group& threadGroup, CScheduler& scheduler);
void InterruptTorControl();
void StopTorControl();

#endif /* BITCOIN_TORCONTROL_H */
