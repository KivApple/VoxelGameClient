#include <chrono>
#include <thread>
#include "GameServerEngine.h"

GameServerEngine::GameServerEngine() {
	m_voxelTypeRegistry.add("grass", std::make_unique<SimpleVoxelType>("grass", "assets/textures/grass_top.png"));
	m_voxelTypeRegistry.add("dirt", std::make_unique<SimpleVoxelType>("dirt", "assets/textures/mud.png"));
	{
		auto &grass = m_voxelTypeRegistry.get("grass");
		auto chunk = m_voxelWorld.mutableChunk({0, 0, 0}, true);
		chunk.at(0, 0, 0).setType(grass);
		chunk.at(0, 1, 0).setType(grass);
		chunk.at(0, 0, 1).setType(grass);
	}
	{
		auto &grass = m_voxelTypeRegistry.get("grass");
		auto &dirt = m_voxelTypeRegistry.get("dirt");
		auto chunk = m_voxelWorld.mutableChunk({0, -1, 0}, true);
		for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
			for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
				chunk.at(x, VOXEL_CHUNK_SIZE - 1, z).setType(grass);
				chunk.at(x, VOXEL_CHUNK_SIZE - 2, z).setType(dirt);
				chunk.at(x, VOXEL_CHUNK_SIZE - 3, z).setType(dirt);
			}
		}
		chunk.at(3, VOXEL_CHUNK_SIZE - 3, 3).setType(m_voxelTypeRegistry.get("invalid"));
	}
}

void GameServerEngine::addTransport(std::unique_ptr<ServerTransport> transport) {
	m_transports.emplace_back(std::move(transport));
}

int GameServerEngine::run() {
	for (auto &&transport : m_transports) {
		transport->start(*this);
	}
	while (m_running) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	for (auto &&transport : m_transports) {
		transport->shutdown();
	}
	std::unique_lock<std::shared_mutex> lock(m_connectionsMutex);
	m_connections.clear();
	return 0;
}

void GameServerEngine::shutdown() {
	m_running = false;
}

void GameServerEngine::registerConnection(std::unique_ptr<ClientConnection> connection) {
	std::unique_lock<std::shared_mutex> lock(m_connectionsMutex);
	auto key = connection.get();
	m_connections.emplace(key, std::move(connection));
}

void GameServerEngine::unregisterConnection(ClientConnection *connection) {
	std::unique_lock<std::shared_mutex> lock(m_connectionsMutex);
	m_connections.erase(connection);
}
