#pragma once

#include <glm/vec3.hpp>

class VoxelChunkExtendedRef;

enum class EntityPhysicsType {
	NONE,
	STATIC,
	DYNAMIC
};


class EntityPhysics {
	EntityPhysicsType m_type;
	int m_width;
	int m_height;
	float m_padding;
	float m_paddingY;
	
	static void constraintMovementAxis(
			int dx, int dy, int dz,
			float &a, float &b, float &c,
			float prevA, float prevB, float prevC,
			float paddingA, float paddingB, float paddingC,
			int start_, int end_
	);
	static void applyMovementConstraint(
			glm::vec3 &position,
			const glm::vec3 &prevPosition,
			int width, int height, float padding, float paddingY,
			const VoxelChunkExtendedRef &chunk
	);

public:
	static const EntityPhysics NONE;
	
	EntityPhysics();
	EntityPhysics(EntityPhysicsType type, int width, int height, float padding, float paddingY);
	[[nodiscard]] EntityPhysicsType type() const {
		return m_type;
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
	void applyMovementConstraint(
			glm::vec3 &position,
			const glm::vec3 &prevPosition,
			const VoxelChunkExtendedRef &chunk
	) const;

};
