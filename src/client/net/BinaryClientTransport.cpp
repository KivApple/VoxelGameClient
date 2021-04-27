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
			bitsery::Deserializer<bitsery::InputBufferAdapter<std::string>> deserializer(
					payload.cbegin(),
					payload.cend()
			);
			ServerMessage<ServerMessageData::SetVoxelTypes> msg;
			deserializer.object(msg);
			deserializer.object(m_voxelTypeSerializationContext);
			break;
		}
		case ServerMessageType::SET_CHUNK: {
			VoxelDeserializer deserializer(m_voxelTypeSerializationContext, payload.cbegin(), payload.cend());
			ServerMessage<ServerMessageData::SetChunk> msg;
			deserializer.object(msg);
			handleSetChunk(msg.data.location, deserializer);
			break;
		}
		default:
			GameEngine::instance().log("Unknown server message type: %i", (int) genericMessage.type);
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
