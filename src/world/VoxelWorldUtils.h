#pragma once

#include <cmath>
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

constexpr static bool almostEqual(float a, float b) {
	return fabsf(a - b) < 0.001f;;
}
