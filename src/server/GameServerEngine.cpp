#include <chrono>
#include <thread>
#include "GameServerEngine.h"

void GameServerEngine::addTransport(std::unique_ptr<ServerTransport> transport) {
	m_transports.emplace_back(std::move(transport));
}

int GameServerEngine::run() {
	for (auto &&transport : m_transports) {
		transport->start();
	}
	while (m_running) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	for (auto &&transport : m_transports) {
		transport->shutdown();
	}
	return 0;
}

void GameServerEngine::shutdown() {
	m_running = false;
}
