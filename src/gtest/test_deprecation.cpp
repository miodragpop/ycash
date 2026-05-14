// Copyright (c) 2017-2023 The Zcash developers
// Copyright (c) 2026 The Ycash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

// Ycash does not enforce node deprecation. DEPRECATION_HEIGHT is pinned to
// INT_MAX in deprecation.h so EnforceNodeDeprecation's shutdown and warning
// branches are unreachable; the upstream Zcash test cases (warning at
// DEPRECATION_HEIGHT - DEPRECATION_WARN_LIMIT, shutdown at/after
// DEPRECATION_HEIGHT) would either overflow at DEPRECATION_HEIGHT + 1 or
// describe behavior that no longer exists. The tests below instead pin the
// new contract: the node never warns and never requests shutdown, on any
// network, at any height we can represent.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "chainparams.h"
#include "clientversion.h"
#include "deprecation.h"
#include "init.h"
#include "ui_interface.h"
#include "util/system.h"

#include <limits>
#include <string>

using namespace boost::placeholders;
using ::testing::StrictMock;

extern std::atomic<bool> fRequestShutdown;

class MockUIInterface {
public:
    MOCK_METHOD3(ThreadSafeMessageBox, bool(const std::string& message,
                                      const std::string& caption,
                                      unsigned int style));
};

static bool ThreadSafeMessageBox(MockUIInterface *mock,
                                 const std::string& message,
                                 const std::string& caption,
                                 unsigned int style)
{
    return mock->ThreadSafeMessageBox(message, caption, style);
}

class DeprecationTest : public ::testing::Test {
protected:
    void SetUp() override {
        uiInterface.ThreadSafeMessageBox.disconnect_all_slots();
        uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, &mock_, _1, _2, _3));
        SelectParams(CBaseChainParams::MAIN);
    }

    void TearDown() override {
        fRequestShutdown = false;
        mapArgs.clear();
    }

    StrictMock<MockUIInterface> mock_;
};

// Sentinel: deprecation is intentionally disabled in Ycash.
TEST_F(DeprecationTest, DeprecationIsDisabled) {
    EXPECT_EQ(DEPRECATION_HEIGHT, std::numeric_limits<int>::max());
}

// At genesis-like heights, the node must not warn or shut down. StrictMock
// will fail the test if ThreadSafeMessageBox is invoked.
TEST_F(DeprecationTest, NodeAtLowHeightNeitherWarnsNorShutsDown) {
    EXPECT_FALSE(ShutdownRequested());
    EnforceNodeDeprecation(Params(), 0);
    EnforceNodeDeprecation(Params(), 0, /*forceLogging=*/true);
    EXPECT_FALSE(ShutdownRequested());
}

// At any plausible Ycash chain height, the node must not warn or shut down.
TEST_F(DeprecationTest, NodeAtTypicalHeightNeitherWarnsNorShutsDown) {
    EXPECT_FALSE(ShutdownRequested());
    EnforceNodeDeprecation(Params(), 2'000'000);
    EnforceNodeDeprecation(Params(), 2'000'000, /*forceLogging=*/true);
    EXPECT_FALSE(ShutdownRequested());
}

// Even at the maximum representable height the node must not shut down.
// (DEPRECATION_HEIGHT is INT_MAX, so this is the boundary case.)
TEST_F(DeprecationTest, NodeAtIntMaxHeightDoesNotShutDown) {
    EXPECT_FALSE(ShutdownRequested());
    EnforceNodeDeprecation(Params(), std::numeric_limits<int>::max());
    EXPECT_FALSE(ShutdownRequested());
}

// EstimatedNodeDeprecationTime must not overflow when DEPRECATION_HEIGHT
// is pinned to INT_MAX -- the implementation saturates to INT64_MAX.
TEST_F(DeprecationTest, EstimatedDeprecationTimeIsSaturated) {
    FixedClock clock(std::chrono::seconds(1000));
    EXPECT_EQ(EstimatedNodeDeprecationTime(clock, 0),
              std::numeric_limits<int64_t>::max());
    EXPECT_EQ(EstimatedNodeDeprecationTime(clock, 2'000'000),
              std::numeric_limits<int64_t>::max());
}

// Regtest and testnet behavior is unchanged: enforcement is skipped there
// regardless of mainnet policy.
TEST_F(DeprecationTest, RegtestAndTestnetAreUnaffected) {
    SelectParams(CBaseChainParams::REGTEST);
    EnforceNodeDeprecation(Params(), 0);
    EXPECT_FALSE(ShutdownRequested());

    SelectParams(CBaseChainParams::TESTNET);
    EnforceNodeDeprecation(Params(), 0);
    EXPECT_FALSE(ShutdownRequested());
}
