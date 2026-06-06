namespace rendern::ui
{
    namespace
    {
        struct PerformanceHistoryPlotContext
        {
            const rendern::PerformanceSnapshot* performanceSnapshot{ nullptr };
        };

        static float GetFrameTimeHistorySample(void* data, int sampleIndex)
        {
            const auto* context = static_cast<const PerformanceHistoryPlotContext*>(data);
            if (!context || !context->performanceSnapshot)
            {
                return 0.0f;
            }

            const rendern::PerformanceSnapshot& performanceSnapshot = *context->performanceSnapshot;
            const std::size_t sampleCount = performanceSnapshot.frameTimeHistoryCount;
            if (sampleIndex < 0 || static_cast<std::size_t>(sampleIndex) >= sampleCount)
            {
                return 0.0f;
            }

            const std::size_t firstSampleIndex = sampleCount == rendern::PerformanceSnapshot::FrameTimeHistoryCapacity
                ? performanceSnapshot.nextFrameTimeHistoryIndex
                : 0u;
            const std::size_t historyIndex =
                (firstSampleIndex + static_cast<std::size_t>(sampleIndex)) % rendern::PerformanceSnapshot::FrameTimeHistoryCapacity;
            return performanceSnapshot.frameTimeHistoryMs[historyIndex];
        }

        static void CalculateFrameTimeSummary(
            const rendern::PerformanceSnapshot& performanceSnapshot,
            float& averageFrameTimeMs,
            float& minimumFrameTimeMs,
            float& maximumFrameTimeMs)
        {
            const std::size_t sampleCount = performanceSnapshot.frameTimeHistoryCount;
            if (sampleCount == 0u)
            {
                averageFrameTimeMs = 0.0f;
                minimumFrameTimeMs = 0.0f;
                maximumFrameTimeMs = 0.0f;
                return;
            }

            float totalFrameTimeMs = 0.0f;
            PerformanceHistoryPlotContext context{ &performanceSnapshot };
            minimumFrameTimeMs = GetFrameTimeHistorySample(&context, 0);
            maximumFrameTimeMs = minimumFrameTimeMs;

            for (std::size_t sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
            {
                const float frameTimeMs = GetFrameTimeHistorySample(&context, static_cast<int>(sampleIndex));
                totalFrameTimeMs += frameTimeMs;
                minimumFrameTimeMs = std::min(minimumFrameTimeMs, frameTimeMs);
                maximumFrameTimeMs = std::max(maximumFrameTimeMs, frameTimeMs);
            }

            averageFrameTimeMs = totalFrameTimeMs / static_cast<float>(sampleCount);
        }

        struct CpuStageDisplayRow
        {
            const char* label{ "" };
            char millisecondsText[32]{};
        };

        struct CpuStageDisplayCache
        {
            static constexpr std::size_t RowCount = 11u;

            std::array<CpuStageDisplayRow, RowCount> cachedCpuStageRows{};
            double lastCpuStageDisplayRefreshTime{ 0.0 };
            bool initialized{ false };
        };

        static void SetCpuStageDisplayRow(CpuStageDisplayRow& row, const char* label, float milliseconds)
        {
            row.label = label;
            std::snprintf(row.millisecondsText, sizeof(row.millisecondsText), "%.3f", milliseconds);
        }

        static void RefreshCpuStageDisplayCache(
            CpuStageDisplayCache& cpuStageDisplayCache,
            const rendern::CpuFrameStageTimingSnapshot& cpuFrameStages,
            double refreshTimeSeconds)
        {
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[0], "Total before sleep", cpuFrameStages.totalBeforeSleepMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[1], "Total with sleep", cpuFrameStages.totalWithSleepMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[2], "Frame timing update", cpuFrameStages.updateFrameTimingMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[3], "Input", cpuFrameStages.inputMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[4], "Editor interaction", cpuFrameStages.editorInteractionMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[5], "Streaming", cpuFrameStages.streamingMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[6], "Gameplay + animation", cpuFrameStages.gameplayAndAnimationMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[7], "ImGui build", cpuFrameStages.buildImGuiMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[8], "Main viewport render", cpuFrameStages.renderMainViewportMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[9], "Debug window render", cpuFrameStages.renderDebugWindowMs);
            SetCpuStageDisplayRow(cpuStageDisplayCache.cachedCpuStageRows[10], "Tiny sleep", cpuFrameStages.tinySleepMs);

            cpuStageDisplayCache.lastCpuStageDisplayRefreshTime = refreshTimeSeconds;
            cpuStageDisplayCache.initialized = true;
        }

        static void DrawCpuStageRow(const CpuStageDisplayRow& row)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.label);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.millisecondsText);
        }
    }

    static void DrawPerformanceWindow(rendern::RendererSettings& rendererSettings)
    {
        if (!rendererSettings.showPerformancePanel)
        {
            return;
        }

        if (!ImGui::Begin("Performance / Profiler", &rendererSettings.showPerformancePanel))
        {
            ImGui::End();
            return;
        }

        const rendern::PerformanceSnapshot& performanceSnapshot = rendererSettings.performanceSnapshot;
        ImGui::Text("FPS: %.1f", performanceSnapshot.fps);
        ImGui::Text("Frame time: %.3f ms", performanceSnapshot.frameTimeMs);
        ImGui::TextDisabled("Raw frame delta: %.3f ms", performanceSnapshot.rawFrameTimeMs);

        float averageFrameTimeMs = 0.0f;
        float minimumFrameTimeMs = 0.0f;
        float maximumFrameTimeMs = 0.0f;
        CalculateFrameTimeSummary(performanceSnapshot, averageFrameTimeMs, minimumFrameTimeMs, maximumFrameTimeMs);
        ImGui::Text(
            "History (%zu): avg %.3f ms | min %.3f ms | max %.3f ms",
            performanceSnapshot.frameTimeHistoryCount,
            averageFrameTimeMs,
            minimumFrameTimeMs,
            maximumFrameTimeMs);

        PerformanceHistoryPlotContext plotContext{ &performanceSnapshot };
        ImGui::PlotLines(
            "Frame time history (ms)",
            GetFrameTimeHistorySample,
            &plotContext,
            static_cast<int>(performanceSnapshot.frameTimeHistoryCount),
            0,
            nullptr,
            0.0f,
            std::max(33.33f, maximumFrameTimeMs),
            ImVec2(0.0f, 90.0f));

        ImGui::SeparatorText("CPU frame stages");
        if (ImGui::BeginTable("PerformanceCpuFrameStages", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Stage");
            ImGui::TableSetupColumn("ms", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableHeadersRow();

            static CpuStageDisplayCache cpuStageDisplayCache{};
            constexpr double CpuStageDisplayRefreshIntervalSeconds = 0.25;
            const double currentTimeSeconds = ImGui::GetTime();
            if (!cpuStageDisplayCache.initialized ||
                currentTimeSeconds - cpuStageDisplayCache.lastCpuStageDisplayRefreshTime >= CpuStageDisplayRefreshIntervalSeconds)
            {
                RefreshCpuStageDisplayCache(cpuStageDisplayCache, performanceSnapshot.cpuFrameStages, currentTimeSeconds);
            }

            for (const CpuStageDisplayRow& cachedCpuStageRow : cpuStageDisplayCache.cachedCpuStageRows)
            {
                DrawCpuStageRow(cachedCpuStageRow);
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("GPU timings");
        if (performanceSnapshot.hasGpuTimings)
        {
            ImGui::TextUnformatted("GPU timings are available, but no shared GPU timing fields are exposed to this panel yet.");
        }
        else
        {
            ImGui::TextDisabled("GPU timings unavailable: the current shared performance snapshot does not expose GPU timestamp data.");
        }
        
        ImGui::End();
    }
}
