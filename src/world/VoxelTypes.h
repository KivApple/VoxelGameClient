#pragma once

#include "Voxel.h"
#include "LiquidVoxelType.h"
#include "VoxelTypeRegistry.h"

class AssetLoader;

class AirVoxelType: public VoxelType<AirVoxelType, EmptyVoxelTypeTraitState, EmptyVoxelType> {
public:
	AirVoxelType();
	std::string toString(const State &voxel);
	bool hasDensity(const State &voxel);
	
};

class GrassVoxelType: public VoxelType<GrassVoxelType, EmptyVoxelTypeTraitState, SimpleVoxelType> {
	VoxelTypeInterface *m_dirt = nullptr;
	VoxelTypeInterface *m_lava = nullptr;
	
public:
	explicit GrassVoxelType(AssetLoader &loader);
	void link(VoxelTypeRegistry &registry);
	void slowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	
};

class WaterVoxelType: public VoxelType<
		WaterVoxelType,
		EmptyVoxelTypeTraitState,
		SimpleVoxelType, LiquidVoxelTrait<SimpleVoxelType>
> {
public:
	explicit WaterVoxelType(AssetLoader &loader);
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		return VoxelType::update(chunk, location, rawVoxel, voxel, deltaTime, invalidatedLocations);
	}
	
};

void registerVoxelTypes(VoxelTypeRegistry &registry, AssetLoader &loader);

class VoxelTypesRegistration {
public:
	explicit VoxelTypesRegistration(VoxelTypeRegistry &registry, AssetLoader &loader) {
		registerVoxelTypes(registry, loader);
	}
	
};
