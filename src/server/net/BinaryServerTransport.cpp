#include "BinaryServerTransport.h"
#include "net/ClientMessage.h"

void BinaryServerTransport::Connection::deserializeAndHandleMessage(const std::string &payload) {
	ClientMessage<ClientMessageData::Empty> genericMessage;
	deserialize(payload, genericMessage);
	switch (genericMessage.type) {
		case ClientMessageType::UPDATE_POSITION: {
			ClientMessage<ClientMessageData::UpdatePosition> msg;
			deserialize(payload, msg);
			updatePosition({msg.data.x, msg.data.y, msg.data.z}, msg.data.yaw, msg.data.pitch, msg.data.viewRadius);
			break;
		}
		default:
			printf("[Client %p] Unknown message received from client: %i\n", this, (int) genericMessage.type);
	}
}
