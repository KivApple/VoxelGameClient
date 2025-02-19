#pragma once

#include <chrono>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <easylogging++.h>
#include <glm/vec3.hpp>
#include "world/Voxel.h"
#include "world/VoxelLocation.h"
#include "world/Entity.h"
#include "ServerTransport.h"

class VoxelChunkRef;

class ClientConnection {
	ServerTransport &m_transport;
	el::Logger *m_logger;
	
	Entity *m_player;
	int m_viewRadius = 0;
	VoxelChunkLocation m_positionChunk;
	std::chrono::time_point<std::chrono::steady_clock> m_lastPositionUpdatedAt;
	bool m_positionValid = false;
	std::shared_mutex m_positionMutex;
	std::unordered_set<VoxelChunkLocation> m_loadedChunks;
	std::unordered_set<VoxelChunkLocation> m_pendingChunks;
	std::mutex m_pendingChunksMutex;
	std::vector<VoxelHolder> m_inventory;
	int m_activeInventoryIndex = 0;
	std::shared_mutex m_inventoryMutex;
	
	void handleChunkChanged(const VoxelChunkLocation &location, int viewRadius);
	
protected:
	[[nodiscard]] el::Logger &logger() const {
		return *m_logger;
	}
	void updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius);
	virtual void setPosition(const glm::vec3 &position) = 0;
	virtual void setChunk(const VoxelChunkRef &chunk) = 0;
	virtual void discardChunks(const std::vector<VoxelChunkLocation> &locations) = 0;
	virtual void newPendingChunk() = 0;
	bool setPendingChunk();
	void queryInventory(std::unordered_map<uint8_t, VoxelHolder> &inventory, int &active);
	virtual void inventoryUpdated(std::unordered_map<uint8_t, VoxelHolder> &&changes, int active) = 0;
	void setActiveInventoryIndex(int active);
	void digVoxel();
	void placeVoxel();

public:
	explicit ClientConnection(ServerTransport &transport);
	virtual ~ClientConnection();
	[[nodiscard]] ServerTransport &transport() const {
		return m_transport;
	}
	void chunkInvalidated(const VoxelChunkLocation &location);
	std::pair<VoxelChunkLocation, int> positionChunk();
	
};
