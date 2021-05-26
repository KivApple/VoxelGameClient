#include <cmath>
#include "ClientConnection.h"
#include "server/GameServerEngine.h"
#include "world/VoxelWorldUtils.h"

ClientConnection::ClientConnection(ServerTransport &transport): m_transport(transport), m_player(
		transport.engine()->voxelWorld(),
		glm::vec3(),
		0.0f,
		0.0f,
		1,
		2,
		0.25f,
		0.05f
) {
	m_logger = el::Loggers::getLogger("default");
	m_inventory.resize(8);
	m_inventory[0].setType(transport.engine()->voxelTypeRegistry().get("grass"));
	m_inventory[1].setType(transport.engine()->voxelTypeRegistry().get("dirt"));
	m_inventory[2].setType(transport.engine()->voxelTypeRegistry().get("stone"));
	m_inventory[3].setType(transport.engine()->voxelTypeRegistry().get("glass"));
	m_inventory[4].setType(transport.engine()->voxelTypeRegistry().get("lava"));
	m_inventory[5].setType(transport.engine()->voxelTypeRegistry().get("water"));
}

void ClientConnection::updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	/* logger().trace(
			"[Client %v] updatePosition(x=%v, y=%v, z=%v, yaw=%v, pitch=%v, viewRadius=%v)",
			this, position.x, position.y, position.z, yaw, pitch, viewRadius
	); */
	std::unique_lock<std::shared_mutex> lock(m_positionMutex);
	bool resetPosition = false;
	if (m_positionValid) {
		std::chrono::duration<float> dt = std::chrono::steady_clock::now() - m_lastPositionUpdatedAt;
		auto delta = (position - m_player.position()) / dt.count();
		static const float MAX_DELTA = 20.0f;
		if (fabsf(delta.x) >= MAX_DELTA || fabsf(delta.y) >= MAX_DELTA || fabsf(delta.z) >= MAX_DELTA) {
			logger().warn(
					"[Client %v] Player is moving too fast (dx=%v,dy=%v,dz=%v)",
					this, delta.x, delta.y, delta.z
			);
			//resetPosition = true;
		}
	}
	bool chunkChanged = false;
	if (!resetPosition) {
		m_player.setPosition(position);
		auto positionChunk = VoxelLocation(
				(int) roundf(position.x),
				(int) roundf(position.y),
				(int) roundf(position.z)
		).chunk();
		if (!m_positionValid || positionChunk != m_positionChunk) {
			m_positionChunk = positionChunk;
			chunkChanged = true;
		}
	}
	m_player.setRotation(yaw, pitch);
	if (viewRadius > 3) {
		logger().warn("[Client %v] Requested too large view radius. Value will be reduced to 3");
		m_viewRadius = 3;
	} else {
		m_viewRadius = viewRadius;
	}
	m_lastPositionUpdatedAt = std::chrono::steady_clock::now();
	m_positionValid = true;
	auto newPosition = m_player.position();
	auto radius = m_viewRadius;
	auto positionChunk = m_positionChunk;
	lock.unlock();
	if (resetPosition) {
		setPosition(newPosition);
	}
	if (chunkChanged) {
		handleChunkChanged(positionChunk, radius);
	}
}

void ClientConnection::handleChunkChanged(const VoxelChunkLocation &location, int viewRadius) {
	std::unique_lock<std::mutex> lock(m_pendingChunksMutex);
	m_pendingChunks.clear();
	bool foundUnloadedChunks = false;
	for (int dz = -viewRadius; dz <= viewRadius; dz++) {
		for (int dy = -viewRadius; dy <= viewRadius; dy++) {
			for (int dx = -viewRadius; dx <= viewRadius; dx++) {
				VoxelChunkLocation l(location.x + dx, location.y + dy, location.z + dz);
				if (m_loadedChunks.find(l) == m_loadedChunks.end()) {
					m_pendingChunks.emplace(l);
					foundUnloadedChunks = true;
				}
			}
		}
	}
	std::vector<VoxelChunkLocation> discardedChunks;
	auto it = m_loadedChunks.begin();
	while (it != m_loadedChunks.end() && discardedChunks.size() < 65535) {
		int dx = it->x - location.x;
		int dy = it->y - location.y;
		int dz = it->z - location.z;
		if (
				dx < -(viewRadius + 1) || dx > (viewRadius + 1) ||
				dy < -(viewRadius + 1) || dy > (viewRadius + 1) ||
				dz < -(viewRadius + 1) || dz > (viewRadius + 1)
				) {
			discardedChunks.emplace_back(*it);
			it = m_loadedChunks.erase(it);
		} else {
			++it;
		}
	}
	lock.unlock();
	if (foundUnloadedChunks) {
		newPendingChunk();
	}
	if (!discardedChunks.empty()) {
		logger().debug("[Client %v] Discarding %v chunk(s)", this, discardedChunks.size());
		discardChunks(discardedChunks);
	}
}

