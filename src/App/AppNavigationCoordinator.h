#pragma once

namespace appLifecycle
{
    struct AppState;
}

namespace appNavigationCoordinator
{
    void Advance(appLifecycle::AppState& app);
}
