#pragma once

#include "Voxel.h"
#include "VoxelTypeRegistry.h"

class AirVoxelType: public VoxelTypeHelper<AirVoxelType, Voxel, EmptyVoxelType> {
public:
	std::string toString(const Voxel &voxel);
	
};

void registerVoxelTypes(VoxelTypeRegistry &registry);

class VoxelTypesRegistration {
public:
	explicit VoxelTypesRegistration(VoxelTypeRegistry &registry) {
		registerVoxelTypes(registry);
	}
	
};
