[[nodiscard]] inline bool ConditionUsesValue(
    const AnimationConditionOp op) noexcept
{
    switch (op)
    {
    case AnimationConditionOp::Greater:
    case AnimationConditionOp::GreaterEqual:
    case AnimationConditionOp::Less:
    case AnimationConditionOp::LessEqual:
    case AnimationConditionOp::Equal:
    case AnimationConditionOp::NotEqual:
        return true;

    case AnimationConditionOp::IfTrue:
    case AnimationConditionOp::IfFalse:
    case AnimationConditionOp::Triggered:
        return false;
    }

    return false;
}

[[nodiscard]] inline bool
EquivalentParameterValues(const AnimationParameterValue &left,
                          const AnimationParameterValue &right) {
  if (left.type != right.type)
    return false;
  switch (left.type) {
  case AnimationParameterType::Bool:
    return left.boolValue == right.boolValue;
  case AnimationParameterType::Int:
    return left.intValue == right.intValue;
  case AnimationParameterType::Float:
    return left.floatValue == right.floatValue;
  case AnimationParameterType::Trigger:
    return left.triggerValue == right.triggerValue;
  }
  return false;
}

[[nodiscard]] inline bool
EquivalentConditions(const AnimationConditionDesc &left,
                     const AnimationConditionDesc &right) {
  if (left.parameter != right.parameter || left.op != right.op)
    return false;
  switch (left.op) {
  case AnimationConditionOp::IfTrue:
  case AnimationConditionOp::IfFalse:
  case AnimationConditionOp::Triggered:
    return true;
  case AnimationConditionOp::Greater:
  case AnimationConditionOp::GreaterEqual:
  case AnimationConditionOp::Less:
  case AnimationConditionOp::LessEqual:
  case AnimationConditionOp::Equal:
  case AnimationConditionOp::NotEqual:
    return EquivalentParameterValues(left.value, right.value);
  }
  return false;
}

[[nodiscard]] inline bool EquivalentConditionLists(
    const std::vector<AnimationConditionDesc>& left,
    const std::vector<AnimationConditionDesc>& right) noexcept
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index)
    {
        if (!EquivalentConditions(left[index], right[index]))
        {
            return false;
        }
    }
    return true;
}

