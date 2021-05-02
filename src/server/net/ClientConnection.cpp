#include <cmath>
#include "ClientConnection.h"
#include "server/GameServerEngine.h"

ClientConnection::ClientConnection(ServerTransport &transport): m_transport(transport) {
	m_logger = el::Loggers::getLogger("default");
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
		auto delta = (position - m_position) / dt.count();
		static const float MAX_DELTA = 20.0f;
		if (fabsf(delta.x) >= MAX_DELTA || fabsf(delta.y) >= MAX_DELTA || fabsf(delta.z) >= MAX_DELTA) {
			logger().warn(
					"[Client %v] Player is moving too fast (dx=%v,dy=%v,dz=%v)",
					this, delta.x, delta.y, delta.z
			);
			resetPosition = true;
		}
	}
	bool chunkChanged = false;
	if (!resetPosition) {
		m_position = position;
		VoxelChunkLocation positionChunk(
				(int) roundf(position.x) / VOXEL_CHUNK_SIZE,
				(int) roundf(position.y) / VOXEL_CHUNK_SIZE,
				(int) roundf(position.z) / VOXEL_CHUNK_SIZE
		);
		if (!m_positionValid || positionChunk != m_positionChunk) {
			m_positionChunk = positionChunk;
			chunkChanged = true;
		}
	}
	m_yaw = yaw;
	m_pitch = pitch;
	if (viewRadius > 3) {
		logger().warn("[Client %v] Requested too large view radius. Value will be reduced to 3");
		m_viewRadius = 3;
	} else {
		m_viewRadius = viewRadius;
	}
	m_lastPositionUpdatedAt = std::chrono::steady_clock::now();
	m_positionValid = true;
	auto newPosition = m_position;
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
		position = m_position;
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
		if (chunk && !chunk.lightComputed()) {
			chunk.unlock(false);
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

void ClientConnection::digVoxel(const VoxelLocation &location) {
	logger().debug("[Client %v] Dig voxel at x=%v,y=%v,z=%v", this, location.x, location.y, location.z);
	auto chunkLocation = location.chunk();
	auto chunk = transport().engine()->voxelWorld().mutableChunk(chunkLocation);
	if (!chunk) {
		logger().warn(
				"[Client %v] Attempt to dig voxel at absent chunk x=%v,y=%v,z=%v",
				this, chunkLocation.x, chunkLocation.y, chunkLocation.z
		);
		return;
	}
	auto l = location.inChunk();
	chunk.at(l).setType(transport().engine()->voxelTypeRegistry().get("air"));
	chunk.markDirty(l);
}

void ClientConnection::placeVoxel(const VoxelLocation &location) {
	logger().debug("[Client %v] Place voxel at x=%v,y=%v,z=%v", this, location.x, location.y, location.z);
	auto chunkLocation = location.chunk();
	auto chunk = transport().engine()->voxelWorld().mutableChunk(chunkLocation);
	if (!chunk) {
		logger().warn(
				"[Client %v] Attempt to place voxel at absent chunk x=%v,y=%v,z=%v",
				this, chunkLocation.x, chunkLocation.y, chunkLocation.z
		);
		return;
	}
	auto l = location.inChunk();
	chunk.at(l).setType(transport().engine()->voxelTypeRegistry().get("stone"));
	chunk.markDirty(l);
}
