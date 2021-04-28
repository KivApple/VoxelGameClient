#pragma once

#include "UIBase.h"
#include "Crosshair.h"
#include "UIJoystick.h"

class UserInterface: public UIElementGroup {
	Crosshair m_crosshair;
	UIJoystick m_joystick;
	UIJoystick m_verticalJoystick;
	bool m_joystickVisible = false;
	
	static glm::mat4 transformMatrix();

public:
	UserInterface();
	void render();
	void mouseMove(const glm::vec2 &position) override;
	bool mouseDown(const glm::vec2 &position) override;
	void mouseDrag(const glm::vec2 &position) override;
	void mouseUp(const glm::vec2 &position) override;
	bool touchStart(long long id, const glm::vec2 &position) override;
	void touchMotion(long long id, const glm::vec2 &position) override;
	void touchEnd(long long id, const glm::vec2 &position) override;
	void setJoystickVisible(bool visible);
	
};
