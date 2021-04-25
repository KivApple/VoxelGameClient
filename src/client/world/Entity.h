#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Model.h"

class Entity {
	glm::vec3 m_position;
	float m_yaw;
	float m_pitch;
	Model *m_model;

public:
	Entity(const glm::vec3 &position, float yaw, float pitch, Model *model);
	virtual ~Entity() = default;
	[[nodiscard]] const glm::vec3 &position() const {
		return m_position;
	}
	[[nodiscard]] float yaw() const {
		return m_yaw;
	}
	[[nodiscard]] float pitch() const {
		return m_pitch;
	}
	void setPosition(const glm::vec3 &position);
	void adjustPosition(const glm::vec3 &dPosition);
	void setRotation(float yaw, float pitch);
	void adjustRotation(float dYaw, float dPitch);
	[[nodiscard]] glm::vec3 direction(bool usePitch) const;
	[[nodiscard]] glm::vec3 upDirection() const;
	void move(const glm::vec3 &delta);
	void render(const glm::mat4 &view, const glm::mat4 &projection) const;
	
};
