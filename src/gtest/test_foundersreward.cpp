#include <gtest/gtest.h>

#include "main.h"
#include "util/moneystr.h"
#include "chainparams.h"
#include "fs.h"
#include "key_io.h"
#include "util/strencodings.h"
#include "zcash/Address.hpp"
#include "wallet/wallet.h"
#include "amount.h"
#include <memory>
#include <string>
#include <set>
#include <vector>
#include "util/system.h"
#include "util/test.h"

// To run tests:
// ./zcash-gtest --gtest_filter="FoundersRewardTest.*"

//
// Enable this test to generate and print 48 testnet 2-of-3 multisig addresses.
// The output can be copied into chainparams.cpp.
// The temporary wallet file can be renamed as wallet.dat and used for testing with zcashd.
//
#if 0
TEST(FoundersRewardTest, create_testnet_2of3multisig) {
    SelectParams(CBaseChainParams::TESTNET);
    fs::path pathTemp = fs::temp_directory_path() / fs::unique_path();
    fs::create_directories(pathTemp);
    mapArgs["-datadir"] = pathTemp.string();
    bool fFirstRun;
    auto pWallet = std::make_shared<CWallet>("wallet.dat");
    ASSERT_EQ(DB_LOAD_OK, pWallet->LoadWallet(fFirstRun));
    pWallet->TopUpKeyPool();
    std::cout << "Test wallet and logs saved in folder: " << pathTemp.native() << std::endl;
    
    int numKeys = 48;
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(3);
    CPubKey newKey;
    std::vector<std::string> addresses;
    KeyIO keyIO(Params());
    for (int i = 0; i < numKeys; i++) {
        ASSERT_TRUE(pWallet->GetKeyFromPool(newKey));
        pubkeys[0] = newKey;
        pWallet->SetAddressBook(newKey.GetID(), "", "receive");

        ASSERT_TRUE(pWallet->GetKeyFromPool(newKey));
        pubkeys[1] = newKey;
        pWallet->SetAddressBook(newKey.GetID(), "", "receive");

        ASSERT_TRUE(pWallet->GetKeyFromPool(newKey));
        pubkeys[2] = newKey;
        pWallet->SetAddressBook(newKey.GetID(), "", "receive");

        CScript result = GetScriptForMultisig(2, pubkeys);
        ASSERT_FALSE(result.size() > MAX_SCRIPT_ELEMENT_SIZE);
        CScriptID innerID(result);
        pWallet->AddCScript(result);
        pWallet->SetAddressBook(innerID, "", "receive");

        std::string address = keyIO.EncodeDestination(innerID);
        addresses.push_back(address);
    }
    
    // Print out the addresses, 4 on each line.
    std::string s = "vFoundersRewardAddress = {\n";
    int i=0;
    int colsPerRow = 4;
    ASSERT_TRUE(numKeys % colsPerRow == 0);
    int numRows = numKeys/colsPerRow;
    for (int row=0; row<numRows; row++) {
        s += "    ";
        for (int col=0; col<colsPerRow; col++) {
            s += "\"" + addresses[i++] + "\", ";
        }
        s += "\n";
    }
    s += "    };";
    std::cout << s << std::endl;

    pWallet->Flush(true);
}
#endif


static int GetLastFoundersRewardHeight(const Consensus::Params& params) {
    int blossomActivationHeight = Params().GetConsensus().vUpgrades[Consensus::UPGRADE_BLOSSOM].nActivationHeight;
    int ycashActivationHeight = Params().GetConsensus().vUpgrades[Consensus::UPGRADE_YCASH].nActivationHeight;
    bool blossom = blossomActivationHeight != Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
    bool ycash = ycashActivationHeight != Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
    // Ycash founders reward (5% YDF) continues past the Zcash pre-Blossom
    // halving boundary that would have ended Zcash's 20% schedule, so when
    // UPGRADE_YCASH is configured we compute the boundary using the pre-
    // Blossom path regardless of Blossom activation.
    return params.GetLastFoundersRewardBlockHeight((blossom && !ycash) ? blossomActivationHeight : 0);
}

// Utility method to check the number of unique addresses from height 1 to maxHeight
void checkNumberOfUniqueAddresses(int nUnique) {
    std::set<std::string> addresses;
    for (int i = 1; i <= GetLastFoundersRewardHeight(Params().GetConsensus()); i++) {
        addresses.insert(Params().GetFoundersRewardAddressAtHeight(i));
    }
    EXPECT_EQ(addresses.size(), nUnique);
}



