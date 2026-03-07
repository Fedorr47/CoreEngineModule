auto FillPerBatchViewLightingConstants = [&](PerBatchConstants& constants,
	const mathUtils::Mat4& viewProj,
	const mathUtils::Mat4& lightViewProj,
	const mathUtils::Vec3& camPos,
	const mathUtils::Vec3& camForward,
	float ambientW = 0.22f,
	float cameraForwardW = 0.0f)
	{
		const mathUtils::Mat4 viewProjT = mathUtils::Transpose(viewProj);
		const mathUtils::Mat4 lightViewProjT = mathUtils::Transpose(lightViewProj);
		std::memcpy(constants.uViewProj.data(), mathUtils::ValuePtr(viewProjT), sizeof(float) * 16);
		std::memcpy(constants.uLightViewProj.data(), mathUtils::ValuePtr(lightViewProjT), sizeof(float) * 16);
		constants.uCameraAmbient = { camPos.x, camPos.y, camPos.z, ambientW };
		constants.uCameraForward = { camForward.x, camForward.y, camForward.z, cameraForwardW };
	};

auto ResetPerBatchEnvProbeBox = [&](PerBatchConstants& constants, float minW = 0.0f, float maxW = 0.0f)
	{
		constants.uEnvProbeBoxMin = { 0.0f, 0.0f, 0.0f, minW };
		constants.uEnvProbeBoxMax = { 0.0f, 0.0f, 0.0f, maxW };
	};

auto ApplyPerBatchReflectionProbeBox = [&](PerBatchConstants& constants, int reflectionProbeIndex)
	{
		ResetPerBatchEnvProbeBox(constants);
		if (reflectionProbeIndex < 0 || static_cast<std::size_t>(reflectionProbeIndex) >= reflectionProbes_.size())
		{
			return;
		}

		const auto& probe = reflectionProbes_[static_cast<std::size_t>(reflectionProbeIndex)];
		const float h = settings_.reflectionProbeBoxHalfExtent;
		constants.uEnvProbeBoxMin = { probe.capturePos.x - h, probe.capturePos.y - h, probe.capturePos.z - h, 0.0f };
		constants.uEnvProbeBoxMax = { probe.capturePos.x + h, probe.capturePos.y + h, probe.capturePos.z + h, 0.0f };
	};