namespace detail {
[[nodiscard]] inline bool ContainsString(const std::vector<std::string> &values,
                                         std::string_view value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

[[nodiscard]] inline std::string
TransitionOriginLabel(const EffectiveAnimationTransition &transition) {
  switch (transition.origin) {
  case AnimationTransitionOrigin::Explicit:
    return "explicit transition";
  case AnimationTransitionOrigin::ExplicitWildcard:
    return "wildcard transition";
  case AnimationTransitionOrigin::Rule:
    return "rule '" + transition.sourceRuleId + "'";
  }
  return "transition";
}
    
[[nodiscard]] inline bool EquivalentTransitions(
    const AnimationTransitionDesc& left,
    const AnimationTransitionDesc& right) noexcept
{
    return left.fromState == right.fromState &&
           left.toState == right.toState &&
           left.hasExitTime == right.hasExitTime &&
           left.exitTimeNormalized == right.exitTimeNormalized &&
           left.blendDurationSeconds == right.blendDurationSeconds &&
           left.priority == right.priority &&
           EquivalentConditionLists(left.conditions, right.conditions);
}
} // namespace detail

inline bool MatchesAnimationState(const AnimationStateDesc &state,
                                  const AnimationStateSelector &selector) {
  if (!selector.states.empty() &&
      !detail::ContainsString(selector.states, state.name))
    return false;
  for (const std::string &tag : selector.allTags) {
    if (!detail::ContainsString(state.tags, tag))
      return false;
  }
  if (!selector.anyTags.empty() &&
      std::none_of(selector.anyTags.begin(), selector.anyTags.end(),
                   [&](const std::string &tag) {
                     return detail::ContainsString(state.tags, tag);
                   }))
    return false;
  for (const std::string &tag : selector.noneTags) {
    if (detail::ContainsString(state.tags, tag))
      return false;
  }
  return true;
}

inline std::vector<int>
ResolveAnimationStateSelector(const AnimationControllerAsset &controller,
                              const AnimationStateSelector &selector) {
  std::vector<int> matches;
  for (std::size_t index = 0; index < controller.states.size(); ++index) {
    if (MatchesAnimationState(controller.states[index], selector))
      matches.push_back(static_cast<int>(index));
  }
  return matches;
}

inline void ValidateAnimationTransitionConditions(
    const AnimationControllerAsset &controller,
    const std::vector<AnimationConditionDesc> &conditions,
    std::string_view context) {
  for (const AnimationConditionDesc &condition : conditions) {
    const auto parameter =
        std::find_if(controller.parameters.begin(), controller.parameters.end(),
                     [&](const AnimationParameterDesc &item) {
                       return item.name == condition.parameter;
                     });
    if (parameter == controller.parameters.end())
      throw std::runtime_error(std::string(context) + ": unknown parameter '" +
                               condition.parameter + "'.");
    const AnimationParameterType type = parameter->defaultValue.type;
    const bool booleanOp = condition.op == AnimationConditionOp::IfTrue ||
                           condition.op == AnimationConditionOp::IfFalse;
    const bool equalityOp = condition.op == AnimationConditionOp::Equal ||
                            condition.op == AnimationConditionOp::NotEqual;
    const bool numericOp = equalityOp ||
                           condition.op == AnimationConditionOp::Greater ||
                           condition.op == AnimationConditionOp::GreaterEqual ||
                           condition.op == AnimationConditionOp::Less ||
                           condition.op == AnimationConditionOp::LessEqual;
    if (type == AnimationParameterType::Trigger &&
        condition.op != AnimationConditionOp::Triggered)
      throw std::runtime_error(std::string(context) + ": trigger parameter '" +
                               condition.parameter +
                               "' requires op 'triggered'.");
    if (type != AnimationParameterType::Trigger &&
        condition.op == AnimationConditionOp::Triggered)
      throw std::runtime_error(std::string(context) + ": parameter '" +
                               condition.parameter + "' is not a trigger.");
    if (type == AnimationParameterType::Bool && !booleanOp && !equalityOp)
      throw std::runtime_error(std::string(context) + ": bool parameter '" +
                               condition.parameter +
                               "' uses an incompatible operator.");
    if ((type == AnimationParameterType::Int ||
         type == AnimationParameterType::Float) &&
        !numericOp)
      throw std::runtime_error(std::string(context) + ": numeric parameter '" +
                               condition.parameter +
                               "' requires a comparison operator.");
    if (condition.value.type != type &&
        condition.op != AnimationConditionOp::Triggered && !booleanOp)
      throw std::runtime_error(std::string(context) +
                               ": value type does not match parameter '" +
                               condition.parameter + "'.");
  }
}

inline std::vector<EffectiveAnimationTransition>
BuildEffectiveAnimationTransitions(const AnimationControllerAsset &controller) {
  if (controller.states.empty())
    throw std::runtime_error("Animation controller '" + controller.id +
                             "': states must not be empty.");
  std::vector<std::string> stateNames;
  for (const AnimationStateDesc &state : controller.states) {
    if (state.name.empty())
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': state name must not be empty.");
    if (detail::ContainsString(stateNames, state.name))
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': duplicate state name '" + state.name + "'.");
    stateNames.push_back(state.name);
    ValidateAnimationStateContentMode(controller, state);
    if (state.motionId.empty() && state.clipName.empty() &&
        state.clipSourceAssetId.empty() && state.blend1D.empty() &&
        state.blend2D.empty())
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': state '" + state.name +
                               "' has no animation content.");
  }
  const auto stateExists = [&](std::string_view name) {
    return std::any_of(
        controller.states.begin(), controller.states.end(),
        [&](const AnimationStateDesc &state) { return state.name == name; });
  };
  if (!controller.defaultState.empty() &&
      !stateExists(controller.defaultState)) {
    throw std::runtime_error("Animation controller '" + controller.id +
                             "': unknown default state '" +
                             controller.defaultState + "'.");
  }

