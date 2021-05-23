#include <algorithm>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "UIJoystick.h"
#include "client/GameEngine.h"

const float UIJoystick::BUFFER_DATA[] = {
		-1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		
		-1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f
};

UIJoystick::UIJoystick(
		bool vertical,
		std::function<void(const glm::vec2&)> callback
): m_vertical(vertical), m_callback(std::move(callback)) {
}

void UIJoystick::render(const glm::mat4 &transform) {
	auto &buffer = staticBufferInstance(BUFFER_DATA, sizeof(BUFFER_DATA));
	
	auto &program = GameEngine::instance().commonShaderPrograms().ui.color;
	
	program.use();
	
	program.setModel(transform);
	program.setView(glm::mat4(1.0f));
	program.setProjection(glm::mat4(1.0f));
	
	program.setColorUniform(glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));
	
	program.setPositions(buffer.pointer(GL_FLOAT, 0, 7 * sizeof(float)));
	program.setColors(buffer.pointer(GL_FLOAT, 3 * sizeof(float), 7 * sizeof(float)));
	
	glDrawArrays(GL_TRIANGLES, 0, sizeof(BUFFER_DATA) / sizeof(float) / 7);
	
	program.setColorUniform(
			m_active ?
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) :
			glm::vec4(0.8f, 0.8f, 0.8f, 0.5f)
	);
	program.setModel(glm::scale(
			glm::translate(transform, glm::vec3(m_position, 1.0f)),
			glm::vec3(0.3f, 0.3f, 1.0f))
	);
	
	glDrawArrays(GL_TRIANGLES, 0, sizeof(BUFFER_DATA) / sizeof(float) / 7);
}

bool UIJoystick::mouseDown(const glm::vec2 &position) {
	if (fabsf(position.x) <= 0.3f && fabsf(position.y) <= 0.3f) {
		m_active = true;
		return true;
	}
	return false;
}

void UIJoystick::mouseDrag(const glm::vec2 &position) {
	m_position = glm::vec2(
			m_vertical ? 0.0f : std::clamp(position.x, -1.0f, 1.0f),
			std::clamp(position.y, -1.0f, 1.0f)
	);
	if (m_callback) {
		m_callback(m_position);
	}
}

void UIJoystick::mouseUp(const glm::vec2 &position) {
	m_active = false;
	m_position = glm::vec2(0.0f);
	if (m_callback) {
		m_callback(m_position);
	}
}
