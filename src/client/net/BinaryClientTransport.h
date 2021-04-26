#pragma once

#include <vector>
#include <mutex>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include "ClientTransport.h"

class BinaryClientTransport: public ClientTransport {
protected:
	template<typename T> static void deserialize(const std::string &data, T& message) {
		bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::string>>(
				{data.begin(), data.size()},
				message
		);
	}
	
	template<typename T> void serializeAndSendMessage(const T &message) {
		std::string buffer;
		auto count = bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::string>>(buffer, message);
		sendMessage(buffer.data(), count);
	}
	
	void handleMessage(const std::string &payload);
	
	virtual void sendMessage(const void *data, size_t dataSize) = 0;
	
	using ClientTransport::sendPlayerPosition;

	void sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) override;
	
	void sendHello();
	
};