  std::vector<EffectiveAnimationTransition> result;
  auto append = [&](EffectiveAnimationTransition candidate) {
    const auto duplicate =
        std::find_if(result.begin(), result.end(),
                     [&](const EffectiveAnimationTransition &existing) {
                       return detail::EquivalentTransitions(
                           existing.transition, candidate.transition);
                     });
    if (duplicate != result.end()) {
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': duplicate effective transition " +
                               candidate.transition.fromState + " -> " +
                               candidate.transition.toState + " produced by " +
                               detail::TransitionOriginLabel(*duplicate) +
                               " and " +
                               detail::TransitionOriginLabel(candidate) + ".");
    }
    result.push_back(std::move(candidate));
  };

  for (std::size_t authoredIndex = 0;
       authoredIndex < controller.transitions.size(); ++authoredIndex) {
    const AnimationTransitionDesc &authored =
        controller.transitions[authoredIndex];
    ValidateAnimationTransitionConditions(
        controller, authored.conditions,
        "Animation controller '" + controller.id + "' transition " +
            authored.fromState + " -> " + authored.toState);
    if (!stateExists(authored.toState))
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': unknown transition target '" +
                               authored.toState + "'.");
    const bool wildcard =
        authored.fromState.empty() || authored.fromState == "*";
    if (!wildcard && !stateExists(authored.fromState))
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': unknown transition source '" +
                               authored.fromState + "'.");
    if (!wildcard) {
      append(EffectiveAnimationTransition{
          .transition = authored,
          .origin = AnimationTransitionOrigin::Explicit,
          .authoredTransitionIndex = authoredIndex});
      continue;
    }
    for (const AnimationStateDesc &source : controller.states) {
      // Runtime deliberately ignores same-state transitions, so they are not
      // part of effective topology.
      if (source.name == authored.toState)
        continue;
      AnimationTransitionDesc concrete = authored;
      concrete.fromState = source.name;
      append(EffectiveAnimationTransition{
          .transition = std::move(concrete),
          .origin = AnimationTransitionOrigin::ExplicitWildcard,
          .authoredTransitionIndex = authoredIndex});
    }
  }

  std::vector<std::string> ruleIds;
  for (std::size_t ruleIndex = 0; ruleIndex < controller.transitionRules.size();
       ++ruleIndex) {
    const AnimationTransitionRuleDesc &rule =
        controller.transitionRules[ruleIndex];
    if (rule.id.empty())
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': transition rule id must not be empty.");
    if (detail::ContainsString(ruleIds, rule.id))
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': duplicate transition rule id '" + rule.id +
                               "'.");
    ruleIds.push_back(rule.id);
    ValidateAnimationTransitionConditions(
        controller, rule.conditions,
        "Animation controller '" + controller.id + "' rule '" + rule.id + "'");
    for (const std::string &name : rule.from.states)
      if (!stateExists(name))
        throw std::runtime_error("Animation controller '" + controller.id +
                                 "': rule '" + rule.id +
                                 "' has unknown source state '" + name + "'.");
    for (const std::string &name : rule.to.states)
      if (!stateExists(name))
        throw std::runtime_error(
            "Animation controller '" + controller.id + "': rule '" + rule.id +
            "' has unknown destination state '" + name + "'.");

    const std::vector<int> sources =
        ResolveAnimationStateSelector(controller, rule.from);
    const std::vector<int> destinations =
        ResolveAnimationStateSelector(controller, rule.to);
    if (sources.empty())
      throw std::runtime_error("Animation controller '" + controller.id +
                               "': rule '" + rule.id +
                               "' source selector matched no states.");
    if (destinations.size() != 1) {
      std::string matchedNames;
      for (int destination : destinations) {
        if (!matchedNames.empty())
          matchedNames += ", ";
        matchedNames +=
            controller.states[static_cast<std::size_t>(destination)].name;
      }
      throw std::runtime_error(
          "Animation controller '" + controller.id + "': rule '" + rule.id +
          "' destination selector matched " +
          std::to_string(destinations.size()) + " states (" +
          (matchedNames.empty() ? std::string("none") : matchedNames) +
          "); exactly one is required.");
    }
    const std::string &target =
        controller.states[static_cast<std::size_t>(destinations.front())].name;
    for (int sourceIndex : sources) {
      const std::string &source =
          controller.states[static_cast<std::size_t>(sourceIndex)].name;
      if (source == target)
        throw std::runtime_error(
            "Animation controller '" + controller.id + "': rule '" + rule.id +
            "' generates unsupported self-transition '" + source + "'.");
      append(EffectiveAnimationTransition{
          .transition =
              AnimationTransitionDesc{
                  source, target, rule.hasExitTime, rule.exitTimeNormalized,
                  rule.blendDurationSeconds, rule.priority, rule.conditions},
          .origin = AnimationTransitionOrigin::Rule,
          .sourceRuleId = rule.id,
          .authoredRuleIndex = ruleIndex});
    }
  }
  return result;
}

