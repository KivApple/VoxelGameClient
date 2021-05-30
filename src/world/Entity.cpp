#include <glm/gtc/matrix_transform.hpp>
#include "Entity.h"
#include "world/VoxelWorld.h"

Entity::Entity(
		EntityTypeInterface &type,
		const VoxelLocation &location,
		const EntityOrientation &orientation
): Entity(type, location.chunk(), location.inChunk().toVec3(), orientation) {
}

Entity::Entity(
		EntityTypeInterface &type,
		const VoxelChunkLocation &chunkLocation,
		const glm::vec3 &inChunkLocation,
		const EntityOrientation &orientation
): m_type(type), m_chunkLocation(chunkLocation), m_inChunkLocation(inChunkLocation), m_orientation(orientation) {
}

void Entity::applyMovementConstraint(const glm::vec3 &prevPosition, const VoxelChunkExtendedRef &chunk) {
	physics().applyMovementConstraint(m_inChunkLocation, prevPosition, chunk);
}

glm::vec3 Entity::direction(bool usePitch) const {
	glm::vec3 direction(0.0f);
	direction.x = cosf(glm::radians(m_orientation.yaw));
	direction.y = 0.0f;
	direction.z = sinf(glm::radians(m_orientation.yaw));
	if (usePitch) {
		direction.x *= cosf(glm::radians(m_orientation.pitch));
		direction.y = sinf(glm::radians(m_orientation.pitch));
		direction.z *= cosf(glm::radians(m_orientation.pitch));
	}
	return direction;
}

glm::vec3 Entity::upDirection() const {
	return glm::vec3(0.0f, 1.0f, 0.0f); // TODO: Use orientation.roll
}

glm::vec3 Entity::position() const {
	return glm::vec3(
			m_chunkLocation.x * VOXEL_CHUNK_SIZE,
			m_chunkLocation.y * VOXEL_CHUNK_SIZE,
			m_chunkLocation.z * VOXEL_CHUNK_SIZE
	) + m_inChunkLocation;
}

VoxelChunkRef Entity::chunk(VoxelWorld &world, bool load) {
	while (true) {
		std::shared_lock<std::shared_mutex> sharedLock(m_chunkLocationMutex);
		auto prevChunkLocation = m_chunkLocation;
		sharedLock.unlock();
		auto chunk = world.chunk(
				prevChunkLocation,
				load ? VoxelWorld::MissingChunkPolicy::LOAD : VoxelWorld::MissingChunkPolicy::NONE
		);
		if (chunk.location() != m_chunkLocation) continue;
		return chunk;
	}
}

VoxelChunkExtendedRef Entity::extendedChunk(VoxelWorld &world, bool load) {
	while (true) {
		std::shared_lock<std::shared_mutex> sharedLock(m_chunkLocationMutex);
		auto prevChunkLocation = m_chunkLocation;
		sharedLock.unlock();
		auto chunk = world.extendedChunk(
				prevChunkLocation,
				load ? VoxelWorld::MissingChunkPolicy::LOAD : VoxelWorld::MissingChunkPolicy::NONE
		);
		if (chunk.location() != m_chunkLocation) continue;
		return chunk;
	}
}

VoxelChunkMutableRef Entity::mutableChunk(VoxelWorld &world, bool load) {
	while (true) {
		std::shared_lock<std::shared_mutex> sharedLock(m_chunkLocationMutex);
		auto prevChunkLocation = m_chunkLocation;
		sharedLock.unlock();
		auto chunk = world.mutableChunk(
				prevChunkLocation,
				load ? VoxelWorld::MissingChunkPolicy::LOAD : VoxelWorld::MissingChunkPolicy::NONE
		);
		if (chunk.location() != m_chunkLocation) continue;
		return chunk;
	}
}

VoxelChunkExtendedMutableRef Entity::extendedMutableChunk(VoxelWorld &world, bool load) {
	while (true) {
		std::shared_lock<std::shared_mutex> sharedLock(m_chunkLocationMutex);
		auto prevChunkLocation = m_chunkLocation;
		sharedLock.unlock();
		auto chunk = world.extendedMutableChunk(
				prevChunkLocation,
				load ? VoxelWorld::MissingChunkPolicy::LOAD : VoxelWorld::MissingChunkPolicy::NONE
		);
		if (chunk.location() != m_chunkLocation) continue;
		return chunk;
	}
}

