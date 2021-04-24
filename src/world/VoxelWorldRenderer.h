#pragma once

#include <unordered_set>
#include <optional>
#include <glm/mat4x4.hpp>
#include "VoxelLocation.h"

class VoxelWorld;

class VoxelWorldRenderer {
	VoxelWorld &m_world;
	std::unordered_set<VoxelChunkLocation> m_queue;
	
	std::optional<VoxelChunkLocation> getInvalidated(const glm::vec3 &playerPosition);
	void build(const VoxelChunkLocation &location);
	void buildInvalidated(const glm::vec3 &playerPosition);
	void render(
			const VoxelChunkLocation &location,
			const glm::vec3 &playerPosition,
			const glm::mat4 &view,
			const glm::mat4 &projection
	);
	
public:
	explicit VoxelWorldRenderer(VoxelWorld &world);
	void invalidate(const VoxelChunkLocation &location);
	void render(
			const glm::vec3 &playerPosition,
			int radius,
			const glm::mat4 &view,
			const glm::mat4 &projection
	);

};
