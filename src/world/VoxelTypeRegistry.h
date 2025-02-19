#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include "Voxel.h"
#ifndef HEADLESS
#include "client/OpenGL.h"
#endif

class AssetLoader;

class VoxelTypeRegistry {
	std::unordered_map<std::string, std::unique_ptr<VoxelTypeInterface>> m_types;
	std::shared_mutex m_mutex;
	AssetLoader &m_assetLoader;
#ifndef HEADLESS
	GL::Texture m_unknownBlockTexture;
#endif
	
	friend class UnknownVoxelType;

public:
	explicit VoxelTypeRegistry(AssetLoader &assetLoader);
	template<typename T, typename ...Args> T &make(std::string name, Args&&... args) {
		return reinterpret_cast<T&>(add(std::move(name), std::make_unique<T>(std::forward<Args>(args)...)));
	}
	VoxelTypeInterface &add(std::string name, std::unique_ptr<VoxelTypeInterface> type);
	VoxelTypeInterface &get(const std::string &name);
	template<typename Callable> void forEach(Callable &&callable) {
		std::shared_lock<std::shared_mutex> lock(m_mutex);
		callable("empty", EmptyVoxelType::INSTANCE);
		for (auto &pair : m_types) {
			callable(pair.first, *pair.second);
		}
	}
	void link();
	
};
