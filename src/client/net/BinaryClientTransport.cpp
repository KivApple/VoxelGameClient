#include <easylogging++.h>
#include "BinaryClientTransport.h"
#include "../GameEngine.h"
#include "net/ServerMessage.h"
#include "net/ClientMessage.h"

BinaryClientTransport::BinaryClientTransport(
		GameEngine &engine
): ClientTransport(engine), m_voxelTypeSerializationContext(engine.voxelTypeRegistry()) {
}

void BinaryClientTransport::handleMessage(const std::string &payload) {
	ServerMessage<ServerMessageData::Empty> genericMessage;
	deserialize(payload, genericMessage);
	switch (genericMessage.type) {
		case ServerMessageType::SET_POSITION: {
			ServerMessage<ServerMessageData::SetPosition> msg;
			deserialize(payload, msg);
			handleSetPosition({msg.data.x, msg.data.y, msg.data.z});
			break;
		}
		case ServerMessageType::SET_VOXEL_TYPES: {
			ServerMessage<ServerMessageData::SetVoxelTypes> msg({m_voxelTypeSerializationContext});
			deserialize(payload, msg);
			auto names = m_voxelTypeSerializationContext.names();
			for (size_t i = 0; i < names.size(); i++) {
				LOG(INFO) << "Received \"" << names[i] << "\" voxel type (id " << i << ") from the server";
			}
			break;
		}
		case ServerMessageType::SET_CHUNK: {
			VoxelDeserializer deserializer(m_voxelTypeSerializationContext, payload.cbegin(), payload.cend());
			ServerMessage<ServerMessageData::SetChunk> msg;
			deserializer.object(msg);
			handleSetChunk(msg.data.location, deserializer);
			break;
		}
		case ServerMessageType::DISCARD_CHUNKS: {
			ServerMessage<ServerMessageData::DiscardChunks> msg;
			deserialize(payload, msg);
			handleDiscardChunks(msg.data.locations);
			break;
		}
		case ServerMessageType::SET_INVENTORY: {
			VoxelDeserializer deserializer(m_voxelTypeSerializationContext, payload.cbegin(), payload.cend());
			ServerMessage<ServerMessageData::SetInventory> msg;
			deserializer.object(msg);
			LOG(DEBUG) << "Received " << msg.data.voxels.size() << " inventory change(s)";
			handleSetInventory(std::move(msg.data.voxels), msg.data.active);
			break;
		}
		default:
			LOG(WARNING) << "Unknown server message type: " << (int) genericMessage.type <<
				" (" << payload.size() << " bytes)";
	}
}

void BinaryClientTransport::sendHello() {
	sendPlayerPosition();
}

void BinaryClientTransport::sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	serializeAndSendMessage(ClientMessage<ClientMessageData::UpdatePosition>({
		position.x, position.y, position.z,
		yaw, pitch,
		(uint8_t) viewRadius
	}));
}

void BinaryClientTransport::updateActiveInventoryItem(int index) {
	LOG(DEBUG) << "Selected inventory item #" << index;
	serializeAndSendMessage(ClientMessage<ClientMessageData::UpdateActiveInventoryItem>({
		(uint8_t) index
	}));
}

void BinaryClientTransport::digVoxel(const VoxelLocation &location) {
	LOG(DEBUG) << "Dig voxel at x=" << location.x << ",y=" << location.y << ",z=" << location.z;
	serializeAndSendMessage(ClientMessage<ClientMessageData::DigVoxel>({
		location
	}));
}

void BinaryClientTransport::placeVoxel(const VoxelLocation &location) {
	LOG(DEBUG) << "Place voxel at x=" << location.x << ",y=" << location.y << ",z=" << location.z;
	serializeAndSendMessage(ClientMessage<ClientMessageData::PlaceVoxel>({
		location
	}));
}
