#include "VoxelTypes.h"

std::string AirVoxelType::toString(const Voxel &voxel) {
	return "air";
}

void registerVoxelTypes(VoxelTypeRegistry &registry) {
	registry.make<AirVoxelType>("air");
	registry.make<SimpleVoxelType>("grass", "grass", "assets/textures/grass.png", true);
	registry.make<SimpleVoxelType>("dirt", "dirt", "assets/textures/mud.png");
	registry.make<SimpleVoxelType>("stone", "stone", "assets/textures/stone.png");
	registry.make<SimpleVoxelType>(
			"lava",
			"lava",
			"assets/textures/lava.png",
			false,
			MAX_VOXEL_LIGHT_LEVEL - 1
	);
	registry.make<SimpleVoxelType>(
			"glass",
			"glass",
			"assets/textures/glass.png",
			false,
			0,
			true
	);
}
