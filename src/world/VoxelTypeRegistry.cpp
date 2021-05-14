#include <easylogging++.h>
#include "VoxelTypeRegistry.h"
#include "Asset.h"

class UnknownVoxelType: public VoxelType<UnknownVoxelType, Voxel, SimpleVoxelType> {
public:
	UnknownVoxelType(VoxelTypeRegistry &registry, std::string name, AssetLoader &loader): VoxelType(
			std::move(name),
#ifdef HEADLESS
			loader.load("assets/textures/unknown_block.png")
#else
			registry.m_unknownBlockTexture
#endif
	) {
	}
	
};

VoxelTypeRegistry::VoxelTypeRegistry(AssetLoader &assetLoader): m_assetLoader(assetLoader) {
#ifndef HEADLESS
	m_unknownBlockTexture = GL::Texture(assetLoader.load("assets/textures/unknown_block.png"));
#endif
}

VoxelTypeInterface &VoxelTypeRegistry::add(std::string name, std::unique_ptr<VoxelTypeInterface> type) {
	type->registerChildren(name, *this);
	auto &typeRef = *type;
	std::unique_lock<std::shared_mutex> lock(m_mutex);
	LOG(INFO) << "Registered \"" << name << "\" voxel type";
	m_types.emplace(std::move(name), std::move(type));
	return typeRef;
}

VoxelTypeInterface &VoxelTypeRegistry::get(const std::string &name) {
	if (name == "empty") {
		return EmptyVoxelType::INSTANCE;
	}
	std::shared_lock<std::shared_mutex> lock(m_mutex);
	auto it = m_types.find(name);
	if (it != m_types.end()) {
		return *it->second;
	}
	lock.unlock();
	add(name, std::make_unique<UnknownVoxelType>(*this, name, m_assetLoader));
	return get(name);
}

void VoxelTypeRegistry::link() {
	for (auto &type : m_types) {
		type.second->link(*this);
	}
}
