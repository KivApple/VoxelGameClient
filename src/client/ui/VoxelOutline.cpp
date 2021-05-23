#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <GL/glew.h>
#include "VoxelOutline.h"
#include "world/Voxel.h"
#include "world/VoxelWorldUtils.h"
#include "../GameEngine.h"

VoxelOutline::VoxelOutline(): m_buffer(GL_ARRAY_BUFFER) {
}

void VoxelOutline::set(const VoxelChunkExtendedRef &chunk, std::optional<PlayerPointingAt> &&pointingAt) {
	if (pointingAt.has_value()) {
		if (!m_vertexData.empty() && m_voxelLocation == pointingAt->location) {
			return;
		}
		m_vertexData = std::move(pointingAt->vertexData);
		m_voxelLocation = pointingAt->location;
		m_direction = pointingAt->direction;
		
		std::vector<float> data;
		for (auto &v : m_vertexData) {
			data.insert(data.end(), {v.x, v.y, v.z, 1.0f, 1.0f, 1.0f, 1.0f});
		}
		m_buffer.setData(data.data(), data.size() * sizeof(float), GL_DYNAMIC_DRAW);
		m_text = chunk.extendedAt(pointingAt->inChunkLocation).toString();
	} else {
		m_vertexData.clear();
	}
}

void VoxelOutline::render(const glm::mat4 &view, const glm::mat4 &projection) {
	if (m_vertexData.empty()) return;
	
	auto &program = GameEngine::instance().commonShaderPrograms().ui.color;

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
