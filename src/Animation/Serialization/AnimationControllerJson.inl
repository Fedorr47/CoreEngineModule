namespace {
[[nodiscard]] std::string
SerializeAnimationController_(const AnimationControllerAsset &controller) {
  if (controller.id.empty())
    throw std::runtime_error(
        "Animation controller JSON: controller id must not be empty");
  (void)BuildEffectiveAnimationTransitions(controller);
  std::ostringstream stream;
  stream << "{\n  \"defaultState\": ";
  WriteJsonEscaped(stream, controller.defaultState);
  if (!controller.notifyAssetPath.empty()) {
    stream << ",\n  \"notifyAsset\": ";
    WriteJsonEscaped(stream, controller.notifyAssetPath);
  }
  if (!controller.eventBindingsAssetPath.empty()) {
    stream << ",\n  \"eventBindingsAsset\": ";
    WriteJsonEscaped(stream, controller.eventBindingsAssetPath);
  }
  stream << ",\n  \"parameters\": {";
  for (std::size_t index = 0; index < controller.parameters.size(); ++index) {
    const AnimationParameterDesc &parameter = controller.parameters[index];
    if (parameter.name.empty())
      throw std::runtime_error(
          "Animation controller JSON: parameter name must not be empty");
    stream << (index == 0 ? "\n    " : ",\n    ");
    WriteJsonEscaped(stream, parameter.name);
    stream << ": {\"type\": ";
    WriteJsonEscaped(stream, AnimationParameterTypeToJsonString_(
                                 parameter.defaultValue.type));
    if (parameter.defaultValue.type != AnimationParameterType::Trigger) {
      stream << ", \"default\": ";
      WriteAnimationParameterLiteral_(stream, parameter.defaultValue);
    }
    stream << "}";
  }
  if (!controller.parameters.empty())
    stream << "\n  ";
  stream << "},\n  \"states\": {";
  for (std::size_t index = 0; index < controller.states.size(); ++index) {
    const AnimationStateDesc &state = controller.states[index];
    ValidateAnimationStateContentMode(controller, state);
    if (state.name.empty())
      throw std::runtime_error(
          "Animation controller JSON: state name must not be empty");
    if (state.motionId.empty() && state.clipName.empty() &&
        state.clipSourceAssetId.empty() && state.blend1D.empty() &&
        state.blend2D.empty())
      throw std::runtime_error("Animation controller JSON: state '" +
                               state.name + "' has no animation content");
    stream << (index == 0 ? "\n    " : ",\n    ");
    WriteJsonEscaped(stream, state.name);
    stream << ": {";
    if (!state.motionId.empty()) {
      stream << "\"motion\": ";
      WriteJsonEscaped(stream, state.motionId.value);
    } else if (!state.blend2D.empty()) {
      stream << "\"blend2D\": {\"parameterX\": ";
      WriteJsonEscaped(stream, state.blendParameterX);
      stream << ", \"parameterY\": ";
      WriteJsonEscaped(stream, state.blendParameterY);
      stream << ", \"points\": [";
      for (std::size_t pointIndex = 0; pointIndex < state.blend2D.size();
           ++pointIndex) {
        const auto &point = state.blend2D[pointIndex];
        if (pointIndex)
          stream << ", ";
        stream << "{\"clip\": ";
        WriteJsonEscaped(stream, point.clipName);
        if (!point.clipSourceAssetId.empty()) {
          stream << ", \"clipSourceAssetId\": ";
          WriteJsonEscaped(stream, point.clipSourceAssetId);
        }
        stream << ", \"x\": " << point.x << ", \"y\": " << point.y << "}";
      }
      stream << "]}";
    } else if (!state.blend1D.empty()) {
      stream << "\"blend1D\": {\"parameter\": ";
      WriteJsonEscaped(stream, state.blendParameter);
      stream << ", \"points\": [";
      for (std::size_t pointIndex = 0; pointIndex < state.blend1D.size();
           ++pointIndex) {
        const auto &point = state.blend1D[pointIndex];
        if (pointIndex)
          stream << ", ";
        stream << "{\"clip\": ";
        WriteJsonEscaped(stream, point.clipName);
        stream << ", \"value\": " << point.value << "}";
      }
      stream << "]}";
    } else {
      stream << "\"clip\": ";
      WriteJsonEscaped(stream, state.clipName);
    }
    if (!state.clipSourceAssetId.empty() && state.blend2D.empty()) {
      stream << ", \"clipSourceAssetId\": ";
      WriteJsonEscaped(stream, state.clipSourceAssetId);
    }
    if (!state.tags.empty()) {
      stream << ", \"tags\": ";
      WriteAnimationStringArray_(stream, state.tags);
    }
    if (!state.looping)
      stream << ", \"loop\": false";
    if (std::fabs(state.playRate - 1.0f) > 1e-6f)
      stream << ", \"playRate\": " << state.playRate;
    if (controller.notifyAssetPath.empty() && !state.notifies.empty()) {
      stream << ", \"notifies\": [";
      for (std::size_t n = 0; n < state.notifies.size(); ++n) {
        if (n)
          stream << ", ";
        const auto &notify = state.notifies[n];
        stream << "{\"id\": ";
        WriteJsonEscaped(stream, notify.id);
        stream << ", \"time\": " << notify.timeNormalized;
        if (notify.fireOnEnter)
          stream << ", \"fireOnEnter\": true";
        stream << "}";
      }
      stream << "]";
    }
    stream << "}";
  }
  if (!controller.states.empty())
    stream << "\n  ";
  stream << "},\n  \"transitions\": [";
  for (std::size_t index = 0; index < controller.transitions.size(); ++index) {
    stream << (index == 0 ? "\n    " : ",\n    ");
    WriteAnimationTransition_(stream, controller.transitions[index]);
  }
  if (!controller.transitions.empty())
    stream << "\n  ";
  stream << "],\n  \"transitionRules\": [";
  for (std::size_t index = 0; index < controller.transitionRules.size();
       ++index) {
    stream << (index == 0 ? "\n    " : ",\n    ");
    WriteAnimationTransitionRule_(stream, controller.transitionRules[index]);
  }
  if (!controller.transitionRules.empty())
    stream << "\n  ";
  stream << "]";
  if (controller.eventBindingsAssetPath.empty() &&
      !controller.eventBindings.empty()) {
    stream << ",\n  \"eventBindings\": [";
    for (std::size_t i = 0; i < controller.eventBindings.size(); ++i) {
      if (i)
        stream << ", ";
      stream << "{\"animationEvent\": ";
      WriteJsonEscaped(stream, controller.eventBindings[i].animationEventId);
      stream << ", \"gameplayEvent\": ";
      WriteJsonEscaped(stream, controller.eventBindings[i].gameplayEventId);
      stream << "}";
    }
    stream << "]";
  }
  stream << "\n}\n";
  return stream.str();
}
} // namespace

AnimationControllerAsset
LoadAnimationControllerAssetFromJson(std::string_view path,
                                     std::string_view id) {
  return LoadExternalAnimationControllerAssetFromJson_(path, id);
}

void SaveAnimationControllerAssetToJson(
    std::string_view path, const AnimationControllerAsset &controller) {
  // Validation and complete serialization happen before opening the
  // destination.
  const std::string text = SerializeAnimationController_(controller);
  const std::filesystem::path absolutePath =
      corefs::ResolveAsset(std::filesystem::path(std::string(path)));
  std::filesystem::create_directories(absolutePath.parent_path());
  std::ofstream file(absolutePath, std::ios::binary | std::ios::trunc);
  if (!file)
    throw std::runtime_error(
        "Animation controller JSON: failed to open for write: " +
        absolutePath.string());
  file.write(text.data(), static_cast<std::streamsize>(text.size()));
  file.flush();
  if (!file)
    throw std::runtime_error("Animation controller JSON: failed to write: " +
                             absolutePath.string());
}