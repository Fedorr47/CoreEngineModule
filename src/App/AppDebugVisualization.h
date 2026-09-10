#pragma once

namespace appLifecycle
{
    struct AppState;
}

namespace appDebugVisualization
{
    void UpdateRuntimeDebugSamples(appLifecycle::AppState& app);
    void ComposeExternalDebugGeometry(appLifecycle::AppState& app);
}
