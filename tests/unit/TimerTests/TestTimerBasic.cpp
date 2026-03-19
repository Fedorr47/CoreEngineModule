#include <gtest/gtest.h>

import core;

TEST(GameTimer, ResetSetsZeroState)
{
    GameTimer timer;
    timer.Reset();
    const auto state = timer.GetState();

    EXPECT_DOUBLE_EQ(state.totalSec, 0.0);
    EXPECT_DOUBLE_EQ(state.deltaSec, 0.0);
}

TEST(GameTimer, StartState)
{
    GameTimer timer;
    timer.Start();

    const auto state = timer.GetState();

    EXPECT_DOUBLE_EQ(state.totalSec, 0.0);
    EXPECT_DOUBLE_EQ(state.deltaSec, 0.0);
}

TEST(GameTimer, StopAndRestartToggleStoppedFlag)
{
    GameTimer timer;
    timer.Start();
    timer.Stop();
    EXPECT_TRUE(timer.IsStopped());

    timer.Start();
    EXPECT_FALSE(timer.IsStopped());
}

TEST(GameTimer, TickProducesNonNegativeDeltaAndClampsToMax)
{
    GameTimer timer;
    timer.SetMaxDelta(0.0);
    timer.Start();
    timer.Tick();

    const auto state = timer.GetState();
    EXPECT_GE(state.totalSec, 0.0);
    EXPECT_DOUBLE_EQ(state.deltaSec, 0.0);
}

TEST(FixedStepScheduler, BaseInvariants)
{
	FixedStepScheduler scheduler(FixedDeltaSec60);
	scheduler.Reset(123);
    const FixedStepResult result = scheduler.Advance(0.0);

	EXPECT_EQ(result.tickToSimulate, 0);
    EXPECT_DOUBLE_EQ(result.alpha, 0.0);
	EXPECT_EQ(result.firstTickindex, 123);
}

TEST(FixedStepScheduler, AccumulateFractionWithoutTick)
{
    FixedStepScheduler scheduler(FixedDeltaSec60);
	scheduler.SetFixedDeltaSec(0.02);
    const FixedStepResult result = scheduler.Advance(0.01);

    EXPECT_EQ(result.tickToSimulate, 0);
    EXPECT_DOUBLE_EQ(result.alpha, 0.5);
    EXPECT_EQ(result.firstTickindex, 0);
}

TEST(FixedStepScheduler, AdvanceExactlyOneTick)
{
    FixedStepScheduler scheduler(FixedDeltaSec60);
    scheduler.SetFixedDeltaSec(0.02);
    const FixedStepResult result = scheduler.Advance(0.02);

    EXPECT_EQ(result.tickToSimulate, 1);
    EXPECT_DOUBLE_EQ(result.alpha, 0.0);
    EXPECT_EQ(result.firstTickindex, 0);
}

TEST(FixedStepScheduler, ConsecutiveFramesAccumulateIntoSingleTick)
{
    FixedStepScheduler scheduler(FixedDeltaSec60);
    scheduler.SetFixedDeltaSec(0.02);

    FixedStepResult result = scheduler.Advance(0.01);
    EXPECT_EQ(result.tickToSimulate, 0);
    EXPECT_DOUBLE_EQ(result.alpha, 0.5);
    EXPECT_EQ(result.firstTickindex, 0);

    result = scheduler.Advance(0.01);
    EXPECT_EQ(result.tickToSimulate, 1);
    EXPECT_DOUBLE_EQ(result.alpha, 0.0);
	EXPECT_EQ(result.firstTickindex, 0);
}

TEST(FixedStepScheduler, CatchupTicksAreClamped)
{
    FixedStepScheduler scheduler(0.01);
    scheduler.SetMaxCatchupTicks(3);
    const FixedStepResult result = scheduler.Advance(0.10);

    EXPECT_EQ(result.tickToSimulate, 3);
    EXPECT_EQ(result.firstTickindex, 0);
    EXPECT_GE(result.alpha, 0.0);
    EXPECT_LE(result.alpha, 1.0);
}
