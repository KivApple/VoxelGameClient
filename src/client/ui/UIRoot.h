#pragma once

#include <unordered_map>
#include <memory>
#include "UIBase.h"
#include "Crosshair.h"
#include "Inventory.h"
#include "UIJoystick.h"

class UserInterface: public UIElementGroup {
	GL::Buffer m_sharedBuffer;
	std::unordered_map<const void*, std::unique_ptr<GL::Buffer>> m_staticBuffers;
	std::unordered_map<const void*, std::unique_ptr<GL::Texture>> m_staticTextureInstances;
	std::unordered_map<unsigned int, std::unique_ptr<GL::Framebuffer>> m_sharedFramebuffers;
	Crosshair m_crosshair;
	Inventory m_inventory;
	UIJoystick m_joystick;
	UIJoystick m_verticalJoystick;
	bool m_joystickVisible = false;
	
	static glm::mat4 transformMatrix();
	
	const GL::Buffer &sharedBufferInstanceImpl();
	const GL::Buffer &staticBufferInstanceImpl(const void *data, size_t dataSize);
	const GL::Texture &staticTextureInstanceImpl(
			unsigned int width,
			unsigned int height,
			bool filter,
			const void *data
	);
	const GL::Framebuffer &sharedFrameBufferInstanceImpl(unsigned int width, unsigned int height, bool depth);
	
	friend class UIElement;
	
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
	const Inventory &inventory() const {
		return m_inventory;
	}
	Inventory &inventory() {
		return m_inventory;
	}
	
};