inline void RenameAnimationControllerState(AnimationControllerAsset &controller,
                                           std::string_view oldName,
                                           std::string newName) {
  if (newName.empty())
    throw std::runtime_error(
        "Animation controller state rename: new name must not be empty.");
  if (oldName != newName &&
      std::any_of(controller.states.begin(), controller.states.end(),
                  [&](const auto &state) { return state.name == newName; }))
    throw std::runtime_error("Animation controller state rename: state '" +
                             newName + "' already exists.");
  auto state =
      std::find_if(controller.states.begin(), controller.states.end(),
                   [&](const auto &item) { return item.name == oldName; });
  if (state == controller.states.end())
    throw std::runtime_error(
        "Animation controller state rename: source state '" +
        std::string(oldName) + "' does not exist.");
  state->name = newName;
  if (controller.defaultState == oldName)
    controller.defaultState = newName;
  for (AnimationTransitionDesc &transition : controller.transitions) {
    if (transition.fromState == oldName)
      transition.fromState = newName;
    if (transition.toState == oldName)
      transition.toState = newName;
  }
  for (AnimationTransitionRuleDesc &rule : controller.transitionRules) {
    for (AnimationStateSelector *selector : {&rule.from, &rule.to})
      for (std::string &exactName : selector->states)
        if (exactName == oldName)
          exactName = newName;
  }
}

inline std::vector<std::string> FindAnimationControllerStateReferences(
    const AnimationControllerAsset &controller, std::string_view stateName) {
  std::vector<std::string> references;
  if (controller.defaultState == stateName)
    references.push_back("defaultState");
  for (std::size_t index = 0; index < controller.transitions.size(); ++index) {
    const AnimationTransitionDesc &transition = controller.transitions[index];
    if (transition.fromState == stateName || transition.toState == stateName)
      references.push_back("transition[" + std::to_string(index) + "] " +
                           transition.fromState + " -> " + transition.toState);
  }
  for (const AnimationTransitionRuleDesc &rule : controller.transitionRules) {
    if (detail::ContainsString(rule.from.states, stateName))
      references.push_back("rule '" + rule.id + "' from.states");
    if (detail::ContainsString(rule.to.states, stateName))
      references.push_back("rule '" + rule.id + "' to.states");
  }
  return references;
}

inline void DeleteAnimationControllerState(AnimationControllerAsset &controller,
                                           std::string_view stateName) {
  const std::vector<std::string> references =
      FindAnimationControllerStateReferences(controller, stateName);
  if (!references.empty())
    throw std::runtime_error("Animation controller state delete: state '" +
                             std::string(stateName) + "' is referenced by " +
                             references.front() + ".");
  const auto state =
      std::find_if(controller.states.begin(), controller.states.end(),
                   [&](const auto &item) { return item.name == stateName; });
  if (state == controller.states.end())
    throw std::runtime_error("Animation controller state delete: state '" +
                             std::string(stateName) + "' does not exist.");
  controller.states.erase(state);
}