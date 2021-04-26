#pragma once

#include <glm/vec3.hpp>
#include <mutex>

class ClientTransport {
	glm::vec3 m_playerPosition = glm::vec3(0.0f);
	float m_playerYaw = 0.0f;
	float m_playerPitch = 0.0f;
	int m_viewRadius = 0;
	bool m_playerPositionValid = false;
	std::mutex m_playerPositionMutex;
	
protected:
	void handleSetPosition(const glm::vec3 &position);
	virtual void sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) = 0;
	
public:
	virtual ~ClientTransport() = default;
	virtual void start() = 0;
	virtual void shutdown() = 0;
	virtual bool isConnected() = 0;
	void updatePlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius);
	void sendPlayerPosition();
	
};
