#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include "net/ServerTransport.h"

class GameServerEngine {
	std::vector<std::unique_ptr<ServerTransport>> m_transports;
	std::atomic<bool> m_running = true;
	
public:
	void addTransport(std::unique_ptr<ServerTransport> transport);
	int run();
	void shutdown();

};
