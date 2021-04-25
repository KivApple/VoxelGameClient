#include <glm/gtc/matrix_transform.hpp>
#include "Entity.h"

Entity::Entity(
		const glm::vec3 &position,
		float yaw,
		float pitch,
		Model *model
): m_position(position), m_yaw(yaw), m_pitch(pitch), m_model(model) {
}

void Entity::setPosition(const glm::vec3 &position) {
	m_position = position;
}

void Entity::adjustPosition(const glm::vec3 &dPosition) {
	m_position += dPosition;
}

void Entity::setRotation(float yaw, float pitch) {
	m_yaw = yaw;
	m_pitch = pitch;
}

void Entity::adjustRotation(float dYaw, float dPitch) {
	m_yaw += dYaw;
	m_pitch += dPitch;
	
	while (m_yaw < -360.0f) {
		m_yaw += 360.0f;
	}
	while (m_yaw > 360.0f) {
		m_yaw -= 360.0f;
	}
	
	if (m_pitch < -89.0f) {
		m_pitch = -89.0f;
	}
	if (m_pitch > 89.0f) {
		m_pitch = 89.0f;
	}
}

glm::vec3 Entity::direction(bool usePitch) const {
	glm::vec3 direction(0.0f);
	direction.x = cos(glm::radians(m_yaw));
	direction.y = 0.0f;
	direction.z = sin(glm::radians(m_yaw));
	if (usePitch) {
		direction.x *= cos(glm::radians(m_pitch));
		direction.y = sin(glm::radians(m_pitch));
		direction.z *= cos(glm::radians(m_pitch));
	}
	return direction;
}

glm::vec3 Entity::upDirection() const {
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

void Entity::move(const glm::vec3 &delta) {
	auto direction = this->direction(false);
	adjustPosition(
			(direction * delta.z + glm::normalize(glm::cross(direction, upDirection())) * delta.x) +
			glm::vec3(0.0f, delta.y, 0.0f)
	);
}

void Entity::render(const glm::mat4 &view, const glm::mat4 &projection) const {
	if (m_model == nullptr) return;
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, m_position);
	m_model->render(transform, view, projection);
}
