#include <chrono>
#include <thread>
#include "GameServerEngine.h"

GameServerEngine::GameServerEngine(
): m_voxelWorldGenerator(m_voxelTypeRegistry), m_voxelWorld(&m_voxelWorldGenerator, this) {
}

void GameServerEngine::addTransport(std::unique_ptr<ServerTransport> transport) {
	m_transports.emplace_back(std::move(transport));
}

void GameServerEngine::chunkInvalidated(
		const VoxelChunkLocation &chunkLocation,
		std::vector<InChunkVoxelLocation> &&locations,
		bool lightComputed
) {
	if (lightComputed) {
		std::shared_lock<std::shared_mutex> lock(m_connectionsMutex);
		for (auto &connection : m_connections) {
			connection.second->chunkInvalidated(chunkLocation);
		}
	} else {
		if (locations.empty()) {
			m_voxelLightComputer.computeAsync(m_voxelWorld, chunkLocation);
		} else {
			m_voxelLightComputer.computeAsync(m_voxelWorld, chunkLocation, std::move(locations));
		}
	}
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
