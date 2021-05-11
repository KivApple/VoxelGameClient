#pragma once

#include "Voxel.h"
#include "LiquidVoxelType.h"
#include "VoxelTypeRegistry.h"

class AirVoxelType: public VoxelTypeHelper<AirVoxelType, Voxel, EmptyVoxelType> {
public:
	std::string toString(const Voxel &voxel);
	bool hasDensity(const Voxel &voxel);
	
};

class GrassVoxelType: public VoxelTypeHelper<GrassVoxelType, Voxel, SimpleVoxelType> {
	VoxelType *m_dirt = nullptr;
	VoxelType *m_lava = nullptr;
	
public:
	GrassVoxelType();
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

class WaterVoxelType: public Liquid<WaterVoxelType, SimpleVoxelType>::VoxelType {
public:
	WaterVoxelType();
	
};

void registerVoxelTypes(VoxelTypeRegistry &registry);

class VoxelTypesRegistration {
public:
	explicit VoxelTypesRegistration(VoxelTypeRegistry &registry) {
		registerVoxelTypes(registry);
	}
	
};
