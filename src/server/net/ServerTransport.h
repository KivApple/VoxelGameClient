#pragma once

class ServerTransport {
public:
	virtual ~ServerTransport() = default;
	virtual void start() = 0;
	virtual void shutdown() = 0;
	
};
