namespace rendern::ui
{
    namespace
    {
        struct InputBindingKeyChoice { int key; const char* name; };

        const std::vector<InputBindingKeyChoice>& InputBindingKeyChoices()
        {
            static const std::vector<InputBindingKeyChoice> choices = []
            {
                std::vector<InputBindingKeyChoice> result{
                    { kGameplayMouseLeft, "Mouse Left" }, { kGameplayMouseRight, "Mouse Right (Reserved for look)" },
                    { kGameplayMouseMiddle, "Mouse Middle" }, { kGameplayMouseX1, "Mouse X1" }, { kGameplayMouseX2, "Mouse X2" },
                    { 0x20, "Space" }, { 0x10, "Shift" }, { 0x11, "Control" },
                    { 0x0D, "Enter" }, { 0x1B, "Escape" }, { 0x09, "Tab" }, { 0x2E, "Delete" }
                };
                static std::array<std::array<char, 2>, 36> alphaNumericNames{};
                std::size_t nameIndex = 0;
                for (int key = 'A'; key <= 'Z'; ++key)
                {
                    alphaNumericNames[nameIndex] = { static_cast<char>(key), '\0' };
                    result.push_back({ key, alphaNumericNames[nameIndex++].data() });
                }
                for (int key = '0'; key <= '9'; ++key)
                {
                    alphaNumericNames[nameIndex] = { static_cast<char>(key), '\0' };
                    result.push_back({ key, alphaNumericNames[nameIndex++].data() });
                }
                static constexpr std::array<const char*, 12> functionNames{
                    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"
                };
                for (int index = 0; index < 12; ++index) result.push_back({ 0x70 + index, functionNames[index] });
                return result;
            }();
            return choices;
        }

        const char* InputBindingKeyName(const int key)
        {
            if (key == 0x74) return "F5 (Reserved)";
            if (key == 0x75) return "F6 (Reserved)";
            if (key == 0x76) return "F7 (Reserved)";
            for (const auto& choice : InputBindingKeyChoices()) if (choice.key == key) return choice.name;
            return "Unassigned";
        }

        const char* InputBindingActionName(const GameplayActionId& action)
        {
            return action.IsValid() ? action.value.c_str() : "None";
        }
    }

