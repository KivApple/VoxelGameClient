#pragma once

#include <chrono>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <easylogging++.h>
#include <glm/vec3.hpp>
#include "world/VoxelLocation.h"
#include "ServerTransport.h"

class VoxelChunkRef;

class ClientConnection {
	ServerTransport &m_transport;
	el::Logger *m_logger;
	
	glm::vec3 m_position = glm::vec3(0.0f);
	float m_yaw = 0.0f;
	float m_pitch = 0.0f;
	int m_viewRadius = 0;
	VoxelChunkLocation m_positionChunk;
	std::chrono::time_point<std::chrono::steady_clock> m_lastPositionUpdatedAt;
	bool m_positionValid = false;
	std::shared_mutex m_positionMutex;
	std::unordered_set<VoxelChunkLocation> m_loadedChunks;
	std::unordered_set<VoxelChunkLocation> m_pendingChunks;
	std::mutex m_pendingChunksMutex;
	
	void handleChunkChanged(const VoxelChunkLocation &location, int viewRadius);
	
protected:
	[[nodiscard]] el::Logger &logger() const {
		return *m_logger;
	}
	void updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius);
	virtual void setPosition(const glm::vec3 &position) = 0;
	virtual void setChunk(const VoxelChunkRef &chunk) = 0;
	virtual void newPendingChunk() = 0;
	bool setPendingChunk();

public:
	explicit ClientConnection(ServerTransport &transport);
	virtual ~ClientConnection() = default;
	[[nodiscard]] ServerTransport &transport() const {
		return m_transport;
	}
	void chunkInvalidated(const VoxelChunkLocation &location);
	
};
