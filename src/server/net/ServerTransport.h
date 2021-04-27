#pragma once

class GameServerEngine;

class ServerTransport {
public:
	virtual ~ServerTransport() = default;
	virtual void start(GameServerEngine &engine) = 0;
	virtual void shutdown() = 0;
	virtual GameServerEngine *engine() = 0;
	
};
