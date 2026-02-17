			if (instanceBuffer_)
			{
				device_.DestroyBuffer(instanceBuffer_);
			}
			if (lightsBuffer_)
			{
				device_.DestroyBuffer(lightsBuffer_);
			}
			if (shadowDataBuffer_)
			{
				device_.DestroyBuffer(shadowDataBuffer_);
			}
			DestroyMesh(device_, skyboxMesh_);
			debugDrawRenderer_.Shutdown();
			psoCache_.ClearCache();
			shaderLibrary_.ClearCache();
