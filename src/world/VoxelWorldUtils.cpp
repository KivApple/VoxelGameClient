#include <cmath>
#include <glm/gtx/intersect.hpp>
#include "VoxelWorldUtils.h"
#include "VoxelWorld.h"

std::optional<PlayerPointingAt> findPlayerPointingAt(
		const VoxelChunkExtendedRef &chunk,
		const glm::vec3 &position,
		const glm::vec3 &direction
) {
	if (!chunk) return std::nullopt;
	std::vector<VoxelVertexData> vertexData;
	InChunkVoxelLocation prevLocation;
	for (float r = 0.0f; r < 4.0f; r += 0.1f) {
		auto p = position + direction * r;
		InChunkVoxelLocation location(
				(int) roundf(p.x) - chunk.location().x * VOXEL_CHUNK_SIZE,
				(int) roundf(p.y) - chunk.location().y * VOXEL_CHUNK_SIZE,
				(int) roundf(p.z) - chunk.location().z * VOXEL_CHUNK_SIZE
		);
		if (r == 0.0f || location != prevLocation) {
			prevLocation = location;
		} else {
			continue;
		}
		
		VoxelLocation globalLocation;
		auto &voxel = chunk.extendedAt(location, &globalLocation);
		
		vertexData.clear();
		voxel.buildVertexData(vertexData);
		if (vertexData.empty()) {
			continue;
		}
		
		glm::vec3 minPoint(INFINITY), maxPoint(-INFINITY);
		for (auto &vertex : vertexData) {
			minPoint.x = std::min(minPoint.x, vertex.x);
			maxPoint.x = std::max(maxPoint.x, vertex.x);
			minPoint.y = std::min(minPoint.y, vertex.y);
			maxPoint.y = std::max(maxPoint.y, vertex.y);
			minPoint.z = std::min(minPoint.z, vertex.z);
			maxPoint.z = std::max(maxPoint.z, vertex.z);
		}
		
		glm::vec3 origin(
				(float) (location.x + chunk.location().x * VOXEL_CHUNK_SIZE),
				(float) (location.y + chunk.location().y * VOXEL_CHUNK_SIZE),
				(float) (location.z + chunk.location().z * VOXEL_CHUNK_SIZE)
		);
		int nearestI = -1;
		glm::vec2 nearestPoint;
		float nearestDistance = INFINITY;
		for (int i = 0; i < vertexData.size(); i += 3) {
			auto &v0 = vertexData[i + 0];
			auto &v1 = vertexData[i + 1];
			auto &v2 = vertexData[i + 2];
			
			glm::vec2 point;
			float distance;
			if (glm::intersectRayTriangle(
					position,
					direction,
					glm::vec3(v0.x, v0.y, v0.z) + origin,
					glm::vec3(v1.x, v1.y, v1.z) + origin,
					glm::vec3(v2.x, v2.y, v2.z) + origin,
					point,
					distance
			)) {
				if (distance < nearestDistance) {
					nearestI = i;
					nearestPoint = point;
					nearestDistance = distance;
				}
			}
		}
		if (nearestI < 0) {
			continue;
		}
		
		glm::vec3 v1(vertexData[nearestI + 0].x, vertexData[nearestI + 0].y, vertexData[nearestI + 0].z);
		glm::vec3 v2(vertexData[nearestI + 1].x, vertexData[nearestI + 1].y, vertexData[nearestI + 1].z);
		glm::vec3 v3(vertexData[nearestI + 2].x, vertexData[nearestI + 2].y, vertexData[nearestI + 2].z);
		auto point = v1 + (v2 - v1) * nearestPoint.x + (v3 - v1) * nearestPoint.y;
		float maxCoordinate = std::max(std::max(fabsf(point.x), fabsf(point.y)),fabsf(point.z));
		
		return std::make_optional(PlayerPointingAt {
				globalLocation,
				location,
				glm::vec3(
						fabsf(point.x) == maxCoordinate ? copysign(1.0f, point.x) : 0.0f,
						fabsf(point.y) == maxCoordinate ? copysign(1.0f, point.y) : 0.0f,
						fabsf(point.z) == maxCoordinate ? copysign(1.0f, point.z) : 0.0f
				),
				std::move(vertexData)
		});
	}
	return std::nullopt;
}
