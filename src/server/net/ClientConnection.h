#pragma once

#include <unordered_set>
#include <shared_mutex>
#include <glm/vec3.hpp>
#include "world/VoxelLocation.h"
#include "ServerTransport.h"

class VoxelChunkRef;

class ClientConnection {
	ServerTransport &m_transport;
	
	glm::vec3 m_position = glm::vec3(0.0f);
	float m_yaw = 0.0f;
	float m_pitch = 0.0f;
	int m_viewRadius = 0;
	bool m_positionValid = false;
	std::shared_mutex m_positionMutex;
	std::unordered_set<VoxelChunkLocation> m_loadedChunks;
	
	void sendUnloadedChunks(const glm::vec3 &position, int viewRadius);
	
public:
	explicit ClientConnection(ServerTransport &transport): m_transport(transport) {
	}
	virtual ~ClientConnection() = default;
	[[nodiscard]] ServerTransport &transport() const {
		return m_transport;
	}
	void updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius);
	virtual void setPosition(const glm::vec3 &position) = 0;
	virtual void setChunk(const VoxelChunkRef &chunk) = 0;
	
};
