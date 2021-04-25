#pragma once

#include <glm/vec3.hpp>

class ClientTransport {
public:
	virtual ~ClientTransport() = default;
	virtual void start() = 0;
	virtual void shutdown() = 0;
	virtual void sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch) = 0;
	
};
