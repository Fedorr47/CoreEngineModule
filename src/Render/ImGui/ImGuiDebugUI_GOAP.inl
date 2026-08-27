namespace rendern::ui
{
    namespace
    {
        [[nodiscard]] std::uint32_t GOAPAgentId(const EntityHandle entity) noexcept
        {
            return static_cast<std::uint32_t>(entity);
        }
        
        const char* GOAPStatusName(const AIPlanExecutionStatus status)
        {
            switch (status)
            {
            case AIPlanExecutionStatus::NotStarted: return "NotStarted";
            case AIPlanExecutionStatus::ReadyToStartStep: return "ReadyToStartStep";
            case AIPlanExecutionStatus::RunningStep: return "RunningStep";
            case AIPlanExecutionStatus::Succeeded: return "Succeeded";
            case AIPlanExecutionStatus::Failed: return "Failed";
            case AIPlanExecutionStatus::Cancelled: return "Cancelled";
            }
            return "Unknown";
        }

        const char* GOAPOperatorName(const AINumericConditionOperator operation)
        {
            switch (operation)
            {
            case AINumericConditionOperator::Equal: return "==";
            case AINumericConditionOperator::NotEqual: return "!=";
            case AINumericConditionOperator::Less: return "<";
            case AINumericConditionOperator::LessOrEqual: return "<=";
            case AINumericConditionOperator::Greater: return ">";
            case AINumericConditionOperator::GreaterOrEqual: return ">=";
            }
            return "?";
        }

        std::string GOAPFactName(const std::string& name, const std::uint16_t id)
        {
            return name.empty() ? "Fact #" + std::to_string(id) : name;
        }

        std::string GOAPActionName(const std::string& name, const AIActionId id)
        {
            return name.empty() ? "Action #" + std::to_string(id.value) : name;
        }

        void DrawGOAPActionIdentity(const std::string& actionName, const AIActionId actionId,
            const std::string& contextName, const AIActionContextId contextId)
        {
            const std::string action = GOAPActionName(actionName, actionId);
            if (!contextName.empty())
            {
                ImGui::Text("%s / %s", action.c_str(), contextName.c_str());
            }
            else if (contextId.IsValid())
            {
                ImGui::Text("%s / Context #%u", action.c_str(), contextId.value);
            }
            else
            {
                ImGui::TextUnformatted(action.c_str());
            }
        }
    }

    void DrawGOAPDebugUI(GameplayRuntime* gameplayRuntime)
    {
        ImGui::Begin("AI / GOAP");
        if (gameplayRuntime == nullptr)
        {
            ImGui::TextDisabled("No GOAP agent selected");
            ImGui::End();
            return;
        }

        const std::vector<GameplayAIDebugAgentView> agents = gameplayRuntime->BuildAIDebugAgentViews();
        if (agents.empty())
        {
            ImGui::TextDisabled("No GOAP agent selected");
            ImGui::End();
            return;
        }

        static EntityHandle selectedAgent = kNullEntity;
        const auto selected = std::ranges::find_if(agents,
            [&](const auto& value) { return value.agent == selectedAgent; });
        const std::size_t selectedIndex = selected == agents.end()
            ? 0u : static_cast<std::size_t>(selected - agents.begin());
        selectedAgent = agents[selectedIndex].agent;
        const std::string selectedAgentLabel =
            std::to_string(GOAPAgentId(selectedAgent));
        if (agents.size() > 1u &&
            ImGui::BeginCombo("Agent", selectedAgentLabel.c_str()))
        {
            for (const auto& agent : agents)
            {
                const std::string label = std::to_string(GOAPAgentId(agent.agent));
                if (ImGui::Selectable(label.c_str(), agent.agent == selectedAgent))
                {
                    selectedAgent = agent.agent;
                }
            }
            ImGui::EndCombo();
        }
        else
        {
            ImGui::Text("Agent: %u", GOAPAgentId(selectedAgent));
        }

        const auto current = std::ranges::find_if(agents,
            [&](const auto& value) { return value.agent == selectedAgent; });
        const AIDebugViewModel& model = (current == agents.end() ? agents.front() : *current).snapshot;
        ImGui::SeparatorText("Decision");
        ImGui::Text("Status: %s", GOAPStatusName(model.decisionStatus));
        if (model.executionStatus)
        {
            ImGui::Text("Execution: %s", GOAPStatusName(*model.executionStatus));
        }
        else
        {
            ImGui::TextDisabled("No current execution");
        }
        if (model.decisionStatus == AIPlanExecutionStatus::Failed && !model.executionStatus)
        {
            ImGui::TextDisabled("Planning failed");
        }

        ImGui::SeparatorText("Goal");
        if (model.selectedGoalId)
        {
            const std::string goal = model.selectedGoalName.empty()
                ? "Goal #" + std::to_string(model.selectedGoalId->value) : model.selectedGoalName;
            ImGui::TextUnformatted(goal.c_str());
        }
        else
        {
            ImGui::TextDisabled("No selected goal");
        }

        ImGui::SeparatorText("World State");
        for (const auto& fact : model.booleanFacts)
        {
            const std::string name = GOAPFactName(fact.name, fact.factId.index);
            ImGui::Text("%-28s bool  %s", name.c_str(), fact.value ? "true" : "false");
        }
        for (const auto& fact : model.integerFacts)
        {
            const std::string name = GOAPFactName(fact.name, fact.factId.index);
            ImGui::Text("%-28s int   %d", name.c_str(), fact.value);
        }

        ImGui::SeparatorText("Selected Plan");
        if (model.selectedPlan.empty())
        {
            ImGui::TextDisabled(model.executionStatus == AIPlanExecutionStatus::Succeeded
                ? "Plan completed" : "No selected plan");
        }
        for (std::size_t index = 0; index < model.selectedPlan.size(); ++index)
        {
            const auto& step = model.selectedPlan[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::TextUnformatted(model.currentStepIndex == index ? ">" : " ");
            ImGui::SameLine();
            ImGui::Text("%zu", index);
            ImGui::SameLine();
            DrawGOAPActionIdentity(step.actionName, step.actionId, step.contextName, step.contextId);
            ImGui::SameLine();
            if (step.bCostResolved)
            {
                ImGui::Text("cost: %.2f", step.cost);
            }
            else
            {
                ImGui::TextDisabled("cost: unresolved");
            }
            ImGui::PopID();
        }
        if (model.bPlanCostComplete)
        {
            ImGui::Text("Plan cost: %.2f", model.totalPlanCost);
        }
        else
        {
            ImGui::Text("Plan cost: %.2f + unresolved", model.totalPlanCost);
            ImGui::TextDisabled("Plan cost incomplete");
        }

        ImGui::SeparatorText("Actions");
        for (std::size_t index = 0; index < model.actionApplicability.size(); ++index)
        {
            const auto& action = model.actionApplicability[index];
            ImGui::PushID(static_cast<int>(index));
            DrawGOAPActionIdentity(action.actionName, action.actionId,
                action.contextName, action.contextId);
            ImGui::Indent();
            ImGui::TextUnformatted(action.applicable ? "applicable" : "rejected");
            for (const auto& failure : action.failedBooleanConditions)
            {
                const std::string fact = GOAPFactName(failure.factName, failure.factId.index);
                ImGui::Text("%s == %s", fact.c_str(), failure.expected ? "true" : "false");
                ImGui::Text("  actual: %s", failure.actual ? "true" : "false");
            }
            for (const auto& failure : action.failedNumericConditions)
            {
                const std::string fact = GOAPFactName(failure.factName, failure.factId.index);
                ImGui::Text("%s %s %d", fact.c_str(), GOAPOperatorName(failure.comparison), failure.expected);
                ImGui::Text("  expected: %d", failure.expected);
                ImGui::Text("  actual:   %d", failure.actual);
            }
            ImGui::Unindent();
            ImGui::Spacing();
            ImGui::PopID();
        }
        ImGui::End();
    }
}