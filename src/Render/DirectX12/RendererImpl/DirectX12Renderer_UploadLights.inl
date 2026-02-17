			std::vector<GPULight> gpu;
			gpu.reserve(std::min<std::size_t>(scene.lights.size(), kMaxLights));

			for (const auto& light : scene.lights)
			{
				if (gpu.size() >= kMaxLights)
				{
					break;
				}

				GPULight gpuLight{};

				gpuLight.p0 = { light.position.x, light.position.y, light.position.z, static_cast<float>(static_cast<std::uint32_t>(light.type)) };
				gpuLight.p1 = { light.direction.x, light.direction.y, light.direction.z, light.intensity };
				gpuLight.p2 = { light.color.x, light.color.y, light.color.z, light.range };

				const float cosOuter = std::cos(mathUtils::DegToRad(light.outerHalfAngleDeg));
				const float cosInner = std::cos(mathUtils::DegToRad(light.innerHalfAngleDeg));

				gpuLight.p3 = { cosInner, cosOuter, light.attLinear, light.attQuadratic };
				gpu.push_back(gpuLight);
			}

			// Small default rig if the scene didn't provide any lights
			if (gpu.empty())
			{
				GPULight dir{};
				const mathUtils::Vec3 dirFromLight = mathUtils::Normalize(mathUtils::Vec3(-0.4f, -1.0f, -0.3f));
				dir.p0 = { 0,0,0, static_cast<float>(static_cast<std::uint32_t>(LightType::Directional)) };
				dir.p1 = { dirFromLight.x, dirFromLight.y, dirFromLight.z, 1.2f };
				dir.p2 = { 1.0f, 1.0f, 1.0f, 0.0f };
				dir.p3 = { 0,0,0,0 };
				gpu.push_back(dir);

				GPULight point{};
				point.p0 = { 2.5f, 2.0f, 1.5f, static_cast<float>(static_cast<std::uint32_t>(LightType::Point)) };
				point.p1 = { 0,0,0, 2.0f };
				point.p2 = { 1.0f, 0.95f, 0.8f, 12.0f };
				point.p3 = { 0,0, 0.12f, 0.04f };
				gpu.push_back(point);

				GPULight spot{};
				const mathUtils::Vec3 spotPos = camPos;
				const mathUtils::Vec3 spotDir = mathUtils::Normalize(mathUtils::Vec3(0, 0, 0) - camPos);
				spot.p0 = { spotPos.x, spotPos.y, spotPos.z, static_cast<float>(static_cast<std::uint32_t>(LightType::Spot)) };
				spot.p1 = { spotDir.x, spotDir.y, spotDir.z, 3.0f };
				spot.p2 = { 0.8f, 0.9f, 1.0f, 30.0f };
				spot.p3 = { std::cos(mathUtils::DegToRad(12.0f)), std::cos(mathUtils::DegToRad(20.0f)), 0.09f, 0.032f };
				gpu.push_back(spot);
			}

			device_.UpdateBuffer(lightsBuffer_, std::as_bytes(std::span{ gpu }));
			return static_cast<std::uint32_t>(gpu.size());
