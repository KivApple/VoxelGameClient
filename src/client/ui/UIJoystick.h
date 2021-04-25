#pragma once

#include <functional>
#include "UIBase.h"
#include "client/OpenGL.h"

class UIJoystick: public UIElement {
	static float s_bufferData[];
	GLBuffer m_buffer;
	bool m_vertical;
	std::function<void(const glm::vec2&)> m_callback;
	glm::vec2 m_position = glm::vec2(0.0f);
	bool m_active = false;
	
public:
	explicit UIJoystick(bool vertical, std::function<void(const glm::vec2&)> callback = nullptr);
	void render(const glm::mat4 &transform) override;
	bool mouseDown(const glm::vec2 &position) override;
	void mouseDrag(const glm::vec2 &position) override;
	void mouseUp(const glm::vec2 &position) override;
	
};
