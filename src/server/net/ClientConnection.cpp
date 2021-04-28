#include <cmath>
#include "ClientConnection.h"
#include "server/GameServerEngine.h"

ClientConnection::ClientConnection(ServerTransport &transport): m_transport(transport) {
	m_logger = el::Loggers::getLogger("default");
}

void ClientConnection::updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	logger().trace(
			"[Client %v] updatePosition(x=%v, y=%v, z=%v, yaw=%v, pitch=%v, viewRadius=%v)",
			this, position.x, position.y, position.z, yaw, pitch, viewRadius
	);
	std::unique_lock<std::shared_mutex> lock(m_positionMutex);
	bool resetPosition = false;
	if (m_positionValid) {
		std::chrono::duration<float> dt = std::chrono::steady_clock::now() - m_lastPositionUpdatedAt;
		auto delta = (position - m_position) / dt.count();
		static const float MAX_DELTA = 10.0f;
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
				(int) position.x / VOXEL_CHUNK_SIZE,
				(int) position.y / VOXEL_CHUNK_SIZE,
				(int) position.z / VOXEL_CHUNK_SIZE
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
	lock.unlock();
	if (foundUnloadedChunks) {
		newPendingChunk();
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
			float dx = (float) it->x - position.x;
			float dy = (float) it->y - position.y;
			float dz = (float) it->z - position.z;
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
		chunk = transport().engine()->voxelWorld().chunk(location);
	} while (!chunk);
	setChunk(chunk);
	return true;
}