void Entity::setPosition(VoxelChunkExtendedMutableRef &chunk, const glm::vec3 &position) {
	VoxelLocation location(
			(int) roundf(position.x),
			(int) roundf(position.y),
			(int) roundf(position.z)
	);
	auto chunkLocation = location.chunk();
	m_inChunkLocation = position - glm::vec3(
			(float) (chunkLocation.x * VOXEL_CHUNK_SIZE),
			(float) (chunkLocation.y * VOXEL_CHUNK_SIZE),
			(float) (chunkLocation.z * VOXEL_CHUNK_SIZE)
	);
	if (chunkLocation != m_chunkLocation) {
		chunk.removeEntity(this);
		int dx = chunkLocation.x - m_chunkLocation.x;
		int dy = chunkLocation.y - m_chunkLocation.y;
		int dz = chunkLocation.z - m_chunkLocation.z;
		{
			std::unique_lock<std::shared_mutex> lock(m_chunkLocationMutex);
			m_chunkLocation = chunkLocation;
		}
		if (abs(dx) <= 1 && abs(dy) <= 1 && abs(dz) <= 1 && chunk.hasNeighbor(dx, dy, dz)) {
			chunk.extendedAddEntity(this);
		} else {
			auto &world = chunk.world();
			chunk.unlock();
			chunk = world.extendedMutableChunk(chunkLocation, VoxelWorld::MissingChunkPolicy::LOAD);
			chunk.addEntity(this);
		}
	} else {
		// TODO
	}
}

void Entity::setRotation(float yaw, float pitch) {
	m_orientation.yaw = yaw;
	m_orientation.pitch = pitch;
}

void Entity::adjustRotation(float dYaw, float dPitch) {
	m_orientation.yaw += dYaw;
	m_orientation.pitch += dPitch;
	
	while (m_orientation.yaw < -360.0f) {
		m_orientation.yaw += 360.0f;
	}
	while (m_orientation.yaw > 360.0f) {
		m_orientation.yaw -= 360.0f;
	}
	
	if (m_orientation.pitch < -89.0f) {
		m_orientation.pitch = -89.0f;
	}
	if (m_orientation.pitch > 89.0f) {
		m_orientation.pitch = 89.0f;
	}
}

void Entity::move(VoxelWorld &world, const glm::vec3 &delta) {
	std::shared_lock<std::shared_mutex> sharedLock(m_chunkLocationMutex);
	auto prevChunkLocation = m_chunkLocation;
	sharedLock.unlock();
	auto chunk = world.extendedMutableChunk(prevChunkLocation);
	if (!chunk) return;
	move(chunk, delta);
}

void Entity::move(VoxelChunkExtendedMutableRef &chunk, const glm::vec3 &delta) {
	auto direction = this->direction(false);
	auto prevPosition = m_inChunkLocation;
	m_inChunkLocation += (direction * delta.z + glm::normalize(glm::cross(direction, upDirection())) * delta.x) +
			glm::vec3(0.0f, delta.y, 0.0f);
	applyMovementConstraint(prevPosition, chunk);
	updateContainedChunk(chunk);
}

void Entity::update(VoxelChunkExtendedMutableRef &chunk) {
	auto prevPosition = m_inChunkLocation;
	m_type.invokeUpdate(chunk, *this);
	applyMovementConstraint(prevPosition, chunk);
	updateContainedChunk(chunk);
}

void Entity::updateContainedChunk(VoxelChunkExtendedMutableRef &chunk) {
	InChunkVoxelLocation inChunkLocation(m_inChunkLocation);
	if (
			inChunkLocation.x >= 0 && inChunkLocation.x < VOXEL_CHUNK_SIZE &&
			inChunkLocation.y >= 0 && inChunkLocation.y < VOXEL_CHUNK_SIZE &&
			inChunkLocation.z >= 0 && inChunkLocation.z < VOXEL_CHUNK_SIZE
	) return;
	chunk.removeEntity(this);
	std::unique_lock<std::shared_mutex> lock(m_chunkLocationMutex);
	if (inChunkLocation.x < 0) {
		m_chunkLocation.x--;
		m_inChunkLocation.x += VOXEL_CHUNK_SIZE;
	} else if (inChunkLocation.x >= VOXEL_CHUNK_SIZE) {
		m_chunkLocation.x++;
		m_inChunkLocation.x -= VOXEL_CHUNK_SIZE;
	}
	if (inChunkLocation.y < 0) {
		m_chunkLocation.y--;
		m_inChunkLocation.y += VOXEL_CHUNK_SIZE;
	} else if (inChunkLocation.y >= VOXEL_CHUNK_SIZE) {
		m_chunkLocation.y++;
		m_inChunkLocation.y -= VOXEL_CHUNK_SIZE;
	}
	if (inChunkLocation.z < 0) {
		m_chunkLocation.z--;
		m_inChunkLocation.z += VOXEL_CHUNK_SIZE;
	} else if (inChunkLocation.z >= VOXEL_CHUNK_SIZE) {
		m_chunkLocation.z++;
		m_inChunkLocation.z -= VOXEL_CHUNK_SIZE;
	}
	lock.unlock();
	chunk.extendedAddEntity(this);
}

SimpleEntityType::SimpleEntityType(
		const EntityPhysics &physics
#ifndef HEADLESS
		, const Model *model
#endif
): m_physics(physics)
#ifndef HEADLESS
, m_model(model)
#endif
{
}

const EntityPhysics &SimpleEntityType::physics(const Entity &entity, const State &state) {
	return m_physics;
}

#ifndef HEADLESS
void SimpleEntityType::render(
		const Entity &entity,
		const State &state,
		const glm::mat4 &view,
		const glm::mat4 &projection
) {
	if (m_model == nullptr) return;
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, entity.position());
	m_model->render(transform, view, projection);
}
#endif
