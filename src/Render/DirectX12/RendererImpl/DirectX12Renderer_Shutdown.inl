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

			if (reflectionCaptureCubeDescIndex_ != 0)
			{
				device_.FreeTextureDescriptor(reflectionCaptureCubeDescIndex_);
				reflectionCaptureCubeDescIndex_ = 0;
			}
			if (reflectionCaptureCube_)
			{
				device_.DestroyTexture(reflectionCaptureCube_);
				reflectionCaptureCube_ = {};
			}
			if (reflectionCaptureDepthCube_)
			{
				device_.DestroyTexture(reflectionCaptureDepthCube_);
				reflectionCaptureDepthCube_ = {};
			}
			//reflectionCaptureExtent_ = {};
			DestroyMesh(device_, skyboxMesh_);
			debugDrawRenderer_.Shutdown();
			psoCache_.ClearCache();
			shaderLibrary_.ClearCache();