TEST(FoundersRewardTest, General) {
    SelectParams(CBaseChainParams::TESTNET);
    KeyIO keyIO(Params());

    CChainParams params = Params();

    // Pre-UPGRADE_YCASH heights return the canonical-prefixed (s2*) Ycash
    // encoding of the legacy Zcash multisig address.
    EXPECT_EQ(HexStr(params.GetFoundersRewardScriptAtHeight(1)), "a914ef775f1f997f122a062fff1a2d7443abd1f9c64287");
    EXPECT_EQ(params.GetFoundersRewardAddressAtHeight(1), keyIO.ZecToYec("t2UNzUUx8mWBCRYPRezvA363EYXyEpHokyi"));
    EXPECT_EQ(HexStr(params.GetFoundersRewardScriptAtHeight(53126)), "a914ac67f4c072668138d88a86ff21b27207b283212f87");
    EXPECT_EQ(params.GetFoundersRewardAddressAtHeight(53126), keyIO.ZecToYec("t2NGQjYMQhFndDHguvUw4wZdNdsssA6K7x2"));
    EXPECT_EQ(HexStr(params.GetFoundersRewardScriptAtHeight(53127)), "a91455d64928e69829d9376c776550b6cc710d42715387");
    EXPECT_EQ(params.GetFoundersRewardAddressAtHeight(53127), keyIO.ZecToYec("t2ENg7hHVqqs9JwU5cgjvSbxnT2a9USNfhy"));

    // At and past UPGRADE_YCASH the YDF cycle takes over; the address is
    // returned as a P2PKH (OP_DUP OP_HASH160 ... OP_EQUALVERIFY OP_CHECKSIG)
    // rather than P2SH.
    int ycashHeight = params.GetConsensus().vUpgrades[Consensus::UPGRADE_YCASH].nActivationHeight;
    EXPECT_EQ(params.GetFoundersRewardAddressAtHeight(ycashHeight), "smDw2LWkeuJ1NGBDDZvdNbzY8A9D1mkkDZm");
    EXPECT_EQ(HexStr(params.GetFoundersRewardScriptAtHeight(ycashHeight)), "76a91409beeb250c2f6b918dbd5e5a065f5b14d51faea288ac");

    // The YDF schedule continues past the pre-Blossom-derived maxHeight that
    // historically capped Zcash's founders reward.
    int zcashMaxHeight = GetLastFoundersRewardHeight(params.GetConsensus());
    EXPECT_EQ(params.GetFoundersRewardAddressAtHeight(zcashMaxHeight + 1), "smLTH7FEiXUVpWjhoL91ToMoSZPU8xAvEh9");
    EXPECT_EQ(HexStr(params.GetFoundersRewardScriptAtHeight(zcashMaxHeight + 1)), "76a91451487b85afdb97974bc0aa5aeac78fff692715be88ac");

    // The "out of bounds" EXPECT_DEATH cases no longer apply because the YDF
    // schedule is unbounded post-UPGRADE_YCASH (addresses cycle modulo the
    // vYcashFoundersRewardAddress size).
    EXPECT_DEATH(params.GetFoundersRewardScriptAtHeight(0), "nHeight");
    EXPECT_DEATH(params.GetFoundersRewardAddressAtHeight(0), "nHeight");
}

TEST(FoundersRewardTest, RegtestGetLastBlockBlossom) {
    int blossomActivationHeight = Consensus::PRE_BLOSSOM_REGTEST_HALVING_INTERVAL / 2; // = 75
    auto params = RegtestActivateBlossom(false, blossomActivationHeight).GetConsensus();
    int lastFRHeight = params.GetLastFoundersRewardBlockHeight(blossomActivationHeight);
    EXPECT_EQ(0, params.Halving(lastFRHeight));
    EXPECT_EQ(1, params.Halving(lastFRHeight + 1));
    RegtestDeactivateBlossom();
}

TEST(FoundersRewardTest, MainnetGetLastBlock) {
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus();
    int lastFRHeight = GetLastFoundersRewardHeight(params);
    EXPECT_EQ(0, params.Halving(lastFRHeight));
    EXPECT_EQ(1, params.Halving(lastFRHeight + 1));
}

// The Ycash fork landed mid-address-transition, so the address-change boundary
// at height 570000 forces an extra distinct address into the iteration: the
// final pre-fork Zcash slot reached, plus the YDF cycle that begins at fork.
#define NUM_MAINNET_FOUNDER_ADDRESSES (48 + 1)

TEST(FoundersRewardTest, Mainnet) {
    SelectParams(CBaseChainParams::MAIN);
    checkNumberOfUniqueAddresses(NUM_MAINNET_FOUNDER_ADDRESSES);
}


#define NUM_TESTNET_FOUNDER_ADDRESSES 48

TEST(FoundersRewardTest, Testnet) {
    SelectParams(CBaseChainParams::TESTNET);
    checkNumberOfUniqueAddresses(NUM_TESTNET_FOUNDER_ADDRESSES);
}


