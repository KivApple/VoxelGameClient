#include <GL/glew.h>
#include "Crosshair.h"
#include "../GameEngine.h"

const float Crosshair::s_bufferData[] = {
		-1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f
};

Crosshair::Crosshair(): m_buffer(GL_ARRAY_BUFFER) {
	m_buffer.setData(s_bufferData, sizeof(s_bufferData), GL_STATIC_DRAW);
}

void Crosshair::render(const glm::mat4 &transform) {
	auto &program = GameEngine::instance().commonShaderPrograms().color;
	
	program.use();
	
	program.setModel(transform);
	program.setView(glm::mat4(1.0f));
	program.setProjection(glm::mat4(1.0f));
	
	program.setColorUniform(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	
	program.setPositions(m_buffer.pointer(GL_FLOAT, 0, 7 * sizeof(float)));
	program.setColors(m_buffer.pointer(GL_FLOAT, 3 * sizeof(float), 7 * sizeof(float)));
	
	glDrawArrays(GL_LINES, 0, sizeof(s_bufferData) / sizeof(float) / 7);
}
