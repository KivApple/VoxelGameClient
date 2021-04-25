#pragma once

#include <atomic>
#include <vector>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include "world/VoxelWorld.h"
#include "world/VoxelTypeRegistry.h"
#include "net/ServerTransport.h"
#include "net/ClientConnection.h"

class GameServerEngine {
	VoxelTypeRegistry m_voxelTypeRegistry;
	VoxelWorld m_voxelWorld;
	std::vector<std::unique_ptr<ServerTransport>> m_transports;
	std::unordered_map<ClientConnection*, std::unique_ptr<ClientConnection>> m_connections;
	std::shared_mutex m_connectionsMutex;
	std::atomic<bool> m_running = true;
	
public:
	void addTransport(std::unique_ptr<ServerTransport> transport);
	int run();
	void shutdown();
	
	VoxelTypeRegistry &voxelTypeRegistry() {
		return m_voxelTypeRegistry;
	}
	VoxelWorld &voxelWorld() {
		return m_voxelWorld;
	}
	
	void registerConnection(std::unique_ptr<ClientConnection> connection);
	void unregisterConnection(ClientConnection *connection);

};
