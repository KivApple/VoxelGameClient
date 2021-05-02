#pragma once

class AirVoxelType: public VoxelTypeHelper<AirVoxelType, Voxel, EmptyVoxelType> {
public:
	std::string toString(const Voxel &voxel) {
		return "air";
	}
	
};

