#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include "Voxel.h"

class VoxelTypeRegistry {
	std::unordered_map<std::string, std::unique_ptr<VoxelType>> m_types;
	std::shared_mutex m_mutex;
	
public:
	void add(std::string name, std::unique_ptr<VoxelType> type);
	VoxelType *get(const std::string &name);

};
