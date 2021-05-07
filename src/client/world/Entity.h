#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Model.h"

class VoxelWorld;
class VoxelChunkExtendedRef;

class Entity {
	VoxelWorld &m_world;
	glm::vec3 m_position;
	float m_yaw;
	float m_pitch;
	int m_width;
	int m_height;
	float m_padding;
	float m_paddingY;
	Model *m_model;
	
	static void constraintMovementAxis(
			int dx, int dy, int dz,
			float &a, float &b, float &c,
			float prevA, float prevB, float prevC,
			float paddingA, float paddingB, float paddingC,
			int start_, int end_
	);
	void applyMovementConstraint(
			glm::vec3 &position,
			const glm::vec3 &prevPosition,
			int width, int height, float padding, float paddingY,
			const VoxelChunkExtendedRef &chunk
	) const;

public:
	Entity(
			VoxelWorld &world,
			const glm::vec3 &position,
			float yaw,
			float pitch,
			int width,
			int height,
			float padding,
			float paddingY,
			Model *model
	);
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
	[[nodiscard]] int width() const {
		return m_width;
	}
	[[nodiscard]] int height() const {
		return m_height;
	}
	[[nodiscard]] float padding() const {
		return m_padding;
	}
	[[nodiscard]] float paddingY() const {
		return m_paddingY;
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
