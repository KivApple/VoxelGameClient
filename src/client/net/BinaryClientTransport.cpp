#include "BinaryClientTransport.h"
#include "../GameEngine.h"
#include "net/ServerMessage.h"
#include "net/ClientMessage.h"

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
