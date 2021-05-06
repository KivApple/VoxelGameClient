#include <GL/glew.h>
#include "Crosshair.h"
#include "../GameEngine.h"

const float Crosshair::BUFFER_DATA[] = {
		-1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f
};

void Crosshair::render(const glm::mat4 &transform) {
	auto &buffer = staticBufferInstance(BUFFER_DATA, sizeof(BUFFER_DATA));
	
	auto &program = GameEngine::instance().commonShaderPrograms().color;
	
	program.use();
	
	program.setModel(transform);
	program.setView(glm::mat4(1.0f));
	program.setProjection(glm::mat4(1.0f));
	
	program.setColorUniform(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	
	program.setPositions(buffer.pointer(GL_FLOAT, 0, 7 * sizeof(float)));
	program.setColors(buffer.pointer(GL_FLOAT, 3 * sizeof(float), 7 * sizeof(float)));
	
	glDrawArrays(GL_LINES, 0, sizeof(BUFFER_DATA) / sizeof(float) / 7);
}
