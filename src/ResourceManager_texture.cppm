module;

#include <deque>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>

export module resource_manager:texture;

import :core;

constexpr int SyncLoadNumberPerCall = 64;

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
		return LoadAsync(id, io, std::forward<PropertiesType>(properties));
	}

	template <typename PropertiesType>
		requires std::same_as<std::remove_cvref_t<PropertiesType>, typename Resource::Properties>
	Handle LoadAsync(std::string_view id, TextureIO& io, PropertiesType&& properties)
	{
		Id key{ id };
		Handle handle{};

		{
			std::scoped_lock lock(mutex_);
			if (auto it = entries_.find(key); it != entries_.end())
			{
				return it->second.textureHandle;
			}

			TextureEntry entry{};
			entry.textureHandle = std::make_shared<Resource>(std::forward<PropertiesType>(properties));
			entry.state = ResourceState::Loading;

			handle = entry.textureHandle;
			entries_.emplace(std::move(key), std::move(entry));
		}

		TextureProperties propertiesCopy = handle->GetProperties();
		std::string path = propertiesCopy.filePath.empty() ? std::string(key) : propertiesCopy.filePath;

		io.jobs.Enqueue([this, key, propertiesCopy = std::move(propertiesCopy), path = std::move(path), &io]() mutable
			{
				auto cpuOpt = io.decoder.Decode(propertiesCopy, path);

				std::scoped_lock lock(mutex_);

				auto it = entries_.find(key);
				if (it == entries_.end())
				{
					return;
				}

				TextureEntry& entry = it->second;

				if (entry.state != ResourceState::Loading)
				{
					return;
				}

				if (!cpuOpt)
				{
					entry.state = ResourceState::Failed;
					entry.error = "Texture decode failed";
					entry.pendingCpu.reset();
					return;
				}

				entry.pendingCpu = std::move(*cpuOpt);
				uploadQueue_.push_back(key);
			});
		
		return handle;
	}

	template <typename PropertiesType>
		requires std::same_as<std::remove_cvref_t<PropertiesType>, typename Resource::Properties>
	Handle LoadSync(std::string_view id, TextureIO& io, PropertiesType&& properties)
	{
		auto handle = LoadAsync(id, io, std::forward<PropertiesType>(properties));

		for (;;)
		{
			auto state = GetState(id);

			if (state == ResourceState::Loaded || state == ResourceState::Failed)
			{
				return handle;
			}

			io.jobs.WaitIdle();

			ProcessUploads(io, SyncLoadNumberPerCall, SyncLoadNumberPerCall);
		}
	}

	Handle Find(std::string_view id) const
	{
		Id key{ id };
		std::scoped_lock lock(mutex_);

		if (auto it = entries_.find(key); it != entries_.end())
		{
			return it->second.textureHandle;
		}
		return {};
	}

	void UnloadUnused()
	{
		std::scoped_lock lock(mutex_);

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
		std::scoped_lock lock(mutex_);

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
		std::scoped_lock lock(mutex_);

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
		std::scoped_lock lock(mutex_);

		if (auto it = entries_.find(key); it != entries_.end())
		{
			return it->second.error;
		}
		return errorStr;
	}

	bool ProcessUploads(TextureIO& io, std::size_t maxPerCall = 8, std::size_t maxDestroyedPerCall = 32)
	{
		std::size_t uploaded = 0;
		std::size_t destroyed = 0;

		while (destroyed < maxDestroyedPerCall)
		{
			GPUTexture gTexture{};
			{
				std::scoped_lock lock(mutex_);
				if (destroyQueue_.empty())
				{
					break;
				}
				gTexture = destroyQueue_.front();
				destroyQueue_.pop_front();
			}

			if (gTexture.id != 0)
			{
				io.uploader.Destroy(gTexture);
			}

			++destroyed;
		}

		while (uploaded < maxPerCall)
		{
			Id id{};
			TextureCPUData cpuData{};
			TextureProperties properties{};
			Handle handle{};

			{
				std::scoped_lock lock(mutex_);

				if (uploadQueue_.empty())
				{
					break;
				}

				id = std::move(uploadQueue_.front());
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

				cpuData = std::move(*entry.pendingCpu);
				entry.pendingCpu.reset();

				handle = entry.textureHandle;
				properties = handle->GetProperties();
			}
			

			std::optional<GPUTexture> gpuOpt = io.uploader.CreateAndUpload(cpuData, properties);

			{
				std::scoped_lock lock(mutex_);

				auto it = entries_.find(id);
				if (it == entries_.end())
				{
					continue;
				}

				TextureEntry& entry = it->second;

				if (!gpuOpt)
				{
					entry.state = ResourceState::Failed;
					entry.error = "GPU texture upload failed";
				}
				else
				{
					entry.textureHandle->SetResource(*gpuOpt);
					entry.state = ResourceState::Loaded;
					entry.error.clear();
				}
			}

			++uploaded;	
		}

		return (uploaded + destroyed) > 0;
	}

private:

	void EnqueueDestroy(GPUTexture texture)
	{
		if (texture.id != 0)
		{
			destroyQueue_.push_back(texture);
		}
	}

	mutable std::mutex mutex_{};
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
