#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include "Voxel.h"
#ifndef HEADLESS
#include "client/OpenGL.h"
#endif

class VoxelTypeRegistry {
	std::unordered_map<std::string, std::unique_ptr<VoxelType>> m_types;
	std::shared_mutex m_mutex;
#ifndef HEADLESS
	GL::Texture m_unknownBlockTexture;
#endif
	
	friend class UnknownVoxelType;

public:
	VoxelTypeRegistry();
	template<typename T, typename ...Args> T &make(std::string name, Args&&... args) {
		return reinterpret_cast<T&>(add(std::move(name), std::make_unique<T>(std::forward<Args>(args)...)));
	}
	VoxelType &add(std::string name, std::unique_ptr<VoxelType> type);
	VoxelType &get(const std::string &name);
	template<typename Callable> void forEach(Callable &&callable) {
		std::shared_lock<std::shared_mutex> lock(m_mutex);
		callable("empty", EmptyVoxelType::INSTANCE);
		for (auto &pair : m_types) {
			callable(pair.first, *pair.second);
		}
	}
	
};
