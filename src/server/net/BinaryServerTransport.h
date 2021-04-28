#pragma once

#include <memory>
#include <shared_mutex>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include "ServerTransport.h"
#include "ClientConnection.h"
#include "world/Voxel.h"

class VoxelChunkRef;

class BinaryServerTransport: public ServerTransport {
protected:
	template<typename T> static void deserialize(const std::string &data, T& message) {
		bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::string>>(
				{data.begin(), data.size()},
				message
		);
	}
	
	class Connection: public ClientConnection {
		std::unique_ptr<VoxelTypeSerializationContext> m_voxelSerializationContext;
		std::shared_mutex m_voxelSerializationContextMutex;
		
	protected:
		void deserializeAndHandleMessage(const std::string &payload);
		virtual void sendMessage(const void *data, size_t dataSize) = 0;
		template<typename T> void serializeAndSendMessage(const T &message) {
			std::string buffer;
			auto count = bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::string>>(
					buffer,
					message
			);
			sendMessage(buffer.data(), count);
		}
		void setPosition(const glm::vec3 &position) override;
		void setVoxelTypes();
		void setChunk(const VoxelChunkRef &chunk) override;
	
	public:
		explicit Connection(BinaryServerTransport &transport): ClientConnection(transport) {
		}
		
	};
	
};
