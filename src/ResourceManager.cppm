module;

#include <cstdint>
#include <algorithm>
#include <chrono>
#include <string>
#include <concepts>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <utility>
#include <deque>

export module resource_manager;

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

//---------------------- Texture section ----------------------//
template <typename T>
concept TexturePropertyType = requires(const T & propertyType)
{
	{ propertyType.width }  -> std::convertible_to<uint32_t>;
	{ propertyType.height } -> std::convertible_to<uint32_t>;
	propertyType.format;
};

export struct TextureProperties
{
	std::uint32_t width{};
	std::uint32_t height{};
	TextureFormat format{};
	std::string filePath{};
};

export template <typename T>
struct TextureTraits
{
	using Properties = TextureProperties;
};

export struct DefaultTexture {};

export template<>
struct TextureTraits<DefaultTexture>
{
	using Properties = TextureProperties;
};

export template <class Resource, class Traits = TextureTraits<Resource>>
class Texture
{
public:

	using Properties = typename Traits::Properties;

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
concept TextureType = requires(const T & textureType)
{
	typename T::properties_;
	requires TexturePropertyType<typename T::Properties>;
	{ textureType.GetProperties() } -> std::same_as<const typename T::Properties&>;
};
//------------------- End of Texture section ------------------//

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

		if (auto aliveResource = cache_.find(id); aliveResource != cache_.end())
		{
			return aliveResource->second.lock();
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
		return storage<T>().LoadOrGGet(id, std::forward<Args>(argss)...);
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

//---------------------- Texture mmanager section ----------------------//

export using TextureResource = Texture<DefaultTexture>;

export struct TextureEntry
{
	using Handle = std::shared_ptr<TextureResource>;

	Handle textureHandle;
	ResourceState state{ ResourceState::Unloaded };
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
	Handle LoadOrGet(std::string_view id, PropertiesType&& properties)
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

		// TODO: Add actual loading logic here and update the state accordingly


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

private:
	std::unordered_map<Id, TextureEntry> entries_;
	std::deque<Id> uploadQueue_;
};

export template <>
struct ResourceTraits<TextureResource>
{
	using Resource = TextureResource;

	// Simulated load function
};
//------------------- End of Texture mmanager section ------------------//

