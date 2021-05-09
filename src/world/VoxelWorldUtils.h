#pragma once

#include <optional>
#include <glm/vec3.hpp>
#include "VoxelLocation.h"
#include "Voxel.h"

class VoxelChunkExtendedRef;

struct PlayerPointingAt {
	VoxelLocation location;
	InChunkVoxelLocation inChunkLocation;
	glm::vec3 direction;
	std::vector<VoxelVertexData> vertexData;
};

std::optional<PlayerPointingAt> findPlayerPointingAt(
		const VoxelChunkExtendedRef &chunk,
		const glm::vec3 &position,
		const glm::vec3 &direction
);
