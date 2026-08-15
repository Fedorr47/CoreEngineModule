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

        const char* InputBindingActionName(const GameplayActionKind action)
        {
            switch (action)
            {
            case GameplayActionKind::LightAttack: return "LightAttack";
            case GameplayActionKind::Interact: return "Interact";
            case GameplayActionKind::Jump: return "Jump";
            default: return "None";
            }
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
        static int newKey = 0x78;
        static GameplayActionKind newAction = GameplayActionKind::LightAttack;
        static std::string status{};
        if (stateRuntime != gameplayRuntime)
        {
            stateRuntime = gameplayRuntime;
            working = gameplayRuntime->GetKeyboardMouseBindings();
            dirty = false;
            status.clear();
        }

        ImGui::SeparatorText("Keyboard Actions");
        if (ImGui::BeginTable("KeyboardActions", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Key"); ImGui::TableSetupColumn("Action"); ImGui::TableSetupColumn("##Remove");
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
            for (const GameplayActionKind action : { GameplayActionKind::LightAttack, GameplayActionKind::Interact, GameplayActionKind::Jump })
                if (ImGui::Selectable(InputBindingActionName(action), action == newAction)) newAction = action;
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("Key", InputBindingKeyName(newKey)))
        {
            for (const auto& choice : InputBindingKeyChoices())
            {
                const bool reserved = IsGameplayActionBindingKeyReserved(choice.key);
                ImGui::BeginDisabled(reserved);
                if (ImGui::Selectable(InputBindingKeyName(choice.key), choice.key == newKey) && !reserved) newKey = choice.key;
                ImGui::EndDisabled();
                if (reserved && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Reserved for an application hotkey.");
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
            if (IsGameplayActionBindingKeyReserved(newKey)) status = "F5, F6, and F7 are reserved application hotkeys.";
            else if (newKey == 0 || newAction == GameplayActionKind::None) status = "Choose a valid key and action.";
            else if (conflict != working.actions.end()) status = "That key is already bound. Remove its binding first.";
            else { working.actions.push_back({ newKey, newAction }); dirty = true; status = "Binding added to working copy."; }
        }
        ImGui::BeginDisabled(!dirty);
        if (ImGui::Button("Apply"))
        {
            const bool containsReservedKey = std::any_of(working.actions.begin(), working.actions.end(),
                [](const GameplayActionKeyBinding& binding) { return IsGameplayActionBindingKeyReserved(binding.key); });
            if (containsReservedKey)
            {
                status = "Cannot apply: remove bindings that use reserved F5, F6, or F7 keys.";
            }
            else
            {
                const bool rebound = gameplayRuntime->ApplyKeyboardMouseBindings(working);
                dirty = false;
                status = rebound ? "Applied to the controlled entity." : "Saved for the next controlled entity; none is currently available.";
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine(); ImGui::TextDisabled(dirty ? "Unsaved changes" : "Up to date");
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
        ImGui::End();
    }
}