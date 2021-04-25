#pragma once

#include <vector>
#include <mutex>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include "ClientTransport.h"

class BinaryClientTransport: public ClientTransport {
	glm::vec3 m_playerPosition = glm::vec3(0.0f);
	float m_playerYaw = 0.0f;
	float m_playerPitch = 0.0f;
	int m_viewRadius = 0;
	bool m_playerPositionValid = false;
	std::mutex m_playerPositionMutex;
	
	void sendPlayerPosition();

protected:
	void handleMessage(const void *data, size_t dataSize);
	template<typename T> void serializeAndSendMessage(const T &message) {
		std::vector<uint8_t> buffer;
		auto count = bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<uint8_t>>>(buffer, message);
		sendMessage(buffer.data(), count);
	}
	virtual void sendMessage(const void *data, size_t dataSize) = 0;
	void sendHello();
	
public:
	void sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) override;
	
};
