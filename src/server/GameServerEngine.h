#pragma once

#include <atomic>
#include <vector>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include "Asset.h"
#include "world/VoxelWorld.h"
#include "world/VoxelTypeRegistry.h"
#include "world/VoxelWorldGenerator.h"
#include "world/VoxelWorldStorage.h"
#include "world/VoxelLightComputer.h"
#include "world/VoxelWorldUpdater.h"
#include "world/VoxelTypes.h"
#include "net/ServerTransport.h"
#include "net/ClientConnection.h"

class GameServerEngine: VoxelChunkListener {
	AssetLoader m_assetLoader;
	VoxelTypeRegistry m_voxelTypeRegistry;
	VoxelTypesRegistration m_voxelTypesRegistration;
	VoxelWorld m_voxelWorld;
	VoxelWorldGenerator m_voxelWorldGenerator;
	VoxelWorldStorage m_voxelWorldStorage;
	VoxelLightComputer m_voxelLightComputer;
	VoxelWorldUpdater m_voxelWorldUpdater;
	std::vector<std::unique_ptr<ServerTransport>> m_transports;
	std::unordered_map<ClientConnection*, std::unique_ptr<ClientConnection>> m_connections;
	std::shared_mutex m_connectionsMutex;
	std::atomic<bool> m_running = true;
	
	void chunkUnlocked(const VoxelChunkLocation &chunkLocation, VoxelChunkLightState lightState) override;
	
public:
	GameServerEngine();
	~GameServerEngine() override;
	void addTransport(std::unique_ptr<ServerTransport> transport);
	int run();
	void shutdown();
	
	VoxelTypeRegistry &voxelTypeRegistry() {
		return m_voxelTypeRegistry;
	}
	VoxelWorld &voxelWorld() {
		return m_voxelWorld;
	}
	VoxelLightComputer &voxelLightComputer() {
		return m_voxelLightComputer;
	}
	VoxelWorldUpdater &voxelWorldUpdater() {
		return m_voxelWorldUpdater;
	}
	
	void registerConnection(std::unique_ptr<ClientConnection> connection);
	void unregisterConnection(ClientConnection *connection);

};
