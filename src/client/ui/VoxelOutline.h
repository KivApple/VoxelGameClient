#pragma once

#include <vector>
#include <glm/mat4x4.hpp>
#include "../OpenGL.h"
#include "world/VoxelLocation.h"

class VoxelHolder;
class VoxelVertexData;

class VoxelOutline {
	GLBuffer m_buffer;
	std::vector<VoxelVertexData> m_vertexData;
	VoxelLocation m_voxelLocation;
	glm::vec3 m_direction = glm::vec3(0.0f);
	std::string m_text;
	
public:
	VoxelOutline();
	bool set(
			const VoxelHolder &voxel,
			const VoxelLocation &location,
			const glm::vec3 &playerPosition,
			const glm::vec3 &playerDirection
	);
	void render(const glm::mat4 &view, const glm::mat4 &projection);
	[[nodiscard]] bool voxelDetected() const {
		return !m_vertexData.empty();
	}
	[[nodiscard]] const VoxelLocation &voxelLocation() const {
		return m_voxelLocation;
	}
	[[nodiscard]] const glm::vec3 &direction() const {
		return m_direction;
	}
	[[nodiscard]] const std::string &text() const {
		return m_text;
	}
	
};
