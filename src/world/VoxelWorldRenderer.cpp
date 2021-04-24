#include "VoxelWorldRenderer.h"
#include "VoxelWorld.h"

VoxelWorldRenderer::VoxelWorldRenderer(VoxelWorld &world): m_world(world) {
}

void VoxelWorldRenderer::invalidate(const VoxelChunkLocation &location) {
	m_queue.emplace(location);
}

std::optional<VoxelChunkLocation> VoxelWorldRenderer::getInvalidated(const glm::vec3 &playerPosition) {
	auto nearestIt = m_queue.end();
	float nearestDistance = INFINITY;
	for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
		float dx = (float) it->x - playerPosition.x;
		float dy = (float) it->y - playerPosition.y;
		float dz = (float) it->z - playerPosition.z;
		float distance = dx * dx + dy * dy + dz * dz;
		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearestIt = it;
		}
	}
	if (nearestIt == m_queue.end()) {
		return std::nullopt;
	}
	auto location = *nearestIt;
	m_queue.erase(nearestIt);
	return std::make_optional(location);
}

void VoxelWorldRenderer::buildInvalidated(const glm::vec3 &playerPosition) {
	auto location = getInvalidated(playerPosition);
	if (location.has_value()) {
		build(*location);
	}
}

void VoxelWorldRenderer::build(const VoxelChunkLocation &location) {
	auto chunk = m_world.extendedChunk(location);
	if (!chunk) {
		return;
	}
	// TODO
}

void VoxelWorldRenderer::render(
		const VoxelChunkLocation &location,
		const glm::vec3 &playerPosition,
		const glm::mat4 &view,
		const glm::mat4 &projection
) {
	// TODO
}

void VoxelWorldRenderer::render(
		const glm::vec3 &playerPosition,
		int radius,
		const glm::mat4 &view,
		const glm::mat4 &projection
) {
	buildInvalidated(playerPosition);
	VoxelChunkLocation playerChunkLocation(
			(int) roundf(playerPosition.x / VOXEL_CHUNK_SIZE),
			(int) roundf(playerPosition.y / VOXEL_CHUNK_SIZE),
			(int) roundf(playerPosition.z / VOXEL_CHUNK_SIZE)
	);
	for (int r = 0; r <= radius; r++) {
		for (int i = -r; i <= r; i++) {
			render({
				playerChunkLocation.x - i,
				playerChunkLocation.y,
				playerChunkLocation.z
			}, playerPosition, view, projection);
			if (r > 0) {
				render({
					playerChunkLocation.x + i,
					playerChunkLocation.y,
					playerChunkLocation.z
				}, playerPosition, view, projection);
			}
			render({
				playerChunkLocation.x,
				playerChunkLocation.y - i,
				playerChunkLocation.z
			}, playerPosition, view, projection);
			if (r > 0) {
				render({
					playerChunkLocation.x,
					playerChunkLocation.y + i,
					playerChunkLocation.z
				}, playerPosition, view, projection);
			}
			render({
				playerChunkLocation.x,
				playerChunkLocation.y,
				playerChunkLocation.z - i
			}, playerPosition, view, projection);
			if (r > 0) {
				render({
					playerChunkLocation.x,
					playerChunkLocation.y,
					playerChunkLocation.z + i
				}, playerPosition, view, projection);
			}
		}
	}
}
