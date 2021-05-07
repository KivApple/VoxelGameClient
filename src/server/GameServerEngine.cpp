#include <chrono>
#include <thread>
#include "GameServerEngine.h"

GameServerEngine::GameServerEngine(
): m_voxelTypesRegistration(m_voxelTypeRegistry), m_voxelWorldGenerator(m_voxelTypeRegistry),
	m_voxelWorldStorage("world.sqlite", m_voxelTypeRegistry, m_voxelWorldGenerator),
	m_voxelWorld(this)
{
	m_voxelWorld.setChunkLoader(&m_voxelWorldStorage);
}

GameServerEngine::~GameServerEngine() {
	m_voxelLightComputer.shutdown();
	m_voxelWorldStorage.shutdown(true);
	m_voxelWorld.setChunkLoader(nullptr);
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
		std::unordered_set<VoxelChunkLocation> locations;
		m_voxelWorld.forEachChunkLocation([&locations](const VoxelChunkLocation &location) {
			locations.emplace(location);
		});
		std::unique_lock<std::shared_mutex> lock(m_connectionsMutex);
		for (auto &connection : m_connections) {
			auto p = connection.second->positionChunk();
			for (int z = -p.second - 1; z <= p.second + 1; z++) {
				for (int y = -p.second - 1; y <= p.second + 1; y++) {
					for (int x = -p.second - 1; x <= p.second + 1; x++) {
						locations.erase({p.first.x + x, p.first.y + y, p.first.z + z});
					}
				}
			}
		}
		lock.unlock();
		if (!locations.empty()) {
			for (auto &location : locations) {
				m_voxelLightComputer.cancelComputeAsync(m_voxelWorld, location);
			}
			m_voxelWorld.unloadChunks(std::vector<VoxelChunkLocation>(locations.begin(), locations.end()));
			LOG(INFO) << "Unloaded " << locations.size() << " chunk(s)";
		}
	}
	for (auto &&transport : m_transports) {
		transport->shutdown();
	}
	std::unique_lock<std::shared_mutex> lock(m_connectionsMutex);
	m_connections.clear();
	m_voxelWorld.unload();
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
