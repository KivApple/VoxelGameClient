#pragma once

#include "Voxel.h"
#include "LiquidVoxelType.h"
#include "VoxelTypeRegistry.h"

class AssetLoader;

class AirVoxelType: public VoxelType<AirVoxelType, Voxel, EmptyVoxelType> {
public:
	std::string toString(const Voxel &voxel);
	bool hasDensity(const Voxel &voxel);
	
};

class GrassVoxelType: public VoxelType<GrassVoxelType, Voxel, SimpleVoxelType> {
	VoxelTypeInterface *m_dirt = nullptr;
	VoxelTypeInterface *m_lava = nullptr;
	
public:
	explicit GrassVoxelType(AssetLoader &loader);
	void link(VoxelTypeRegistry &registry) override;
	void slowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	
};

class WaterVoxelType: public Liquid<WaterVoxelType, SimpleVoxelType>::SourceVoxelType {
public:
	explicit WaterVoxelType(AssetLoader &loader);
	
};

void registerVoxelTypes(VoxelTypeRegistry &registry, AssetLoader &loader);

class VoxelTypesRegistration {
public:
	explicit VoxelTypesRegistration(VoxelTypeRegistry &registry, AssetLoader &loader) {
		registerVoxelTypes(registry, loader);
	}
	
};
