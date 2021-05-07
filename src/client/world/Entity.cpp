#include <glm/gtc/matrix_transform.hpp>
#include "Entity.h"
#include "world/VoxelWorld.h"

Entity::Entity(
		VoxelWorld &world,
		const glm::vec3 &position,
		float yaw,
		float pitch,
		int width,
		int height,
		float padding,
		float paddingY,
		Model *model
): m_world(world), m_position(position), m_yaw(yaw), m_pitch(pitch),
	m_width(width), m_height(height), m_padding(padding), m_paddingY(paddingY), m_model(model)
{
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
	auto prevPosition = m_position;
	adjustPosition(
			(direction * delta.z + glm::normalize(glm::cross(direction, upDirection())) * delta.x) +
			glm::vec3(0.0f, delta.y, 0.0f)
	);
	auto chunk = m_world.extendedChunk({
		(int) roundf(prevPosition.x) / VOXEL_CHUNK_SIZE,
		(int) roundf(prevPosition.y) / VOXEL_CHUNK_SIZE,
		(int) roundf(prevPosition.z) / VOXEL_CHUNK_SIZE
	});
	if (!chunk) return;
	applyMovementConstraint(m_position, prevPosition, m_width, m_height, m_padding, m_paddingY, chunk);
}

void Entity::render(const glm::mat4 &view, const glm::mat4 &projection) const {
	if (m_model == nullptr) return;
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, m_position);
	m_model->render(transform, view, projection);
}

void Entity::constraintMovementAxis(
		int dx, int dy, int dz,
		float &a, float &b, float &c,
		float prevA, float prevB, float prevC,
		float paddingA, float paddingB, float paddingC, int start_, int end_
) {
	(void) prevB;
	(void) b;
	
	if ((dx > 0) && (dy > start_) && (dy < end_) && (dz > 0)) {        //a c
		if ((a > roundf(prevA) + paddingA) && (c > roundf(prevC) + paddingC)) {
			if (abs(a - roundf(prevA)) < abs((c - roundf(prevC)))) {
				a = roundf(prevA) + paddingA;
			} else {
				c = roundf(prevC) + paddingC;
			}
		}
	} else if ((dx > 0) && (dy > start_) && (dy < end_) && (dz < 0)) {        //a -c
		if ((a > roundf(prevA) + paddingA) && (c < roundf(prevC) - paddingC)) {
			if (abs(a - roundf(prevA)) < abs((c - roundf(prevC)))) {
				a = roundf(prevA) + paddingA;
			} else {
				c = roundf(prevC) - paddingC;
			}
		}
	} else if ((dx < 0) && (dy > start_) && (dy < end_) && (dz > 0)) {        //-a c
		if ((a < roundf(prevA) - paddingA) && (c > roundf(prevC) + paddingC)) {
			if (abs(a - roundf(prevA)) < abs((c - roundf(prevC)))) {
				a = roundf(prevA) - paddingA;
			} else {
				c = roundf(prevC) + paddingC;
			}
		}
	} else if ((dx < 0) && (dy > start_) && (dy < end_) && (dz < 0)) {        //-a -c
		if ((a < roundf(prevA) - paddingA) && (c < roundf(prevC) - paddingC)) {
			if (abs(a - roundf(prevA)) < abs((c - roundf(prevC)))) {
				a = roundf(prevA) - paddingA;
			} else {
				c = roundf(prevC) - paddingC;
			}
		}
	}
}

