// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2016-2023 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 270140;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version.
//! 270013 = Ycash v4.4.4, the oldest protocol legitimately on the
//! Ycash network; agreed as the floor across ycashd and Yolk/Zebra.
static const int MIN_PEER_PROTO_VERSION = 270013;

//! nTime field added to CAddress, starting with this version.
//! This can't be removed because it affects the encoding of the
//! `addrFrom` field in a "version" message.
static const int CADDR_TIME_VERSION = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 170004;

//! Changes to CInv parsing, starting with this version:
//! - MSG_WTX type defined, which contains two 32-byte hashes.
//! In the better-ycash lineage wtx inventory is introduced by this
//! build (no prior Ycash release spoke MSG_WTX), so this equals
//! PROTOCOL_VERSION.
static const int CINV_WTX_VERSION = 270140;

//! disconnect from testnet peers older than this proto version.
//! Same Ycash v4.4.4 floor (270013) as mainnet, aligned with
//! Yolk/Zebra.
static const int MIN_TESTNET_PEER_PROTO_VERSION = 270013;

#endif // BITCOIN_VERSION_H
