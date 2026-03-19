#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

import core;

struct FakeTextureDecoder final : ITextureDecoder
{
	bool succeed{ true };
	std::uint32_t nextWidth{ 4 };
	std::uint32_t nextHeight{ 4 };

	std::optional<TextureCPUData> Decode(
		const TextureProperties& properties,
		std::string_view) override
	{
		if (!succeed)
		{
			return std::nullopt;
		}

		const int channel =
			(properties.format == TextureFormat::GRAYSCALE) ? 1 :
			(properties.format == TextureFormat::RGB) ? 3 : 4;

		TextureCPUData cpuData{};
		cpuData.width = nextWidth;
		cpuData.height = nextHeight;
		cpuData.channels = channel;
		cpuData.format = properties.format;

		const std::size_t bytes =
			static_cast<std::size_t>(cpuData.width)
			* static_cast<std::size_t>(cpuData.height)
			* static_cast<std::size_t>(cpuData.channels);

		TextureMipLevel baseMip{};
		baseMip.width = cpuData.width;
		baseMip.height = cpuData.height;
		baseMip.pixels.resize(bytes, 0xAB);
		cpuData.mips.push_back(std::move(baseMip));

		return cpuData;
	}
};

struct FakeTextureUploader final : ITextureUploader
{
	bool succeed{ true };
	std::uint32_t nextId{ 1 };
	bool batchActive{ false };

	std::vector<std::uint32_t> createdIds{};
	std::vector<std::uint32_t> destroyedIds{};

	void BeginUploadBatch() override
	{
		batchActive = true;
	}

	void EndUploadBatch() override
	{
		batchActive = false;
	}

	std::optional<GPUTexture> CreateAndUpload(
		const TextureCPUData& cpuData,
		const TextureProperties&) override
	{
		if (!succeed)
		{
			return std::nullopt;
		}

		if (cpuData.width == 0 || cpuData.height == 0 || cpuData.mips.empty() || cpuData.mips[0].pixels.empty())
		{
			return std::nullopt;
		}

		GPUTexture texture{};
		texture.id = nextId++;
		createdIds.push_back(texture.id);
		return texture;
	}

	void Destroy(const GPUTexture texture) noexcept override
	{
		if (texture.id != 0)
		{
			destroyedIds.push_back(texture.id);
		}
	}
};

struct FakeJobSystem final : IJobSystem
{
	std::deque<std::function<void()>> jobs{};

	void Enqueue(std::function<void()> job) override
	{
		jobs.push_back(std::move(job));
	}

	void WaitIdle() override
	{
		Drain();
	}

	void Drain()
	{
		while (!jobs.empty())
		{
			auto job = std::move(jobs.front());
			jobs.pop_front();
			job();
		}
	}
};

struct FakeRenderQueue final : IRenderQueue
{
	std::deque<std::function<void()>> jobs{};

	void Enqueue(std::function<void()> job) override
	{
		jobs.push_back(std::move(job));
	}

	void Drain()
	{
		while (!jobs.empty())
		{
			auto job = std::move(jobs.front());
			jobs.pop_front();
			job();
		}
	}
};

inline TextureIO MakeIO(
	FakeTextureDecoder& decoder,
	FakeTextureUploader& uploader,
	FakeJobSystem& jobSystem,
	FakeRenderQueue& renderQueue)
{
	return TextureIO{ decoder, uploader, jobSystem, renderQueue };
}
