#include <gtest/gtest.h>

#include <cstdint>

namespace appLifecycle
{
    std::uint32_t ComputeStreamingWorkerCount(unsigned int hardwareThreadCount) noexcept;
}

TEST(AppLifeCycleStreamingWprkerCount, UnknownHardwareCountFallsBackToOne)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(0u), 1u);
}

TEST(AppLifeCycleStreamingWprkerCount, OneHardwareThreadReturnsOne)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(1u), 1u);
}

TEST(AppLifeCycleStreamingWprkerCount, InRangeHardwareThreadCountReservesMainThread)
{
    // Policy: worker count = hardware threads - 1, clamped to [1, 8].
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(4u), 3u);
}

TEST(AppLifeCycleStreamingWprkerCount, EightHardwareThreadsReservesMainThread)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(8u), 7u);
}

TEST(AppLifeCycleStreamingWprkerCount, AboveMaximumHardwareThreadCountClampsToEight)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(9u), 8u);
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(64u), 8u);
}