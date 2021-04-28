#pragma once

#include "UIBase.h"
#include "../OpenGL.h"

class Crosshair: public UIElement {
	static const float s_bufferData[];
	GLBuffer m_buffer;

public:
	Crosshair();
	void render(const glm::mat4 &transform) override;
	
};
