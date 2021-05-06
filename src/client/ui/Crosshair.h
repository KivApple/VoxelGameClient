#pragma once

#include "UIBase.h"
#include "../OpenGL.h"

class Crosshair: public UIElement {
	static const float BUFFER_DATA[];

public:
	void render(const glm::mat4 &transform) override;
	
};