#define NUM_REGTEST_FOUNDER_ADDRESSES 1

TEST(FoundersRewardTest, Regtest) {
    SelectParams(CBaseChainParams::REGTEST);
    checkNumberOfUniqueAddresses(NUM_REGTEST_FOUNDER_ADDRESSES);
}



// Test that the inherited Zcash 20% founders reward, summed over the pre-fork
// portion of the founders reward schedule, equals 10% of MAX_MONEY. This
// matches the original Zcash invariant when applied to the founders window
// that exists pre-UPGRADE_YCASH, because the formula sums (subsidy / 5) over
// the same height range and pre-fork Ycash inherits Zcash's subsidy schedule.
TEST(FoundersRewardTest, SlowStartSubsidy) {
    SelectParams(CBaseChainParams::MAIN);
    CChainParams params = Params();

    CAmount totalSubsidy = 0;
    for (int nHeight = 1; nHeight <= GetLastFoundersRewardHeight(Params().GetConsensus()); nHeight++) {
        CAmount nSubsidy = params.GetConsensus().GetBlockSubsidy(nHeight) / 5;
        totalSubsidy += nSubsidy;
    }

    ASSERT_TRUE(totalSubsidy == MAX_MONEY/10.0);
}


// Verify the count of pre-UPGRADE_YCASH founder reward payouts per Zcash-era
// address. numRewardAddresses is the count of full Zcash founder slots
// reached before UPGRADE_YCASH activation; the slot after it has a partial
// count (3312 blocks on mainnet, 14396 on testnet).
void verifyNumberOfRewards(int numRewardAddresses) {
    CChainParams params = Params();

    int maxHeight = params.GetConsensus().vUpgrades[Consensus::UPGRADE_YCASH].nActivationHeight;
    std::multiset<std::string> ms;
    for (int nHeight = 1; nHeight < maxHeight; nHeight++) {
        ms.insert(params.GetFoundersRewardAddressAtHeight(nHeight));
    }

    // Slot 0 is one short of the full 17709-block interval because of the
    // slow-start shift.
    ASSERT_EQ(ms.count(params.GetZcashFoundersRewardAddressAtIndex(0)), 17708);
    for (int i = 1; i <= numRewardAddresses - 1; i++) {
        ASSERT_EQ(ms.count(params.GetZcashFoundersRewardAddressAtIndex(i)), 17709);
    }
    // The slot the fork landed in receives only a partial count.
    if (params.NetworkIDString() == "test") {
        ASSERT_EQ(ms.count(params.GetZcashFoundersRewardAddressAtIndex(numRewardAddresses)), 14396);
    } else {
        ASSERT_EQ(ms.count(params.GetZcashFoundersRewardAddressAtIndex(numRewardAddresses)), 3312);
    }
}

// Verify the post-UPGRADE_YCASH YDF address rotation. We iterate two full
// cycles of vYcashFoundersRewardAddress (48 addresses x 17917 blocks each)
// and confirm each address gets hit exactly twice.
void verifyNumberOfYcashRewards(int numRewardAddresses) {
    CChainParams params = Params();

    int startHeight = params.GetConsensus().vUpgrades[Consensus::UPGRADE_YCASH].nActivationHeight;
    const int addressChangeInterval = 17917;
    int endHeight = startHeight + (addressChangeInterval * 48 * 2);
    std::multiset<std::string> ms;
    for (int nHeight = startHeight; nHeight < endHeight; nHeight++) {
        ms.insert(params.GetFoundersRewardAddressAtHeight(nHeight));
    }
    for (int i = 0; i < numRewardAddresses; i++) {
        ASSERT_EQ(ms.count(params.GetYcashFoundersRewardAddressAtIndex(i)), addressChangeInterval * 2);
    }
}

// Verify the number of rewards going to each mainnet address. 32 full Zcash
// slots are reached before the fork; the 33rd slot is partial. After the
// fork, the YDF cycle takes over.
TEST(FoundersRewardTest, PerAddressRewardMainnet) {
    SelectParams(CBaseChainParams::MAIN);
    verifyNumberOfRewards(32);
    verifyNumberOfYcashRewards(48);
}

// Verify the number of rewards going to each testnet address. 28 full Zcash
// slots are reached before the fork; the 29th slot is partial. After the
// fork, the YDF cycle takes over.
TEST(FoundersRewardTest, PerAddressRewardTestnet) {
    SelectParams(CBaseChainParams::TESTNET);
    verifyNumberOfRewards(28);
    verifyNumberOfYcashRewards(48);
}

// FundingStreamsRewardTest tests removed: Ycash has no ZIP 207 funding
// streams; the related infrastructure is gone from the consensus layer.
