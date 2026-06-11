#include <gtest/gtest.h>

#include <cstdint>

namespace appLifecycle
{
    std::uint32_t ComputeStreamingWorkerCount(unsigned int hardwareThreadCount) noexcept;
}

TEST(AppLifecycleStreamingWorkerCount, UnknownHardwareCountFallsBackToOne)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(0u), 1u);
}

TEST(AppLifecycleStreamingWorkerCount, OneHardwareThreadReturnsOne)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(1u), 1u);
}

TEST(AppLifecycleStreamingWorkerCount, InRangeHardwareThreadCountReservesMainThread)
{
    // Policy: worker count = hardware threads - 1, clamped to [1, 8].
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(4u), 3u);
}

TEST(AppLifecycleStreamingWorkerCount, EightHardwareThreadsReservesMainThread)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(8u), 7u);
}

TEST(AppLifecycleStreamingWorkerCount, AboveMaximumHardwareThreadCountClampsToEight)
{
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(9u), 8u);
    EXPECT_EQ(appLifecycle::ComputeStreamingWorkerCount(64u), 8u);
}