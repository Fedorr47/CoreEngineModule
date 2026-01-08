module;

#include <deque>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>

export module resource_manager:texture;

import :core;

export using TextureResource = Texture<GPUTexture>;

export struct TextureEntry
{
	using Handle = std::shared_ptr<TextureResource>;

	Handle textureHandle;
	ResourceState state{ ResourceState::Unloaded };
	std::optional<TextureCPUData> pendingCpu{};
	std::string error{};
};

export template <>
class ResourceStorage<TextureResource>
{
public:
	using Resource = TextureResource;
	using Handle = std::shared_ptr<Resource>;
	using Id = std::string;

	template <typename PropertiesType>
		requires std::same_as<std::remove_cvref_t<PropertiesType>, typename Resource::Properties>
	Handle LoadOrGet(std::string_view id, TextureIO& io, PropertiesType&& properties)
	{
		Id key{ id };

		if (auto it = entries_.find(key); it != entries_.end())
		{
			return it->second.textureHandle;
		}

		TextureEntry entry{};

		Handle texture = std::make_shared<Resource>(std::forward<PropertiesType>(properties));
		entry.textureHandle = texture;
		entry.state = ResourceState::Loading;

		auto Result = ResourceTraits<Resource>::Load(key, io, entry.textureHandle->GetProperties());
		entry.pendingCpu = std::move(Result.pendingCpu);
		entry.state = Result.state;
		entry.error = std::move(Result.error);

		if (entry.state == ResourceState::Loading && entry.pendingCpu.has_value())
		{
			uploadQueue_.push_back(key);
		}

		entries_.emplace(std::move(key), std::move(entry));
		return texture;
	}

	Handle Find(std::string_view id) const
	{
		Id key{ id };

		if (auto it = entries_.find(key); it != entries_.end())
		{
			return it->second.textureHandle;
		}
		return {};
	}

	void UnloadUnused()
	{
		for (auto it = entries_.begin(); it != entries_.end(); )
		{
			if (it->second.textureHandle.use_count() == 1)
			{
				if (it->second.state == ResourceState::Loaded)
				{
					EnqueueDestroy(it->second.textureHandle->GetResource());
				}
				it = entries_.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void Clear()
	{
		for (auto& [id, entry] : entries_)
		{
			if (entry.state == ResourceState::Loaded)
			{
				EnqueueDestroy(entry.textureHandle->GetResource());
			}
		}

		entries_.clear();
		uploadQueue_.clear();
	}

	ResourceState GetState(std::string_view id) const
	{
		Id key{ id };
		if (auto it = entries_.find(key); it != entries_.end())
		{
			return it->second.state;
		}

		return ResourceState::Unknown;
	}

	const std::string& GetError(std::string_view id) const
	{
		static const std::string errorStr{};
		Id key{ id };
		if (auto it = entries_.find(key); it != entries_.end())
		{
			return it->second.error;
		}
		return errorStr;
	}

	bool ProcessUploads(TextureIO& io, std::size_t maxPerCall = 8, std::size_t maxDestroyedPerCall = 32)
	{
		std::size_t done = 0;
		std::size_t destroyed = 0;

		while (!destroyQueue_.empty() && destroyed < maxDestroyedPerCall)
		{
			GPUTexture gTexture = destroyQueue_.front();
			destroyQueue_.pop_front();

			if (gTexture.id != 0)
			{
				io.uploader.Destroy(gTexture);
			}

			++destroyed;
		}

		while (!uploadQueue_.empty() && done < maxPerCall)
		{
			Id id = std::move(uploadQueue_.front());
			uploadQueue_.pop_front();

			auto it = entries_.find(id);
			if (it == entries_.end())
			{
				continue;
			}

			TextureEntry& entry = it->second;
			if (entry.state != ResourceState::Loading || !entry.pendingCpu.has_value())
			{
				continue;
			}

			std::optional<GPUTexture> gpuOpt = io.uploader.CreateAndUpload(*entry.pendingCpu, entry.textureHandle->GetProperties());
			if (!gpuOpt)
			{
				entry.state = ResourceState::Failed;
				entry.error = "GPU texture upload failed";
				entry.pendingCpu.reset();
			}
			else
			{
				entry.textureHandle->SetResource(*gpuOpt);
				entry.state = ResourceState::Loaded;
				entry.pendingCpu.reset();
			}

			++done;
		}

		return (done + destroyed) > 0;
	}

private:

	void EnqueueDestroy(GPUTexture texture)
	{
		if (texture.id != 0)
		{
			destroyQueue_.push_back(texture);
		}
	}

	std::unordered_map<Id, TextureEntry> entries_;
	std::deque<Id> uploadQueue_;
	std::deque<GPUTexture> destroyQueue_;
};

export template <>
struct ResourceTraits<TextureResource>
{
	using Resource = TextureResource;
	using Handle = std::shared_ptr<Resource>;
	using Properties = typename Resource::Properties;

	struct LoadResult
	{
		std::optional<TextureCPUData> pendingCpu{};
		ResourceState state{ ResourceState::Failed };
		std::string error{};
	};

	static LoadResult Load(std::string_view id, TextureIO& io, Properties properties)
	{
		LoadResult Result{};

		std::string_view path = properties.filePath.empty() ? id : std::string_view{ properties.filePath };

		auto cpuOpt = io.decoder.Decode(properties, path);
		if (!cpuOpt)
		{
			Result.state = ResourceState::Failed;
			Result.error = "Texture decode failed";
			return Result;
		}

		Result.pendingCpu = std::move(*cpuOpt);
		Result.state = ResourceState::Loading;
		return Result;
	}
};
