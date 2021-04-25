#include "VoxelTypeRegistry.h"

class UnknownVoxelType: public SimpleVoxelType {
public:
	explicit UnknownVoxelType(std::string name): SimpleVoxelType(
			std::move(name),
			"assets/textures/unknown_block.png"
	) {
	}
	
};

void VoxelTypeRegistry::add(std::string name, std::unique_ptr<VoxelType> type) {
	std::unique_lock<std::shared_mutex> lock(m_mutex);
	m_types.emplace(std::move(name), std::move(type));
}

VoxelType &VoxelTypeRegistry::get(const std::string &name) {
	std::shared_lock<std::shared_mutex> lock(m_mutex);
	auto it = m_types.find(name);
	if (it != m_types.end()) {
		return *it->second;
	}
	lock.unlock();
	add(name, std::make_unique<UnknownVoxelType>(name));
	return get(name);
}
