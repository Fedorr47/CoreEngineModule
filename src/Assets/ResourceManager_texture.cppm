module;

#include <deque>
#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <exception>
#include <concepts>
#include <type_traits>

export module core:resource_manager_texture;

import :resource_manager_core;

export using TextureResource = Texture<GPUTexture>;

export struct TextureEntry
{
	using Handle = std::shared_ptr<TextureResource>;

	Handle textureHandle;
	ResourceState state{ ResourceState::Unloaded };
	std::uint64_t generation{ 0 };
	std::optional<TextureCPUData> pendingCpu{};
	std::string error{};
};

struct TextureUploadTicket
{
	std::string id;
	std::uint64_t generation{};
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
		Id stableKey = Id{ id };
		Handle handle{};
		std::uint64_t generation{};

		{
			std::scoped_lock lock(mutex_);
			if (auto it = entries_.find(stableKey); it != entries_.end())
			{
				// If the resource exists, return it. If it previously failed, restart loading.
				TextureEntry& existing = it->second;
				handle = existing.textureHandle;
				if (existing.state != ResourceState::Failed)
				{
					return handle;
				}

				// Restart failed load.
				existing.state = ResourceState::Loading;
				existing.error.clear();
				existing.pendingCpu.reset();
				++existing.generation;
				generation = existing.generation;
				handle->SetProperties(std::forward<PropertiesType>(properties));
			}
			else
			{
				TextureEntry entry{};
				entry.textureHandle = std::make_shared<Resource>(std::forward<PropertiesType>(properties));
				entry.state = ResourceState::Loading;
				entry.generation = 1;

				handle = entry.textureHandle;
				generation = entry.generation;
				entries_.emplace(stableKey, std::move(entry));
			}
		}

		TextureProperties propertiesCopy = handle->GetProperties();
		std::string path = propertiesCopy.filePath.empty() ? std::string(stableKey) : propertiesCopy.filePath;

		TextureIO ioCopy = io;

