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
			setPosition({msg.data.x, msg.data.y, msg.data.z});
			break;
		}
		default:
			GameEngine::instance().log("Unknown server message type: %i", (int) genericMessage.type);
	}
}

void BinaryClientTransport::sendHello() {
	sendPlayerPosition();
}

void BinaryClientTransport::sendPlayerPosition() {
	std::unique_lock<std::mutex> lock(m_playerPositionMutex);
	if (!m_playerPositionValid) return;
	serializeAndSendMessage(ClientMessage<ClientMessageData::UpdatePosition>({
		m_playerPosition.x, m_playerPosition.y, m_playerPosition.z,
		m_playerYaw, m_playerPitch,
		(uint8_t) m_viewRadius
	}));
}

void BinaryClientTransport::sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	std::unique_lock<std::mutex> lock(m_playerPositionMutex);
	if (
			m_playerPositionValid &&
			m_playerPosition == position && m_playerYaw == yaw && m_playerPitch == pitch &&
			m_viewRadius == viewRadius
	) {
		return;
	}
	m_playerPosition = position;
	m_playerYaw = yaw;
	m_playerPitch = pitch;
	m_viewRadius = viewRadius;
	m_playerPositionValid = true;
	lock.unlock();
	sendPlayerPosition();
}
