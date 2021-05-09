#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "UIRoot.h"
#include "client/GameEngine.h"

UserInterface::UserInterface(
): m_sharedBuffer(
		GL_ARRAY_BUFFER
), m_joystick(false, [](const glm::vec2 &position) {
	GameEngine::instance().updatePlayerMovement(&position.x, nullptr, &position.y);
}), m_verticalJoystick(true, [](const glm::vec2 &position) {
	GameEngine::instance().updatePlayerMovement(nullptr, &position.y, nullptr);
}), m_inventory(8, 1) {
	m_root = this;
	addChild(&m_crosshair, glm::vec2(0.0f), glm::vec2(0.05f));
	addChild(&m_inventory, glm::vec2(0.0f, -0.90f), glm::vec2(0.1f * 8, 0.1f));
}

glm::mat4 UserInterface::transformMatrix() {
	auto transform = glm::mat4(1.0f);
	int w = GameEngine::instance().viewportWidth();
	int h = GameEngine::instance().viewportHeight();
	transform = glm::scale(transform, glm::vec3(
			w > h ? (float) h / (float) w : 1.0f,
			h > w ? (float) w / (float) h : 1.0f,
			1.0f
	));
	return transform;
}

void UserInterface::render() {
	UIElementGroup::render(transformMatrix());
}

void UserInterface::mouseMove(const glm::vec2 &position) {
	setJoystickVisible(false);
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	UIElementGroup::mouseMove(glm::vec2(pos.x, pos.y));
}

bool UserInterface::mouseDown(const glm::vec2 &position) {
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	return UIElementGroup::mouseDown(glm::vec2(pos.x, pos.y));
}

void UserInterface::mouseDrag(const glm::vec2 &position) {
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	UIElementGroup::mouseDrag(glm::vec2(pos.x, pos.y));
}

void UserInterface::mouseUp(const glm::vec2 &position) {
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	UIElementGroup::mouseUp(glm::vec2(pos.x, pos.y));
}

bool UserInterface::touchStart(long long id, const glm::vec2 &position) {
	setJoystickVisible(true);
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	return UIElementGroup::touchStart(id, glm::vec2(pos.x, pos.y));
}

void UserInterface::touchMotion(long long id, const glm::vec2 &position) {
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	UIElementGroup::touchMotion(id, glm::vec2(pos.x, pos.y));
}

void UserInterface::touchEnd(long long id, const glm::vec2 &position) {
	auto pos = glm::inverse(transformMatrix()) * glm::vec4(position, 0.0f, 1.0f);
	UIElementGroup::touchEnd(id, glm::vec2(pos.x, pos.y));
}

void UserInterface::setJoystickVisible(bool visible) {
	if (m_joystickVisible == visible) return;
	m_joystickVisible = visible;
	if (visible) {
		addChild(&m_joystick, glm::vec2(-0.7f, -0.7f), glm::vec2(0.2f, 0.2f));
		addChild(&m_verticalJoystick, glm::vec2(0.7f, -0.7f), glm::vec2(0.2f, 0.2f));
	} else {
		removeChild(&m_joystick);
		removeChild(&m_verticalJoystick);
	}
}

const GL::Buffer &UserInterface::sharedBufferInstanceImpl() {
	return m_sharedBuffer;
}

const GL::Buffer &UserInterface::staticBufferInstanceImpl(const void *data, size_t dataSize) {
	auto it = m_staticBuffers.find(data);
	if (it == m_staticBuffers.end()) {
		it = m_staticBuffers.emplace(data, std::make_unique<GL::Buffer>(GL_ARRAY_BUFFER)).first;
		it->second->setData(data, dataSize, GL_STATIC_DRAW);
	}
	return *it->second;
}

const GL::Framebuffer &UserInterface::sharedFrameBufferInstanceImpl(unsigned int width, unsigned int height, bool depth) {
	assert(width <= 32768);
	assert(height <= 32768);
	unsigned int id = width << 17 | height << 1 | (depth ? 1 : 0);
	auto it = m_sharedFramebuffers.find(id);
	if (it == m_sharedFramebuffers.end()) {
		it = m_sharedFramebuffers.emplace(id, std::make_unique<GL::Framebuffer>(width, height, depth)).first;
	}
	return *it->second;
}