    void DrawInputBindingsUI(GameplayRuntime* gameplayRuntime)
    {
        ImGui::Begin("Input Bindings");
        if (gameplayRuntime == nullptr)
        {
            ImGui::TextDisabled("Gameplay runtime is unavailable.");
            ImGui::End();
            return;
        }

        static GameplayRuntime* stateRuntime = nullptr;
        static GameplayKeyboardMouseBindings working{};
        static bool dirty = false;
        static int newKey = kGameplayMouseLeft;
        static GameplayActionId newAction = kGameplayActionLightAttack;
        static std::string status{};
        static GameplayActionDefinitions workingDefinitions{};
        static GameplayActionAnimationBindings workingAnimationBindings{};
        static bool actionsDirty = false;
        if (stateRuntime != gameplayRuntime)
        {
            stateRuntime = gameplayRuntime;
            working = gameplayRuntime->GetKeyboardMouseBindings();
            workingDefinitions = gameplayRuntime->GetGameplayActionDefinitions();
            workingAnimationBindings = gameplayRuntime->GetGameplayActionAnimationBindings();
            dirty = false;
            actionsDirty = false;
            status.clear();
        }

        ImGui::SeparatorText("Gameplay Actions");
        static char newActionId[128]{ "Combat.PunchingAttack" };
        for (std::size_t index = 0; index < workingDefinitions.size(); ++index)
        {
            GameplayActionDefinition& definition = workingDefinitions[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::TextUnformatted(definition.id.value.c_str());
            actionsDirty |= ImGui::InputInt("Priority", &definition.priority);
            int group = static_cast<int>(definition.group);
            int source = static_cast<int>(definition.source);
            int executor = static_cast<int>(definition.executor);
            if (ImGui::Combo("Group", &group, "None\0Input\0Combat\0Interaction\0Any\0")) { definition.group = static_cast<GameplayActionPolicyGroup>(group); actionsDirty = true; }
            if (ImGui::Combo("Source", &source, "None\0Input\0Combat\0Interaction\0AnimationEvent\0Script\0")) { definition.source = static_cast<GameplayActionRequestSource>(source); actionsDirty = true; }
            if (ImGui::Combo("Executor", &executor, "None\0Jump\0CombatAttack\0Interact\0")) { definition.executor = static_cast<GameplayActionExecutorKind>(executor); actionsDirty = true; }
            const std::array gateChoices{
                std::pair{ GameplayActionPolicyGate::RequireGrounded, "Require Grounded" },
                std::pair{ GameplayActionPolicyGate::RequireAirborne, "Require Airborne" },
                std::pair{ GameplayActionPolicyGate::RequireNotBusy, "Require Not Busy" },
                std::pair{ GameplayActionPolicyGate::RequireBusy, "Require Busy" },
                std::pair{ GameplayActionPolicyGate::RequireNoPending, "Require No Pending" },
                std::pair{ GameplayActionPolicyGate::RequireNoBuffered, "Require No Buffered" }
            };
            for (const auto& [gate, label] : gateChoices)
            {
                bool enabled = HasGameplayActionPolicyGate(definition.gates, gate);
                if (ImGui::Checkbox(label, &enabled))
                {
                    const std::uint32_t bit = GameplayActionPolicyGateMask(gate);
                    definition.gates = enabled ? definition.gates | bit : definition.gates & ~bit;
                    actionsDirty = true;
                }
            }
            GameplayActionAnimationBinding* presentation = nullptr;
            for (auto& binding : workingAnimationBindings) if (binding.actionId == definition.id) { presentation = &binding; break; }
            char trigger[128]{};
            if (presentation != nullptr) std::snprintf(trigger, sizeof(trigger), "%s", presentation->triggerParameter.c_str());
            if (ImGui::InputText("Animation Trigger", trigger, sizeof(trigger)))
            {
                if (presentation == nullptr && trigger[0] != '\0') workingAnimationBindings.push_back({ definition.id, trigger });
                else if (presentation != nullptr) presentation->triggerParameter = trigger;
                actionsDirty = true;
            }
            if (presentation != nullptr && ImGui::SmallButton("Remove Presentation Binding"))
            {
                workingAnimationBindings.erase(std::remove_if(workingAnimationBindings.begin(), workingAnimationBindings.end(),
                    [&](const GameplayActionAnimationBinding& binding) { return binding.actionId == definition.id; }), workingAnimationBindings.end());
                presentation = nullptr;
                actionsDirty = true;
            }
            const auto inputReference = std::find_if(working.actions.begin(), working.actions.end(), [&](const auto& binding) { return binding.action == definition.id; });
            const bool presentationReferenced = presentation != nullptr;
            const bool required = IsRequiredGameplayAction(definition.id);
            const bool referenced = inputReference != working.actions.end() || presentationReferenced || required;
            ImGui::BeginDisabled(referenced);
            if (ImGui::SmallButton("Delete")) { workingDefinitions.erase(workingDefinitions.begin() + static_cast<std::ptrdiff_t>(index)); --index; actionsDirty = true; }
            ImGui::EndDisabled();
            if (referenced && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                if (required) ImGui::SetTooltip("Required by built-in physical jump integration.");
                else if (inputReference != working.actions.end()) ImGui::SetTooltip("Referenced by keyboard binding %s.", InputBindingKeyName(inputReference->key));
                else ImGui::SetTooltip("Remove its animation presentation binding before deletion.");
            }
            ImGui::Separator(); ImGui::PopID();
        }
        ImGui::InputText("New Id", newActionId, sizeof(newActionId));
        if (ImGui::Button("Add Action"))
        {
            const GameplayActionId id{ newActionId };
            if (!id.IsValid()) status = "Gameplay action ID cannot be empty.";
            else if (FindGameplayActionDefinition(workingDefinitions, id) != nullptr) status = "Gameplay action ID already exists.";
            else { workingDefinitions.push_back({ id, GameplayActionPolicyGroup::Combat, GameplayActionRequestSource::Combat, GameplayActionExecutorKind::CombatAttack, 10, GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireGrounded) }); newAction = id; actionsDirty = true; status = "Gameplay action added to working copy."; }
        }
        const bool inheritedDisabled =
    (GImGui->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;

        assert(!inheritedDisabled);
        ImGui::BeginDisabled(!actionsDirty);
        if (ImGui::Button("Save / Apply Actions"))
        {
            if (gameplayRuntime->ApplyGameplayActionConfiguration(workingDefinitions, workingAnimationBindings, status))
            {
                actionsDirty = false;
                status = "Gameplay actions and presentation bindings saved and applied.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard / Reload"))
        {
            workingDefinitions = gameplayRuntime->GetGameplayActionDefinitions();
            workingAnimationBindings = gameplayRuntime->GetGameplayActionAnimationBindings();
            actionsDirty = false;
            status = "Gameplay action changes discarded.";
        }
        ImGui::EndDisabled();

        ImGui::SeparatorText("Keyboard / Mouse Actions");
        if (ImGui::BeginTable("KeyboardMouseActions", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Input"); 
            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupColumn("##Remove");
            ImGui::TableHeadersRow();
            for (std::size_t index = 0; index < working.actions.size();)
            {
                const auto& binding = working.actions[index];
                ImGui::PushID(static_cast<int>(index)); ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(InputBindingKeyName(binding.key));
                ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(InputBindingActionName(binding.action));
                ImGui::TableSetColumnIndex(2);
                if (ImGui::SmallButton("Remove"))
                {
                    working.actions.erase(working.actions.begin() + static_cast<std::ptrdiff_t>(index));
                    dirty = true; status = "Binding removed from working copy."; ImGui::PopID(); continue;
                }
                ImGui::PopID(); ++index;
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("Add Binding");
        if (ImGui::BeginCombo("Action", InputBindingActionName(newAction)))
        {
            for (const GameplayActionDefinition& definition : workingDefinitions)
            {
                if (ImGui::Selectable(InputBindingActionName(definition.id), definition.id == newAction))
                {
                    newAction = definition.id;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("Input", InputBindingKeyName(newKey)))
        {
            for (const auto& choice : InputBindingKeyChoices())
            {
                const bool reserved = IsGameplayActionBindingKeyReserved(choice.key);
                ImGui::BeginDisabled(reserved);
                if (ImGui::Selectable(InputBindingKeyName(choice.key), choice.key == newKey) && !reserved)
                {
                    newKey = choice.key;
                }
                ImGui::EndDisabled();
                if (reserved && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip(choice.key == kGameplayMouseRight ? "Reserved for relative camera look." : "Reserved for an application hotkey.");
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Add Binding"))
        {
            const int candidateKey = newKey;
            const auto conflict = std::find_if(
                working.actions.begin(),
                working.actions.end(),
                [candidateKey](const GameplayActionKeyBinding& binding)
                {
                    return binding.key == candidateKey;
                });
            if (IsGameplayActionBindingKeyReserved(newKey)) {status = "That input is reserved for an application hotkey or camera look.";}
            else if (newKey == 0 || !newAction.IsValid()) {status = "Choose a valid input and action.";}
            else if (conflict != working.actions.end()) {status = "That input is already bound. Remove its binding first.";}
            else { working.actions.push_back({ newKey, newAction }); dirty = true; status = "Binding added to working copy."; }
        }
        const bool canApplyBindings = dirty && !actionsDirty;

        ImGui::BeginDisabled(!canApplyBindings);

        if (ImGui::Button("Apply"))
        {
            const bool containsReservedKey = std::any_of(
                working.actions.begin(),
                working.actions.end(),
                [](const GameplayActionKeyBinding& binding)
                {
                    return IsGameplayActionBindingKeyReserved(binding.key);
                });

            if (containsReservedKey)
            {
                status = "Cannot apply: remove bindings that use reserved application inputs.";
            }
            else
            {
                const bool rebound =
                    gameplayRuntime->ApplyKeyboardMouseBindings(working);

                if (rebound)
                {
                    dirty = false;
                    status = "Saved and applied (or stored for the next controlled entity).";
                }
                else
                {
                    status = "Failed to apply keyboard/mouse bindings.";
                }
            }
        }

        ImGui::EndDisabled();

        if (dirty && actionsDirty)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("Apply Gameplay Actions first.");
        }
        else
        {
            ImGui::SameLine();
            ImGui::TextDisabled(dirty ? "Unsaved changes" : "Up to date");
        }
        
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
        ImGui::End();
    }
}