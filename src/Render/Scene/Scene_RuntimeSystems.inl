		MaterialHandle CreateMaterial(const Material& m)
		{
			materials.push_back(m);
			return MaterialHandle{ static_cast<std::uint32_t>(materials.size()) }; // id is 1-based
		}

		const Material& GetMaterial(MaterialHandle h) const
		{
			if (h.id == 0 || h.id > materials.size())
			{
				throw std::runtime_error("Scene::GetMaterial: invalid MaterialHandle");
			}
			return materials[h.id - 1];
		}

		Material& GetMaterial(MaterialHandle h)
		{
			if (h.id == 0 || h.id > materials.size())
			{
				throw std::runtime_error("Scene::GetMaterial: invalid MaterialHandle");
			}
			return materials[h.id - 1];
		}

		DrawItem& AddDraw(const DrawItem& item)
		{
			drawItems.push_back(item);
			return drawItems.back();
		}

		SkinnedDrawItem& AddSkinnedDraw(SkinnedDrawItem item)
		{
			skinnedDrawItems.push_back(std::move(item));
			return skinnedDrawItems.back();
		}

		SkinnedDrawItem& AddSkinnedDraw(
			const SkinnedHandle& asset,
			const Transform& transform,
			MaterialHandle material,
			int clipIndex = -1,
			bool autoplay = true,
			bool loop = true,
			float playRate = 1.0f)
		{
			SkinnedDrawItem item{};
			item.asset = asset;
			item.transform = transform;
			item.material = material;
			item.autoplay = autoplay;
			item.activeClipIndex = clipIndex;

			if (asset)
			{
				InitializeAnimator(item.animator, &asset->mesh.skeleton, nullptr);

				if (autoplay)
				{
					const int resolvedClip =
						(clipIndex >= 0) ? clipIndex : (asset->clips.empty() ? -1 : 0);

					item.activeClipIndex = resolvedClip;
					SetAnimatorClip(item.animator, asset->mesh.skeleton, asset->clips, resolvedClip, loop, playRate);
					EvaluateAnimator(item.animator);
				}
				else
				{
					item.animator.looping = loop;
					item.animator.playRate = playRate;
					item.animator.paused = true;
					EvaluateAnimator(item.animator);
				}

				SyncAnimationControllerLegacyClip(
					item.controller,
					asset->mesh.skeleton,
					asset->clips,
					item.activeClipIndex,
					item.autoplay,
					item.animator.looping,
					item.animator.playRate,
					item.animator.paused,
					item.debugForceBindPose);
			}

			skinnedDrawItems.push_back(std::move(item));
			return skinnedDrawItems.back();
		}

		void UpdateSkinned(float dt)
		{
			for (SkinnedDrawItem& item : skinnedDrawItems)
			{
				if (!item.asset)
				{
					continue;
				}

				if (IsAnimationControllerUsingLegacyClipMode(item.controller) || item.controller.stateMachineAsset == nullptr)
				{
					SyncAnimationControllerLegacyClip(
						item.controller,
						item.asset->mesh.skeleton,
						item.asset->clips,
						item.activeClipIndex,
						item.autoplay,
						item.animator.looping,
						item.animator.playRate,
						item.animator.paused,
						item.debugForceBindPose);
				}
				else
				{
					RefreshAnimationControllerRuntimeBindings(
						item.controller,
						item.asset->mesh.skeleton,
						item.asset->clips,
						item.asset->clipSourceAssetIds,
						item.autoplay,
						item.animator.paused,
						item.debugForceBindPose);
				}

				UpdateAnimationControllerRuntime(item.controller, item.animator, dt);
			}
		}

		const std::vector<SkinnedDrawItem>& GetSkinnedDrawItems() const noexcept
		{
			return skinnedDrawItems;
		}

		Light& AddLight(const Light& l)
		{
			lights.push_back(l);
			return lights.back();
		}

		Particle& AddParticle(const Particle& particle)
		{
			particles.push_back(particle);
			return particles.back();
		}

		ParticleEmitter& AddParticleEmitter(const ParticleEmitter& emitter)
		{
			ParticleEmitter runtime = emitter;
			runtime.elapsed = 0.0f;
			runtime.spawnAccumulator = 0.0f;
			runtime.spawnSequence = 0u;
			runtime.burstDone = false;
			particleEmitters.push_back(runtime);
			return particleEmitters.back();
		}

		void EmitParticleFromEmitter(ParticleEmitter& emitter, int emitterIndex)
		{
			if (emitter.maxParticles > 0u)
			{
				std::uint32_t aliveOwned = 0u;
				for (const Particle& existing : particles)
				{
					if (existing.alive && existing.ownerEmitter == emitterIndex)
					{
						++aliveOwned;
					}
				}
				if (aliveOwned >= emitter.maxParticles)
				{
					return;
				}
			}

			std::uint32_t rng = (emitter.spawnSequence++ + 1u) * 747796405u + 2891336453u;

			Particle particle{};
			particle.position =
				emitter.position +
				mathUtils::Vec3(
					detail::ParticleRandRange(rng, -emitter.positionJitter.x, emitter.positionJitter.x),
					detail::ParticleRandRange(rng, -emitter.positionJitter.y, emitter.positionJitter.y),
					detail::ParticleRandRange(rng, -emitter.positionJitter.z, emitter.positionJitter.z));

			particle.velocity = mathUtils::Vec3(
				detail::ParticleRandRange(rng, emitter.velocityMin.x, emitter.velocityMax.x),
				detail::ParticleRandRange(rng, emitter.velocityMin.y, emitter.velocityMax.y),
				detail::ParticleRandRange(rng, emitter.velocityMin.z, emitter.velocityMax.z));

			particle.colorBegin = emitter.colorBegin;
			particle.colorEnd = emitter.colorEnd;
			particle.color = particle.colorBegin;
			const float randomizedSizeBegin = detail::ParticleRandRange(rng, emitter.sizeMin, emitter.sizeMax);
			particle.sizeBegin = (emitter.sizeBegin > 0.0f) ? emitter.sizeBegin : randomizedSizeBegin;
			particle.sizeEnd = (emitter.sizeEnd > 0.0f) ? emitter.sizeEnd : particle.sizeBegin;
			particle.size = particle.sizeBegin;
			particle.lifetime = detail::ParticleRandRange(rng, emitter.lifetimeMin, emitter.lifetimeMax);
			particle.age = 0.0f;
			particle.rotationRad = detail::ParticleRandRange(rng, 0.0f, 6.28318530718f);
			particle.alive = true;
			particle.ownerEmitter = emitterIndex;

			particles.push_back(particle);
		}
		void UpdateParticles(float dt)
		{
			for (std::size_t emitterIndex = 0; emitterIndex < particleEmitters.size(); ++emitterIndex)
			{
				ParticleEmitter& emitter = particleEmitters[emitterIndex];
				if (!emitter.enabled)
				{
					continue;
				}

				const float previousElapsed = emitter.elapsed;
				emitter.elapsed += dt;

				if (!emitter.burstDone && emitter.burstCount > 0u && previousElapsed <= emitter.startDelay && emitter.elapsed >= emitter.startDelay)
				{
					for (std::uint32_t i = 0; i < emitter.burstCount; ++i)
					{
						EmitParticleFromEmitter(emitter, static_cast<int>(emitterIndex));
					}
					emitter.burstDone = true;
				}

				if (emitter.elapsed < emitter.startDelay)
				{
					continue;
				}

				if (!emitter.looping && emitter.duration > 0.0f && (emitter.elapsed - emitter.startDelay) > emitter.duration)
				{
					continue;
				}

				if (emitter.spawnRate > 0.0f)
				{
					emitter.spawnAccumulator += dt * emitter.spawnRate;
					while (emitter.spawnAccumulator >= 1.0f)
					{
						emitter.spawnAccumulator -= 1.0f;
						EmitParticleFromEmitter(emitter, static_cast<int>(emitterIndex));
					}
				}
			}

			for (Particle& particle : particles)
			{
				if (!particle.alive)
				{
					continue;
				}

				particle.age += dt;
				particle.position = particle.position + particle.velocity * dt;
				const float lifeT = (particle.lifetime > 0.0f) ? std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f) : 1.0f;
				particle.color = mathUtils::Lerp(particle.colorBegin, particle.colorEnd, lifeT);
				particle.size = mathUtils::Lerp(particle.sizeBegin, particle.sizeEnd, lifeT);

				if (particle.lifetime > 0.0f && particle.age >= particle.lifetime)
				{
					particle.alive = false;
				}
			}

			particles.erase(
				std::remove_if(particles.begin(), particles.end(), [](const Particle& particle)
					{
						return !particle.alive || particle.size <= 0.0f || particle.color.w <= 0.0f;
					}),
				particles.end());
		}

		std::span<const Material> GetMaterials() const { return materials; }
		std::span<Material> GetMaterials() { return materials; }

		std::span<const DrawItem> GetDrawItems() const { return drawItems; }
		std::span<DrawItem> GetDrawItems() { return drawItems; }

		std::span<const Light> GetLights() const { return lights; }
		std::span<Light> GetLights() { return lights; }

		std::span<const Particle> GetParticles() const { return particles; }
		std::span<Particle> GetParticles() { return particles; }

		void RestartParticleEmitter(int emitterIndex)
		{
			if (emitterIndex < 0 || static_cast<std::size_t>(emitterIndex) >= particleEmitters.size())
			{
				return;
			}

			particles.erase(
				std::remove_if(particles.begin(), particles.end(), [emitterIndex](const Particle& particle)
					{
						return particle.ownerEmitter == emitterIndex;
					}),
				particles.end());

			ParticleEmitter& emitter = particleEmitters[static_cast<std::size_t>(emitterIndex)];
			emitter.elapsed = 0.0f;
			emitter.spawnAccumulator = 0.0f;
			emitter.spawnSequence = 0u;
			emitter.burstDone = false;
		}

		void RestartAllParticleEmitters()
		{
			particles.clear();
			for (ParticleEmitter& emitter : particleEmitters)
			{
				emitter.elapsed = 0.0f;
				emitter.spawnAccumulator = 0.0f;
				emitter.spawnSequence = 0u;
				emitter.burstDone = false;
			}
		}
		std::span<const ParticleEmitter> GetParticleEmitters() const { return particleEmitters; }
		std::span<ParticleEmitter> GetParticleEmitters() { return particleEmitters; }