void Entity::applyMovementConstraint(
		glm::vec3 &position, const glm::vec3 &prevPosition,
		int width, int height, float padding, float paddingY,
		const VoxelChunkExtendedRef &chunk
) const {
	(void) paddingY;
	
	VoxelLocation chunkLocationZero(chunk.location(), {0, 0, 0});
	
	for (int dx = -width; dx <= width; dx++) {		//Здесь нужен пробег по всем плоскостям, без рёбер и без углов
		for (int dy = -1; dy <= height; dy++) {
			for (int dz = -width; dz <= width; dz++) {
				if (!(dx == -width || dx == width
					  || dz == -width || dz == width
					  || dy == -1 || dy == height)) continue;
				if (chunk.extendedAt(
						(int) lroundf(prevPosition.x - (float) chunkLocationZero.x + (float) dx),
						(int) lroundf(prevPosition.y - (float) chunkLocationZero.y + (float) dy),
						(int) lroundf(prevPosition.z - (float) chunkLocationZero.z + (float) dz)
				).shaderProvider() == nullptr) {
					continue;
				}
				if ((dx > 0) && (dy > -1) && (dy < height) && (dz > -width) && (dz < width)) {              //x
					if (position.x > round(prevPosition.x) + padding) {
						position.x = round(prevPosition.x) + padding;
					}
				} else if ((dx < 0) && (dy > -1) && (dy < height) && (dz > -width) && (dz < width)) {      //-x
					if (position.x < round(prevPosition.x) - padding) {
						position.x = round(prevPosition.x) - padding;
					}
				}  else if ((dy > 0) && (dx > -width) && (dx < width) && (dz > -width) && (dz < width)) {        //y
					if (position.y > round(prevPosition.y) + paddingY) {
						position.y = round(prevPosition.y) + paddingY;
					}
				}  else if ((dy < 0) && (dx > -width) && (dx < width) && (dz > -width) && (dz < width)) {        //-y
					if (position.y < round(prevPosition.y) - paddingY) {
						position.y = round(prevPosition.y) - paddingY;
					}
				}  else if ((dz > 0) && (dx > -width) && (dx < width) && (dy > -1) && (dy < height)) {        //z
					if (position.z > round(prevPosition.z) + padding) {
						position.z = round(prevPosition.z) + padding;
					}
				} else if ((dz < 0) && (dx > -width) && (dx < width) && (dy > -1) && (dy < height)) {        //-z
					if (position.z < round(prevPosition.z) - padding) {
						position.z = round(prevPosition.z) - padding;
					}
				}
			}
		}
	}
	for (int dx = -width; dx <= width; dx++) {			//Тут нужен пробег только по рёбрам (без без самих углов)
		for (int dy = -1; dy <= height; dy++) {			//Переносить всё в один цикл нельзя!
			for (int dz = -width; dz <= width; dz++) {
				if (!(dx == -width || dx == width
					  || dz == -width || dz == width
					  || dy == -1 || dy == height)) continue;
				if (chunk.extendedAt(
						(int) lroundf(prevPosition.x - (float) chunkLocationZero.x + (float) dx),
						(int) lroundf(prevPosition.y - (float) chunkLocationZero.y + (float) dy),
						(int) lroundf(prevPosition.z - (float) chunkLocationZero.z + (float) dz)
				).shaderProvider() == nullptr) {
					continue;
				}
				constraintMovementAxis(
						dx, dy, dz,
						position.x, position.y, position.z,
						prevPosition.x, prevPosition.y, prevPosition.z,
						padding, paddingY, padding,
						-1, height
				);
				constraintMovementAxis(
						dx, dz, dy,
						position.x, position.z, position.y,
						prevPosition.x, prevPosition.z, prevPosition.y,
						padding, padding, paddingY,
						-width, width
				);
				constraintMovementAxis(
						dy, dx, dz,
						position.y, position.x, position.z,
						prevPosition.y, prevPosition.x, prevPosition.z,
						paddingY, padding, padding,
						-width, width
				);
			}
		}
	}
	for (int dx = -width; dx <= width; dx += 2 * width) {			//Тут только по 8 углам, сделано
		for (int dy = -1; dy <= height; dy += height + 1) {
			for (int dz = -width; dz <= width; dz += 2 * width) {
				//if (dx == 0 && dy == 0 && dz == 0) continue;
				if (chunk.extendedAt(
					(int) lroundf(prevPosition.x - (float) chunkLocationZero.x + (float) dx),
					(int) lroundf(prevPosition.y - (float) chunkLocationZero.y + (float) dy),
					(int) lroundf(prevPosition.z - (float) chunkLocationZero.z + (float) dz)
				).shaderProvider() == nullptr) {
					continue;
				}
				if ((dx > 0) && (dy > 0) && (dz > 0)) {        //x y z
					if ((position.x > roundf(prevPosition.x) + padding)
						&& (position.y > roundf(prevPosition.y) + paddingY)
						&& (position.z > roundf(prevPosition.z) + padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) + padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) + paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) + padding;
						}
					}
				} else if ((dx > 0) && (dy > 0) && (dz < 0)) {        //x y -z
					if ((position.x > roundf(prevPosition.x) + padding)
						&& (position.y > roundf(prevPosition.y) + paddingY)
						&& (position.z < roundf(prevPosition.z) - padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) + padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) + paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) - padding;
						}
					}
				} else if ((dx > 0) && (dy < 0) && (dz > 0)) {        //x -y z
					if ((position.x > roundf(prevPosition.x) + padding)
						&& (position.y < roundf(prevPosition.y) - paddingY)
						&& (position.z > roundf(prevPosition.z) + padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) + padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) - paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) + padding;
						}
					}
				} else if ((dx < 0) && (dy > 0) && (dz > 0)) {        //-x y z
					if ((position.x < roundf(prevPosition.x) - padding)
						&& (position.y > roundf(prevPosition.y) + paddingY)
						&& (position.z > roundf(prevPosition.z) + padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) - padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) + paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) + padding;
						}
					}
				} else if ((dx > 0) && (dy < 0) && (dz < 0)) {        //x -y -z
					if ((position.x > roundf(prevPosition.x) + padding)
						&& (position.y < roundf(prevPosition.y) - paddingY)
						&& (position.z < roundf(prevPosition.z) - padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) + padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) - paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) - padding;
						}
					}
				} else if ((dx < 0) && (dy < 0) && (dz > 0)) {        //-x -y z
					if ((position.x < roundf(prevPosition.x) - padding)
						&& (position.y < roundf(prevPosition.y) - paddingY)
						&& (position.z > roundf(prevPosition.z) + padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) - padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) - paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) + padding;
						}
					}
				} else if ((dx < 0) && (dy > 0) && (dz < 0)) {        //-x y -z
					if ((position.x < roundf(prevPosition.x) - padding)
						&& (position.y > roundf(prevPosition.y) + paddingY)
						&& (position.z < roundf(prevPosition.z) - padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) - padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) + paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) - padding;
						}
					}
				} else if ((dx < 0) && (dy < 0) && (dz < 0)) {        //-x -y -z
					if ((position.x < roundf(prevPosition.x) - padding)
						&& (position.y < roundf(prevPosition.y) - paddingY)
						&& (position.z < roundf(prevPosition.z) - padding)) {
						float delX = abs(position.x - roundf(prevPosition.x));
						float delY = abs(position.y - roundf(prevPosition.y));
						float delZ = abs(position.z - roundf(prevPosition.z));
						//printf("%f %f %f\n", delX, delY, delZ);
						if ((delX <= delY) && (delX <= delZ)) {
							position.x = roundf(prevPosition.x) - padding;
						} else if ((delY <= delX) && (delY <= delZ)) {
							position.y = roundf(prevPosition.y) - paddingY;
						} else if ((delZ <= delX) && (delZ <= delY)) {
							position.z = roundf(prevPosition.z) - padding;
						}
					}
				}
			}
		}
	}
}
