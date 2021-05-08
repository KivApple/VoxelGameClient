#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <GL/glew.h>
#include "VoxelOutline.h"
#include "world/Voxel.h"
#include "../GameEngine.h"

VoxelOutline::VoxelOutline(): m_buffer(GL_ARRAY_BUFFER) {
}

bool VoxelOutline::set(
		const VoxelHolder &voxel,
		const VoxelLocation &location,
		const glm::vec3 &playerPosition,
		const glm::vec3 &playerDirection
) {
	if (!m_vertexData.empty() && m_voxelLocation == location) {
		return true;
	}
	
	m_vertexData.clear();
	voxel.buildVertexData(m_vertexData);
	if (m_vertexData.empty()) {
		return false;
	}
	
	glm::vec3 minPoint(INFINITY), maxPoint(-INFINITY);
	for (auto &vertex : m_vertexData) {
		minPoint.x = std::min(minPoint.x, vertex.x);
		maxPoint.x = std::max(maxPoint.x, vertex.x);
		minPoint.y = std::min(minPoint.y, vertex.y);
		maxPoint.y = std::max(maxPoint.y, vertex.y);
		minPoint.z = std::min(minPoint.z, vertex.z);
		maxPoint.z = std::max(maxPoint.z, vertex.z);
	}
	
	glm::vec3 origin((float) location.x, (float) location.y, (float) location.z);
	int nearestI = -1;
	float nearestDistance = INFINITY;
	for (int i = 0; i < m_vertexData.size(); i += 3) {
		auto &v0 = m_vertexData[i + 0];
		auto &v1 = m_vertexData[i + 1];
		auto &v2 = m_vertexData[i + 2];
		
		glm::vec2 point;
		float distance;
		if (glm::intersectRayTriangle(
				playerPosition,
				playerDirection,
				glm::vec3(v0.x, v0.y, v0.z) + origin,
				glm::vec3(v1.x, v1.y, v1.z) + origin,
				glm::vec3(v2.x, v2.y, v2.z) + origin,
				point,
				distance
		)) {
			if (distance < nearestDistance) {
				nearestI = i;
				nearestDistance = distance;
			}
		}
	}
	if (nearestI < 0) {
		m_vertexData.clear();
		m_text.clear();
		return false;
	}
	
	m_voxelLocation = location;
	
	glm::vec3 triangleCenter(
			(m_vertexData[nearestI + 0].x + m_vertexData[nearestI + 1].x + m_vertexData[nearestI + 2].x) / 3.0f,
			(m_vertexData[nearestI + 0].y + m_vertexData[nearestI + 1].y + m_vertexData[nearestI + 2].y) / 3.0f,
			(m_vertexData[nearestI + 0].z + m_vertexData[nearestI + 1].z + m_vertexData[nearestI + 2].z) / 3.0f
	);
	float maxCoordinate = std::max(
			std::max(fabsf(triangleCenter.x), fabsf(triangleCenter.y)),
			fabsf(triangleCenter.z)
	);
	m_direction = glm::vec3(
			fabsf(triangleCenter.x) == maxCoordinate ? copysign(1.0f, triangleCenter.x) : 0.0f,
			fabsf(triangleCenter.y) == maxCoordinate ? copysign(1.0f, triangleCenter.y) : 0.0f,
			fabsf(triangleCenter.z) == maxCoordinate ? copysign(1.0f, triangleCenter.z) : 0.0f
	);
	
	std::vector<float> data;
	for (auto &v : m_vertexData) {
		data.insert(data.end(), {v.x, v.y, v.z, 1.0f, 1.0f, 1.0f, 1.0f});
	}
	m_buffer.setData(data.data(), data.size() * sizeof(float), GL_DYNAMIC_DRAW);
	m_text = voxel.toString();
	return true;
}

void VoxelOutline::render(const glm::mat4 &view, const glm::mat4 &projection) {
	if (m_vertexData.empty()) return;
	
	auto &program = GameEngine::instance().commonShaderPrograms().color;

	program.use();
	
	program.setModel(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(
			(float) m_voxelLocation.x,
			(float) m_voxelLocation.y,
			(float) m_voxelLocation.z
	)), glm::vec3(1.01f)));
	program.setView(view);
	program.setProjection(projection);
	
	program.setColorUniform(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	
	program.setPositions(m_buffer.pointer(GL_FLOAT, 0, 7 * sizeof(float)));
	program.setColors(m_buffer.pointer(GL_FLOAT, 3 * sizeof(float), 7 * sizeof(float)));
	
	glDrawArrays(GL_LINE_STRIP, 0, m_vertexData.size());
}
