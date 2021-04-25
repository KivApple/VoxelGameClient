#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

class UIElement {
	long long m_touchId = 0;
	bool m_touchStarted = false;
	
public:
	virtual ~UIElement() = default;
	virtual void render(const glm::mat4 &transform) = 0;
	virtual void mouseMove(const glm::vec2 &position) {
	}
	virtual bool mouseDown(const glm::vec2 &position) {
		return false;
	}
	virtual void mouseUp(const glm::vec2 &position) {
	}
	virtual void mouseDrag(const glm::vec2 &position) {
	}
	virtual bool touchStart(long long id, const glm::vec2 &position);
	virtual void touchMotion(long long id, const glm::vec2 &position);
	virtual void touchEnd(long long id, const glm::vec2 &position);
	
};

struct UIElementChild {
	UIElement *element;
	glm::vec2 position, size;
	
	constexpr UIElementChild(
			UIElement *element,
			const glm::vec2 &position,
			const glm::vec2 &size
	): element(element), position(position), size(size) {
	}
};

class UIElementGroup: public UIElement {
	std::vector<std::unique_ptr<UIElementChild>> m_children;
	const UIElementChild *m_mouseElement = nullptr;
	std::unordered_map<long long, const UIElementChild*> m_touchElements;
	
	const UIElementChild *findByPosition(const glm::vec2 &position);
	
public:
	void render(const glm::mat4 &transform) override;
	void addChild(UIElement *element, const glm::vec2 &position, const glm::vec2 &size);
	void removeChild(UIElement *element);
	void mouseMove(const glm::vec2 &position) override;
	bool mouseDown(const glm::vec2 &position) override;
	void mouseDrag(const glm::vec2 &position) override;
	void mouseUp(const glm::vec2 &position) override;
	bool touchStart(long long id, const glm::vec2 &position) override;
	void touchMotion(long long id, const glm::vec2 &position) override;
	void touchEnd(long long id, const glm::vec2 &position) override;
	
};
