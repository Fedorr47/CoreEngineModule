		void Clear()
		{
			drawItems.clear();
			skinnedDrawItems.clear();
			lights.clear();
			particles.clear();
			particleEmitters.clear();
			skyboxDescIndex = 0;
			debugPickRay = {};
			gameplayMovementDebug = {};
			editorSelectedNode = -1;
			editorSelectedParticleEmitter = -1;
			editorSelectedNodes.clear();
			editorSelectedLight = -1;
			editorSelectedLights.clear();
			editorSelectedDrawItem = -1;
			editorSelectedSkinnedDrawItem = -1;
			editorSelectedDrawItems.clear();
			editorSelectedSkinnedDrawItems.clear();
			editorDrawSelectedSkinnedSkeleton = false;
			editorDrawSelectedSkinnedBounds = false;
			editorReflectionCaptureOwnerNode = -1;
			editorReflectionCaptureOwnerDrawItem = -1;
			editorGizmoMode = GizmoMode::Translate;
			editorTranslateSpace = GizmoSpace::World;
			editorTranslateGizmo = {};
			editorRotateGizmo = {};
			editorParticleEmitterTranslateDrag = {};
			editorScaleGizmo = {};
		}

		void EditorClearNodeSelection() noexcept
		{
			editorSelectedNode = -1;
			editorSelectedNodes.clear();
			editorSelectedDrawItem = -1;
			editorSelectedSkinnedDrawItem = -1;
			editorSelectedDrawItems.clear();
			editorSelectedSkinnedDrawItems.clear();
		}

		void EditorClearParticleEmitterSelection() noexcept
		{
			editorSelectedParticleEmitter = -1;
		}

		void EditorClearLightSelection() noexcept
		{
			editorSelectedLight = -1;
			editorSelectedLights.clear();
		}

		void EditorClearSelection() noexcept
		{
			EditorClearNodeSelection();
			EditorClearParticleEmitterSelection();
			EditorClearLightSelection();
		}

		bool EditorIsNodeSelected(int nodeIndex) const noexcept
		{
			for (const int v : editorSelectedNodes)
			{
				if (v == nodeIndex)
				{
					return true;
				}
			}
			return false;
		}

		// Sets selection to a single node (or clears when nodeIndex < 0).
		void EditorSetSelectionSingle(int nodeIndex) noexcept
		{
			EditorClearSelection();
			if (nodeIndex >= 0)
			{
				editorSelectedNode = nodeIndex;
				editorSelectedNodes.push_back(nodeIndex);
			}
		}

		bool EditorIsLightSelected(int lightIndex) const noexcept
		{
			for (const int v : editorSelectedLights)
			{
				if (v == lightIndex)
				{
					return true;
				}
			}
			return false;
		}

		void EditorSetLightSelectionSingle(int lightIndex) noexcept
		{
			EditorClearSelection();
			if (lightIndex >= 0)
			{
				editorSelectedLight = lightIndex;
				editorSelectedLights.push_back(lightIndex);
			}
		}

		void EditorSanitizeLightSelection(std::size_t lightCount) noexcept
		{
			auto isValidLight = [lightCount](int lightIndex) noexcept
				{
					return lightIndex >= 0 && static_cast<std::size_t>(lightIndex) < lightCount;
				};

			if (!isValidLight(editorSelectedLight))
			{
				editorSelectedLight = -1;
			}

			std::size_t write = 0;
			for (std::size_t i = 0; i < editorSelectedLights.size(); ++i)
			{
				const int selectedLight = editorSelectedLights[i];
				if (!isValidLight(selectedLight))
				{
					continue;
				}

				bool duplicate = false;
				for (std::size_t j = 0; j < write; ++j)
				{
					if (editorSelectedLights[j] == selectedLight)
					{
						duplicate = true;
						break;
					}
				}

				if (!duplicate)
				{
					editorSelectedLights[write++] = selectedLight;
				}
			}
			editorSelectedLights.resize(write);

			if (editorSelectedLight >= 0)
			{
				bool found = false;
				for (const int selectedLight : editorSelectedLights)
				{
					if (selectedLight == editorSelectedLight)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					editorSelectedLight = -1;
					editorSelectedLights.clear();
				}
			}
			else if (!editorSelectedLights.empty())
			{
				editorSelectedLight = editorSelectedLights.back();
			}
		}

		bool EditorIsParticleEmitterSelected(int emitterIndex) const noexcept
		{
			return editorSelectedParticleEmitter == emitterIndex;
		}

		void EditorSetSelectionSingleParticleEmitter(int emitterIndex) noexcept
		{
			EditorClearSelection();
			if (emitterIndex >= 0)
			{
				editorSelectedParticleEmitter = emitterIndex;
			}
		}


		void EditorToggleSelectionParticleEmitter(int emitterIndex) noexcept
		{
			if (emitterIndex < 0)
			{
				return;
			}

			if (editorSelectedParticleEmitter == emitterIndex)
			{
				editorSelectedParticleEmitter = -1;
			}
			else
			{
				EditorClearSelection();
				editorSelectedParticleEmitter = emitterIndex;
			}
		}

		// Toggle node in the selection set. Updates primary selection.
		void EditorToggleSelectionNode(int nodeIndex) noexcept
		{
			if (nodeIndex < 0)
			{
				return;
			}

			if (editorSelectedParticleEmitter >= 0)
			{
				EditorClearParticleEmitterSelection();
			}
			if (editorSelectedLight >= 0 || !editorSelectedLights.empty())
			{
				EditorClearLightSelection();
			}

			for (std::size_t i = 0; i < editorSelectedNodes.size(); ++i)
			{
				if (editorSelectedNodes[i] == nodeIndex)
				{
					editorSelectedNodes.erase(editorSelectedNodes.begin() + static_cast<std::vector<int>::difference_type>(i));
					if (editorSelectedNode == nodeIndex)
					{
						editorSelectedNode = editorSelectedNodes.empty() ? -1 : editorSelectedNodes.back();
					}
					return;
				}
			}

			editorSelectedNodes.push_back(nodeIndex);
			editorSelectedNode = nodeIndex;
		}

		void EditorToggleSelectionLight(int lightIndex) noexcept
		{
			if (lightIndex < 0)
			{
				return;
			}

			if (editorSelectedNode >= 0 || !editorSelectedNodes.empty())
			{
				EditorClearNodeSelection();
			}
			if (editorSelectedParticleEmitter >= 0)
			{
				EditorClearParticleEmitterSelection();
			}

			for (std::size_t i = 0; i < editorSelectedLights.size(); ++i)
			{
				if (editorSelectedLights[i] == lightIndex)
				{
					editorSelectedLights.erase(editorSelectedLights.begin() + static_cast<std::vector<int>::difference_type>(i));
					if (editorSelectedLight == lightIndex)
					{
						editorSelectedLight = editorSelectedLights.empty() ? -1 : editorSelectedLights.back();
					}
					return;
				}
			}

			editorSelectedLights.push_back(lightIndex);
			editorSelectedLight = lightIndex;
		}
