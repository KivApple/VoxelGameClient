#pragma once

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include "ServerTransport.h"
#include "ClientConnection.h"

class BinaryServerTransport: public ServerTransport {
protected:
	template<typename T> static void deserialize(const std::string &data, T& message) {
		bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::string>>(
				{data.begin(), data.size()},
				message
		);
	}
	
	class Connection: public ClientConnection {
	protected:
		void deserializeAndHandleMessage(const std::string &payload);
		virtual void sendMessage(const void *data, size_t dataSize) = 0;
		template<typename T> void serializeAndSendMessage(const T &message) {
			std::vector<uint8_t> buffer;
			auto count = bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<uint8_t>>>(
					buffer,
					message
			);
			sendMessage(buffer.data(), count);
		}
	
	public:
		constexpr explicit Connection(BinaryServerTransport &transport): ClientConnection(transport) {
		}
		
	};
	
};
