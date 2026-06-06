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

        static void DrawCpuStageRow(const char* label, float milliseconds)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", milliseconds);
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

            const rendern::CpuFrameStageTimingSnapshot& cpuFrameStages = performanceSnapshot.cpuFrameStages;
            DrawCpuStageRow("Total before sleep", cpuFrameStages.totalBeforeSleepMs);
            DrawCpuStageRow("Total with sleep", cpuFrameStages.totalWithSleepMs);
            DrawCpuStageRow("Frame timing update", cpuFrameStages.updateFrameTimingMs);
            DrawCpuStageRow("Input", cpuFrameStages.inputMs);
            DrawCpuStageRow("Editor interaction", cpuFrameStages.editorInteractionMs);
            DrawCpuStageRow("Streaming", cpuFrameStages.streamingMs);
            DrawCpuStageRow("Gameplay + animation", cpuFrameStages.gameplayAndAnimationMs);
            DrawCpuStageRow("ImGui build", cpuFrameStages.buildImGuiMs);
            DrawCpuStageRow("Main viewport render", cpuFrameStages.renderMainViewportMs);
            DrawCpuStageRow("Debug window render", cpuFrameStages.renderDebugWindowMs);
            DrawCpuStageRow("Tiny sleep", cpuFrameStages.tinySleepMs);
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

        ImGui::Separator();
        ImGui::TextDisabled("Viewport FPS/CPU overlays are disabled by default; use Renderer / Shadows for fallback toggles.");
        ImGui::End();
    }
}