bool ClientConnection::setPendingChunk() {
	glm::vec3 position;
	{
		std::shared_lock<std::shared_mutex> sharedLock(m_positionMutex);
		position = m_player.position();
	}
	VoxelChunkRef chunk;
	do {
		std::unique_lock<std::mutex> lock(m_pendingChunksMutex);
		auto nearestIt = m_pendingChunks.end();
		float nearestDistance = INFINITY;
		for (auto it = m_pendingChunks.begin(); it != m_pendingChunks.end(); ++it) {
			float dx = (float) it->x - floorf(roundf(position.x) / VOXEL_CHUNK_SIZE);
			float dy = (float) it->y - floorf(roundf(position.y) / VOXEL_CHUNK_SIZE);
			float dz = (float) it->z - floorf(roundf(position.z) / VOXEL_CHUNK_SIZE);
			float distance = dx * dx + dy * dy + dz * dz;
			if (distance < nearestDistance) {
				nearestDistance = distance;
				nearestIt = it;
			}
		}
		if (nearestIt == m_pendingChunks.end()) {
			return false;
		}
		auto location = *nearestIt;
		m_pendingChunks.erase(nearestIt);
		m_loadedChunks.emplace(location);
		lock.unlock();
		chunk = transport().engine()->voxelWorld().chunk(location, VoxelWorld::MissingChunkPolicy::LOAD_ASYNC);
		if (chunk && chunk.lightState() != VoxelChunkLightState::COMPLETE) {
			chunk.unlock();
		}
		transport().engine()->voxelLightComputer().computeAsync(transport().engine()->voxelWorld(), location);
	} while (!chunk);
	setChunk(chunk);
	return true;
}

void ClientConnection::chunkInvalidated(const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_pendingChunksMutex);
	if (m_loadedChunks.find(location) == m_loadedChunks.end()) {
		return;
	}
	m_pendingChunks.emplace(location);
	lock.unlock();
	newPendingChunk();
}

std::pair<VoxelChunkLocation, int> ClientConnection::positionChunk() {
	std::shared_lock<std::shared_mutex> sharedLock(m_positionMutex);
	return std::make_pair(m_positionChunk, m_viewRadius);
}

void ClientConnection::queryInventory(std::unordered_map<uint8_t, VoxelHolder> &inventory, int &active) {
	std::shared_lock<std::shared_mutex> lock(m_inventoryMutex);
	int i = 0;
	for (auto &it : m_inventory) {
		if (&it.type() != &EmptyVoxelType::INSTANCE) {
			inventory.emplace(i, it);
		}
		i++;
	}
	active = m_activeInventoryIndex;
}

void ClientConnection::setActiveInventoryIndex(int active) {
	logger().debug("[Client %v] Selected inventory item #%v", this, active);
	std::unique_lock<std::shared_mutex> lock(m_inventoryMutex);
	m_activeInventoryIndex = active;
}

void ClientConnection::digVoxel() {
	glm::vec3 playerPosition;
	glm::vec3 playerDirection;
	{
		std::shared_lock<std::shared_mutex> sharedLock(m_positionMutex);
		playerPosition = m_player.position() + glm::vec3(
				0.0f,
				(float) m_player.height() - 0.75f - m_player.paddingY(),
				0.0f
		);
		playerDirection = m_player.direction(true);
	}
	auto chunk = transport().engine()->voxelWorld().extendedMutableChunk(VoxelLocation(
			(int) roundf(playerPosition.x),
			(int) roundf(playerPosition.y),
			(int) roundf(playerPosition.z)
	).chunk());
	auto pointingAt = findPlayerPointingAt(chunk, playerPosition, playerDirection);
	if (pointingAt.has_value()) {
		auto &location = pointingAt->location;
		logger().debug("[Client %v] Dig voxel at x=%v,y=%v,z=%v", this, location.x, location.y, location.z);
		chunk.extendedAt(pointingAt->inChunkLocation).setType(transport().engine()->voxelTypeRegistry().get("air"));
		chunk.extendedMarkDirty(pointingAt->inChunkLocation);
	} else {
		logger().warn("[Client %v] Attempt to dig empty voxel", this);
	}
}

void ClientConnection::placeVoxel() {
	glm::vec3 playerPosition;
	glm::vec3 playerDirection;
	{
		std::shared_lock<std::shared_mutex> sharedLock(m_positionMutex);
		playerPosition = m_player.position() + glm::vec3(
				0.0f,
				(float) m_player.height() - 0.75f - m_player.paddingY(),
				0.0f
		);
		playerDirection = m_player.direction(true);
	}
	auto chunk = transport().engine()->voxelWorld().extendedMutableChunk(VoxelLocation(
			(int) roundf(playerPosition.x),
			(int) roundf(playerPosition.y),
			(int) roundf(playerPosition.z)
	).chunk());
	auto pointingAt = findPlayerPointingAt(chunk, playerPosition, playerDirection);
	if (pointingAt.has_value()) {
		std::shared_lock<std::shared_mutex> lock(m_inventoryMutex);
		auto &item = m_inventory[m_activeInventoryIndex];
		if (&item.type() == &EmptyVoxelType::INSTANCE) return;
		VoxelLocation location(
				pointingAt->location.x + (int) pointingAt->direction.x,
				pointingAt->location.y + (int) pointingAt->direction.y,
				pointingAt->location.z + (int) pointingAt->direction.z
		);
		InChunkVoxelLocation inChunkLocation(
				pointingAt->inChunkLocation.x + (int) pointingAt->direction.x,
				pointingAt->inChunkLocation.y + (int) pointingAt->direction.y,
				pointingAt->inChunkLocation.z + (int) pointingAt->direction.z
		);
		logger().debug("[Client %v] Place voxel at x=%v,y=%v,z=%v", this, location.x, location.y, location.z);
		chunk.extendedAt(inChunkLocation) = m_inventory[m_activeInventoryIndex];
		chunk.extendedMarkDirty(inChunkLocation);
	} else {
		logger().warn("[Client %v] Attempt to place voxel at void", this);
	}
}
