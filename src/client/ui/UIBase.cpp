#include <glm/gtc/matrix_transform.hpp>
#include "UIBase.h"
#include "UIRoot.h"

void UIElement::setParent(UIElement *parent) {
	m_root = parent != nullptr ? parent->m_root : nullptr;
}

bool UIElement::touchStart(long long id, const glm::vec2 &position) {
	if (m_touchStarted) return false;
	if (!mouseDown(position)) return false;
	m_touchId = id;
	m_touchStarted = true;
	return true;
}

void UIElement::touchMotion(long long id, const glm::vec2 &position) {
	if (!m_touchStarted || m_touchId != id) return;
	mouseDrag(position);
}

void UIElement::touchEnd(long long id, const glm::vec2 &position) {
	if (m_touchStarted && m_touchId == id) {
		mouseUp(position);
		m_touchStarted = false;
	}
}

const GL::Buffer &UIElement::sharedBufferInstance() {
	assert(m_root != nullptr);
	return m_root->sharedBufferInstanceImpl();
}

const GL::Buffer &UIElement::staticBufferInstance(const void *data, size_t dataSize) {
	assert(m_root != nullptr);
	return m_root->staticBufferInstanceImpl(data, dataSize);
}

const GL::Texture &UIElement::staticTextureInstance(
		unsigned int width,
		unsigned int height,
		bool filter,
		const void *data
) {
	assert(m_root != nullptr);
	return m_root->staticTextureInstanceImpl(width, height, filter, data);
}

const GL::Framebuffer &UIElement::sharedFramebufferInstance(unsigned int width, unsigned int height, bool depth) {
	assert(m_root != nullptr);
	return m_root->sharedFrameBufferInstanceImpl(width, height, depth);
}

void UIElementGroup::render(const glm::mat4 &transform) {
	for (auto &child : m_children) {
		child->element->render(
				glm::scale(
						glm::translate(
								transform,
								glm::vec3(child->position, 0.0f)
						),
						glm::vec3(child->size, 1.0f)
				)
		);
	}
}

void UIElementGroup::setParent(UIElement *parent) {
	UIElement::setParent(parent);
	for (auto &child : m_children) {
		child->element->setParent(this);
	}
}

void UIElementGroup::addChild(UIElement *element, const glm::vec2 &position, const glm::vec2 &size) {
	m_children.emplace_back(std::make_unique<UIElementChild>(element, position, size));
	element->setParent(this);
}

void UIElementGroup::removeChild(UIElement *element) {
	if (m_mouseElement != nullptr && m_mouseElement->element == element) {
		m_mouseElement = nullptr;
	}
	auto it = m_touchElements.begin();
	while (it != m_touchElements.end()) {
		if (it->second->element == element) {
			it = m_touchElements.erase(it);
		} else {
			++it;
		}
	}
	for (auto it2 = m_children.begin(); it2 != m_children.end(); ++it2) {
		if ((*it2)->element == element) {
			m_children.erase(it2);
			break;
		}
	}
	element->setParent(nullptr);
}

const UIElementChild *UIElementGroup::findByPosition(const glm::vec2 &position) {
	for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
		auto &child = *it;
		auto min = child->position - child->size;
		auto max = child->position + child->size;
		if (position.x >= min.x && position.y >= min.y && position.x <= max.x && position.y <= max.y) {
			return child.get();
		}
	}
	return nullptr;
}

void UIElementGroup::mouseMove(const glm::vec2 &position) {
	auto child = findByPosition(position);
	if (child != nullptr) {
		child->element->mouseMove(glm::vec2(
				(position.x - child->position.x) / child->size.x,
				(position.y - child->position.y) / child->size.y
		));
	}
}

bool UIElementGroup::mouseDown(const glm::vec2 &position) {
	auto child = findByPosition(position);
	if (child != nullptr && child->element->mouseDown(glm::vec2(
			(position.x - child->position.x) / child->size.x,
			(position.y - child->position.y) / child->size.y
	))) {
		m_mouseElement = child;
		return true;
	}
	return false;
}

void UIElementGroup::mouseDrag(const glm::vec2 &position) {
	if (m_mouseElement != nullptr) {
		m_mouseElement->element->mouseDrag(glm::vec2(
				(position.x - m_mouseElement->position.x) / m_mouseElement->size.x,
				(position.y - m_mouseElement->position.y) / m_mouseElement->size.y
		));
	}
}

void UIElementGroup::mouseUp(const glm::vec2 &position) {
	if (m_mouseElement != nullptr) {
		auto child = m_mouseElement;
		m_mouseElement = nullptr;
		child->element->mouseUp(glm::vec2(
				(position.x - child->position.x) / child->size.x,
				(position.y - child->position.y) / child->size.y
		));
	}
}

bool UIElementGroup::touchStart(long long id, const glm::vec2 &position) {
	auto child = findByPosition(position);
	if (child && child->element->touchStart(id, glm::vec2(
			(position.x - child->position.x) / child->size.x,
			(position.y - child->position.y) / child->size.y
	))) {
		m_touchElements.insert({id, child});
		return true;
	}
	return false;
}

void UIElementGroup::touchMotion(long long id, const glm::vec2 &position) {
	auto it = m_touchElements.find(id);
	if (it == m_touchElements.end()) return;
	auto child = it->second;
	child->element->touchMotion(id, glm::vec2(
			(position.x - child->position.x) / child->size.x,
			(position.y - child->position.y) / child->size.y
	));
}

void UIElementGroup::touchEnd(long long id, const glm::vec2 &position) {
	auto it = m_touchElements.find(id);
	if (it == m_touchElements.end()) return;
	auto child = it->second;
	m_touchElements.erase(it);
	child->element->touchEnd(id, glm::vec2(
			(position.x - child->position.x) / child->size.x,
			(position.y - child->position.y) / child->size.y
	));
}
