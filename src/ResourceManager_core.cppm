module;

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>
#include <concepts>
#include <type_traits>
#include <utility>
#include <stdexcept>

export module resource_manager:core;

export enum class TextureFormat : uint8_t
{
	RGB,
	RGBA,
	GRAYSCALE
};

export enum class ResourceState : uint8_t
{
	Unloaded,
	Loading,
	Loaded,
	Failed,
	Unknown
};

export struct TextureProperties
{
	std::uint32_t width{};
	std::uint32_t height{};
	TextureFormat format{ TextureFormat::RGBA };
	std::string filePath{};
	bool srgb{ true };
	bool generateMips{ true };
};

export struct TextureCPUData
{
	std::uint32_t width{};
	std::uint32_t height{};
	int channels{};
	TextureFormat format{ TextureFormat::RGBA };
	std::vector<unsigned char> pixels;
};

export struct GPUTexture
{
	unsigned int id{};
};

export class ITextureDecoder
{
public:
	virtual ~ITextureDecoder() = default;
	virtual std::optional<TextureCPUData> Decode(const TextureProperties& properties, std::string_view resolvedPath) = 0;
};

export class ITextureUploader
{
public:
	virtual ~ITextureUploader() = default;
	virtual std::optional<GPUTexture> CreateAndUpload(const TextureCPUData& cpuData, const TextureProperties& properties) = 0;
	virtual void Destroy(GPUTexture texture) noexcept = 0;
};

export struct TextureIO
{
	ITextureDecoder& decoder;
	ITextureUploader& uploader;
};

export template <class Resource>
class Texture
{
public:

	using Properties = TextureProperties;

	Texture() = default;

	template <typename PropertiesType>
		requires std::same_as<std::remove_cvref_t<PropertiesType>, Properties>
	explicit Texture(PropertiesType&& inProperties) : properties_(std::forward<PropertiesType>(inProperties))
	{
	}

	const Properties& GetProperties() const { return properties_; }
	const Resource& GetResource() const { return resource_; }

	template<typename PropertiesType>
		requires std::same_as<std::remove_cvref_t<PropertiesType>, Properties>
	void SetProperties(PropertiesType&& inProperties)
	{
		properties_ = std::forward<PropertiesType>(inProperties);
	}

	template<typename ResourceType>
		requires std::same_as<std::remove_cvref_t<ResourceType>, Resource>
	void SetResource(ResourceType&& inResource)
	{
		resource_ = std::forward<ResourceType>(inResource);
	}

private:
	Resource resource_{};
	Properties properties_{};
};

template <typename T>
concept TexturePropertyType = requires(const T & propertyType)
{
	{ propertyType.width }  -> std::convertible_to<uint32_t>;
	{ propertyType.height } -> std::convertible_to<uint32_t>;
	propertyType.format;
};

template<typename T>
struct ResourceTraits;

template <typename ResourceType>
class ResourceStorage
{
public:
	using Handle = std::shared_ptr<ResourceType>;
	using WeakHandle = std::weak_ptr<ResourceType>;
	using Id = std::string;

	template <typename... Args>
	Handle LoadOrGet(std::string_view id, Args&&... args)
		requires requires(const Id& sid)
	{
		{ ResourceTraits<ResourceType>::Load(sid, std::forward<Args>(args)...) } ->std::same_as<Handle>;
	}
	{
		Id key{ id };

		if (auto aliveResource = cache_.find(key); aliveResource != cache_.end())
		{
			if (auto alive = aliveResource->second.lock())
			{
				return aliveResource;
			}

			cache_.erase(aliveResource);
		}

		Handle resource = ResourceTraits<ResourceType>::Load(key, std::forward<Args>(args)...);
		cache_[std::move(key)] = resource;
		return resource;
	}

	Handle Find(const Id& id) const
	{
		Id key{ id };
		if (auto it = cache_.find(key); it != cache_.end())
		{
			return it->second.lock();
		}
		return {};
	}

	void UnloadUnused()
	{
		for (auto it = cache_.begin(); it != cache_.end(); )
		{
			if (it->second.expired())
			{
				it = cache_.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void Clear()
	{
		cache_.clear();
	}

private:
	std::unordered_map<Id, WeakHandle> cache_;
};

export class ResourceManager
{
public:
	template <typename T, typename... Args>
	std::shared_ptr<T> Load(const std::string& id, Args&&... argss)
	{
		return storage<T>().LoadOrGet(id, std::forward<Args>(argss)...);
	}

	template <typename T>
	std::shared_ptr<T> Get(const std::string& id)
	{
		return storage<T>().Find(id);
	}

	template <typename T>
	void UnloadUnused()
	{
		storage<T>().UnloadUnused();
	}

	template <typename T>
	void Clear()
	{
		storage<T>().Clear();
	}

	template <typename T>
	ResourceStorage<T>& GetStorage()
	{
		return storage<T>();
	}

private:
	template <typename T>
	static ResourceStorage<T>& StorageImpl()
	{
		static ResourceStorage<T> instance;
		return instance;
	}

	template <typename T>
	ResourceStorage<T>& storage()
	{
		return StorageImpl<T>();
	}

	template <typename T>
	const ResourceStorage<T>& storage() const
	{
		return StorageImpl<T>();
	}
};
