#pragma once

#include <glm/vec3.hpp>
#include "ServerTransport.h"

class ClientConnection {
	ServerTransport &m_transport;

public:
	constexpr explicit ClientConnection(ServerTransport &transport): m_transport(transport) {
	}
	virtual ~ClientConnection() = default;
	[[nodiscard]] ServerTransport &transport() const {
		return m_transport;
	}
	void updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius);
	
};