		ioCopy.jobs.Enqueue([this,
			key = stableKey,
			generation,
			propertiesCopy = std::move(propertiesCopy),
			path = std::move(path),
			ioCopy]() mutable
			{
				std::optional<TextureCPUData> cpuOpt{};
				std::string decodeError{};
				try
				{
					cpuOpt = ioCopy.decoder.Decode(propertiesCopy, path);
				}
				catch (const std::exception& e)
				{
					decodeError = e.what();
				}
				catch (...)
				{
					decodeError = "Texture decode threw (unknown exception)";
				}

				std::scoped_lock lock(mutex_);

				auto it = entries_.find(key);
				if (it == entries_.end())
				{
					return;
				}

				TextureEntry& entry = it->second;
				if (entry.generation != generation)
				{
					return;
				}

				if (entry.state != ResourceState::Loading)
				{
					return;
				}

				if (!cpuOpt)
				{
					entry.state = ResourceState::Failed;
					entry.error = decodeError.empty() ? "Texture decode failed" : decodeError;
					entry.pendingCpu.reset();
					return;
				}

				entry.pendingCpu = std::move(*cpuOpt);
				uploadQueue_.push_back(TextureUploadTicket{ key, generation });
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

	ResourceStreamingStats GetStreamingStats() const
	{
		std::scoped_lock lock(mutex_);

		ResourceStreamingStats stats{};
		stats.totalEntries = static_cast<std::uint32_t>(entries_.size());
		stats.queuedUploads = static_cast<std::uint32_t>(uploadQueue_.size());
		stats.queuedDestroys = static_cast<std::uint32_t>(destroyQueue_.size());

		for (const auto& [id, entry] : entries_)
		{
			switch (entry.state)
			{
			case ResourceState::Loading:
				++stats.loadingEntries;
				break;
			case ResourceState::Loaded:
				++stats.loadedEntries;
				break;
			case ResourceState::Failed:
				++stats.failedEntries;
				break;
			default:
				break;
			}

			if (entry.pendingCpu.has_value())
			{
				++stats.pendingCpuEntries;
			}
		}

		return stats;
	}

	bool ProcessUploads(TextureIO& io, std::size_t maxPerCall = 8, std::size_t maxDestroyedPerCall = 32)
	{
		struct PendingUpload
		{
			std::string id{};
			std::uint64_t generation{};
			Handle handle{};
			TextureProperties properties{};
			std::shared_ptr<TextureCPUData> cpuPtr{};
		};

		std::size_t uploaded = 0;
		std::size_t destroyed = 0;

		std::vector<PendingUpload> readyUploads{};
		readyUploads.reserve(maxPerCall);

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
				TextureIO ioCopy = io;
				ioCopy.render.Enqueue([ioCopy, gTexture]()
					{
						ioCopy.uploader.Destroy(gTexture);
					});
			}

			++destroyed;
		}

		while (uploaded < maxPerCall)
		{
			TextureUploadTicket ticket{};
			TextureProperties properties{};
			Handle handle{};
			TextureCPUData cpuData{};
			std::uint64_t generation{};

			{
				std::scoped_lock lock(mutex_);

				if (uploadQueue_.empty())
				{
					break;
				}

				ticket = std::move(uploadQueue_.front());
				uploadQueue_.pop_front();

				auto it = entries_.find(ticket.id);
				if (it == entries_.end())
				{
					continue;
				}

				TextureEntry& entry = it->second;
				generation = ticket.generation;
				if (entry.generation != generation)
				{
					continue;
				}

				if (entry.state != ResourceState::Loading || !entry.pendingCpu.has_value())
				{
					continue;
				}

				cpuData = std::move(*entry.pendingCpu);
				entry.pendingCpu.reset();

				handle = entry.textureHandle;
				properties = handle->GetProperties();
			}

			PendingUpload upload{};
			upload.id = std::move(ticket.id);
			upload.generation = generation;
			upload.handle = handle;
			upload.properties = std::move(properties);
			upload.cpuPtr = std::make_shared<TextureCPUData>(std::move(cpuData));
			readyUploads.push_back(std::move(upload));
			++uploaded;
		}

		if (!readyUploads.empty())
		{
			TextureIO ioCopy = io;
			ioCopy.render.Enqueue([this, ioCopy, uploads = std::move(readyUploads)]() mutable
				{
					struct UploadResult
					{
						std::optional<GPUTexture> gpuOpt{};
						std::string error{};
					};

					std::vector<UploadResult> results(uploads.size());
					bool batchBegun = false;
					std::string batchError{};

					try
					{
						ioCopy.uploader.BeginUploadBatch();
						batchBegun = true;
					}
					catch (const std::exception& e)
					{
						batchError = e.what();
					}
					catch (...)
					{
						batchError = "GPU texture upload batch begin threw";
					}

					for (std::size_t i = 0; i < uploads.size(); ++i)
					{
						auto& result = results[i];
						if (!batchBegun)
						{
							result.error = batchError.empty() ? "GPU texture upload batch begin failed" : batchError;
							continue;
						}

						try
						{
							result.gpuOpt = ioCopy.uploader.CreateAndUpload(*uploads[i].cpuPtr, uploads[i].properties);
							if (!result.gpuOpt)
							{
								result.error = "GPU texture upload failed";
							}
						}
						catch (const std::exception& e)
						{
							result.error = e.what();
						}
						catch (...)
						{
							result.error = "GPU texture upload threw";
						}
					}

					if (batchBegun)
					{
						try
						{
							ioCopy.uploader.EndUploadBatch();
						}
						catch (const std::exception& e)
						{
							batchError = e.what();
						}
						catch (...)
						{
							batchError = "GPU texture upload batch end threw";
						}
					}

					if (!batchError.empty())
					{
						for (auto& result : results)
						{
							if (result.gpuOpt && result.gpuOpt->id != 0)
							{
								ioCopy.uploader.Destroy(*result.gpuOpt);
								result.gpuOpt.reset();
							}
							if (result.error.empty())
							{
								result.error = batchError;
							}
						}
					}

					std::vector<GPUTexture> destroyList{};
					destroyList.reserve(results.size());

					{
						std::scoped_lock lock(mutex_);
						for (std::size_t i = 0; i < uploads.size(); ++i)
						{
							auto& upload = uploads[i];
							auto& result = results[i];
							auto it = entries_.find(upload.id);
							if (it == entries_.end())
							{
								if (result.gpuOpt && result.gpuOpt->id != 0)
								{
									destroyList.push_back(*result.gpuOpt);
								}
								continue;
							}

							TextureEntry& entry = it->second;
							if (entry.generation != upload.generation || entry.textureHandle != upload.handle || entry.state != ResourceState::Loading)
							{
								if (result.gpuOpt && result.gpuOpt->id != 0)
								{
									destroyList.push_back(*result.gpuOpt);
								}
								continue;
							}

							if (!result.gpuOpt)
							{
								entry.state = ResourceState::Failed;
								entry.error = result.error.empty() ? "GPU texture upload failed" : result.error;
								continue;
							}

							entry.textureHandle->SetResource(*result.gpuOpt);
							entry.state = ResourceState::Loaded;
							entry.error.clear();
						}
					}

					for (const GPUTexture texture : destroyList)
					{
						ioCopy.uploader.Destroy(texture);
					}
				});
		}

		return (uploaded + destroyed) > 0;
	}

private:

	void EnqueueDestroy(GPUTexture texture)
	{
		if (texture.id == 0)
		{
			return;
		}
		std::scoped_lock lock(mutex_);
		destroyQueue_.push_back(texture);
	}

	mutable std::mutex mutex_{};
	std::unordered_map<Id, TextureEntry> entries_;
	std::deque<TextureUploadTicket> uploadQueue_;
	std::deque<GPUTexture> destroyQueue_;
};