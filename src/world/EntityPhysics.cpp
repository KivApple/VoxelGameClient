#include <cmath>
#include "VoxelWorld.h"
#include "EntityPhysics.h"

const EntityPhysics EntityPhysics::NONE;

EntityPhysics::EntityPhysics(
): m_type(EntityPhysicsType::NONE), m_width(1), m_height(1), m_padding(1.0f), m_paddingY(1.0f) {
}

EntityPhysics::EntityPhysics(
		EntityPhysicsType type,
		int width,
		int height,
		float padding,
		float paddingY
): m_type(type), m_width(width), m_height(height), m_padding(padding), m_paddingY(paddingY) {
}

void EntityPhysics::applyMovementConstraint(
		glm::vec3 &position,
		const glm::vec3 &prevPosition,
		const VoxelChunkExtendedRef &chunk
) const {
	if (m_type != EntityPhysicsType::DYNAMIC) return;
	applyMovementConstraint(position, prevPosition, m_width, m_height, m_padding, m_paddingY, chunk);
}

void EntityPhysics::constraintMovementAxis(
		int dx, int dy, int dz,
		float &a, float &b, float &c,
		float prevA, float prevB, float prevC,
		float paddingA, float paddingB, float paddingC, int start_, int end_
) {
	(void) prevB;
	(void) b;
	(void) paddingB;
	
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

void EntityPhysics::applyMovementConstraint(
		glm::vec3 &position, const glm::vec3 &prevPosition,
		int width, int height, float padding, float paddingY,
		const VoxelChunkExtendedRef &chunk
) {
	(void) paddingY;
	
	//VoxelLocation chunkLocationZero(chunk.location(), {0, 0, 0});
	
	for (int dx = -width; dx <= width; dx++) {		//Здесь нужен пробег по всем плоскостям, без рёбер и без углов
		for (int dy = -1; dy <= height; dy++) {
			for (int dz = -width; dz <= width; dz++) {
				if (!(dx == -width || dx == width
					  || dz == -width || dz == width
					  || dy == -1 || dy == height)) continue;
				if (!chunk.extendedAt(
						(int) lroundf(prevPosition.x /* - (float) chunkLocationZero.x */ + (float) dx),
						(int) lroundf(prevPosition.y /* - (float) chunkLocationZero.y */ + (float) dy),
						(int) lroundf(prevPosition.z /* - (float) chunkLocationZero.z */ + (float) dz)
				).hasDensity()) {
					continue;
				}
				if ((dx > 0) && (dy > -1) && (dy < height) && (dz > -width) && (dz < width)) {              //x
					if (position.x > roundf(prevPosition.x) + padding) {
						position.x = roundf(prevPosition.x) + padding;
					}
				} else if ((dx < 0) && (dy > -1) && (dy < height) && (dz > -width) && (dz < width)) {      //-x
					if (position.x < roundf(prevPosition.x) - padding) {
						position.x = roundf(prevPosition.x) - padding;
					}
				}  else if ((dy > 0) && (dx > -width) && (dx < width) && (dz > -width) && (dz < width)) {        //y
					if (position.y > roundf(prevPosition.y) + paddingY) {
						position.y = roundf(prevPosition.y) + paddingY;
					}
				}  else if ((dy < 0) && (dx > -width) && (dx < width) && (dz > -width) && (dz < width)) {        //-y
					if (position.y < roundf(prevPosition.y) - paddingY) {
						position.y = roundf(prevPosition.y) - paddingY;
					}
				}  else if ((dz > 0) && (dx > -width) && (dx < width) && (dy > -1) && (dy < height)) {        //z
					if (position.z > roundf(prevPosition.z) + padding) {
						position.z = roundf(prevPosition.z) + padding;
					}
				} else if ((dz < 0) && (dx > -width) && (dx < width) && (dy > -1) && (dy < height)) {        //-z
					if (position.z < roundf(prevPosition.z) - padding) {
						position.z = roundf(prevPosition.z) - padding;
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
				if (!chunk.extendedAt(
						(int) lroundf(prevPosition.x /* - (float) chunkLocationZero.x */ + (float) dx),
						(int) lroundf(prevPosition.y /* - (float) chunkLocationZero.y */ + (float) dy),
						(int) lroundf(prevPosition.z /* - (float) chunkLocationZero.z */ + (float) dz)
				).hasDensity()) {
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
				if (!chunk.extendedAt(
						(int) lroundf(prevPosition.x /* - (float) chunkLocationZero.x */ + (float) dx),
						(int) lroundf(prevPosition.y /* - (float) chunkLocationZero.y */ + (float) dy),
						(int) lroundf(prevPosition.z /* - (float) chunkLocationZero.z */ + (float) dz)
				).hasDensity()) {
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
