#include "VoxelTypeRegistry.h"

void VoxelTypeRegistry::add(std::string name, std::unique_ptr<VoxelType> type) {
	std::unique_lock<std::shared_mutex> lock(m_mutex);
	m_types.emplace(std::move(name), std::move(type));
}

VoxelType *VoxelTypeRegistry::get(const std::string &name) {
	std::shared_lock<std::shared_mutex> lock(m_mutex);
	auto it = m_types.find(name);
	return it != m_types.end() ? it->second.get() : nullptr;
}
