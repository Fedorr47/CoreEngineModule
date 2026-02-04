module;

#include <algorithm>
#include <chrono>
#include <cstdint>

export module core:gametimer_core;

// Small time utilities used across the project.
// This is a thin wrapper over std::chrono::steady_clock.

export inline constexpr double MaxDeltaSec = 1e-9;
export inline constexpr double FixedDeltaSec60 = 1.0 / 60.0;

export struct TimeState
{
    double totalSec{};
    double deltaSec{};
};

export class GameTimer
{
public:
    GameTimer() { Reset(); }
    ~GameTimer() = default;

    GameTimer(const GameTimer& other) = default;
    GameTimer(GameTimer&& other) noexcept = default;

    GameTimer& operator=(const GameTimer& other) = default;
    GameTimer& operator=(GameTimer&& other) noexcept = default;

    void Reset();
    void Start();
    void Stop();
    void Tick();

    double GetTotalTime() const;
    double GetDeltaTime() const;

    bool IsStopped() const { return stopped_; }

    void SetMaxDelta(double sec) { maxDeltaSec_ = std::max(0.0, sec); }
    double GetMaxDelta() const { return maxDeltaSec_; }

    TimeState GetState() const { return { GetTotalTime(), GetDeltaTime() }; }

private:
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

    double deltaTime_{ 0.0 };

    time_point baseTime_{};
    time_point previousTime_{};
    time_point currentTime_{};
    time_point stopTime_{};

    clock::duration pausedTime_{ clock::duration::zero() };

    bool stopped_{ false };
    double maxDeltaSec_{ 0.1 };
};

export struct FixedStepResult
{
    int tickToSimulate{ 0 };
    std::int64_t firstTickindex{ 0 };
    double alpha{ 0.0 };
};

export class FixedStepScheduler
{
public:
    explicit FixedStepScheduler(double fixedDeltaSec = FixedDeltaSec60)
        : fixedDeltaSec_{ std::max(fixedDeltaSec, MaxDeltaSec) }
    {
    }

    void SetFixedDeltaSec(double sec) { fixedDeltaSec_ = std::max(sec, MaxDeltaSec); }
    double GetFixedDeltaSec() const { return fixedDeltaSec_; }

    void SetMaxCatchupTicks(int numberOfTicks) { maxCatchupTicks_ = std::max(0, numberOfTicks); }
    int GetMaxCatchupTicks() const { return maxCatchupTicks_; }

    void Reset(std::int64_t tickIndex = 0)
    {
        accumulatedDeltaSec_ = 0.0;
        tickIndex_ = tickIndex;
    }

    FixedStepResult Advance(double frameDeltaSec);

private:
    double fixedDeltaSec_{ FixedDeltaSec60 };
    double accumulatedDeltaSec_{ 0.0 };
    std::int64_t tickIndex_{ 0 };
    int maxCatchupTicks_{ 8 };
};


void GameTimer::Reset()
{
    const auto now = clock::now();

    baseTime_ = now;
    previousTime_ = now;
    currentTime_ = now;

    stopTime_ = time_point{};
    pausedTime_ = clock::duration::zero();

    stopped_ = false;
    deltaTime_ = 0.0;
}

void GameTimer::Start()
{
    if (!stopped_)
    {
        return;
    }

    const auto startTime = clock::now();
    pausedTime_ += startTime - stopTime_;

    previousTime_ = startTime;
    currentTime_ = startTime;     // keep total time correct before first Tick()
    stopTime_ = time_point{};
    stopped_ = false;
}

void GameTimer::Stop()
{
    if (stopped_)
    {
        return;
    }

    stopTime_ = clock::now();
    stopped_ = true;
}

void GameTimer::Tick()
{
    if (stopped_)
    {
        deltaTime_ = 0.0;
        return;
    }

    currentTime_ = clock::now();
    const auto delta = currentTime_ - previousTime_;
    previousTime_ = currentTime_;

    deltaTime_ = std::chrono::duration<double>(delta).count();
    deltaTime_ = std::clamp(deltaTime_, 0.0, maxDeltaSec_);
}

double GameTimer::GetTotalTime() const
{
    const auto totalTime = stopped_ ? stopTime_ : currentTime_;
    const auto elapsed = (totalTime - baseTime_) - pausedTime_;
    return std::chrono::duration<double>(elapsed).count();
}

double GameTimer::GetDeltaTime() const
{
    return deltaTime_;
}

FixedStepResult FixedStepScheduler::Advance(double frameDeltaSec)
{
    if (frameDeltaSec < 0.0)
    {
        frameDeltaSec = 0.0;
    }

    accumulatedDeltaSec_ += frameDeltaSec;

    FixedStepResult result{};
    result.firstTickindex = tickIndex_;

    int tickCount = 0;
    while (accumulatedDeltaSec_ >= fixedDeltaSec_ && tickCount < maxCatchupTicks_)
    {
        accumulatedDeltaSec_ -= fixedDeltaSec_;
        ++tickCount;
        ++tickIndex_;
    }

    result.tickToSimulate = tickCount;
    result.alpha = std::clamp(accumulatedDeltaSec_ / fixedDeltaSec_, 0.0, 1.0);
    return result;
}
