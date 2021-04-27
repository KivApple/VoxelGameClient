#include <easylogging++.h>
#include "ClientTransport.h"
#include "../GameEngine.h"

ClientTransport::ClientTransport(GameEngine &engine): m_engine(engine) {
}

void ClientTransport::handleSetPosition(const glm::vec3 &position) {
	LOG(WARNING) << "Player position set from the server";
	m_engine.setPlayerPosition(position);
}

void ClientTransport::handleSetChunk(const VoxelChunkLocation &location, VoxelDeserializer &deserializer) {
	LOG(DEBUG) << "Received chunk x=" << location.x << ",y=" << location.y << ",z=" << location.z;
	auto chunk = m_engine.voxelWorld().mutableChunk(location, true);
	deserializer.object(chunk);
	m_engine.voxelWorldRenderer().invalidate(location);
}

void ClientTransport::sendPlayerPosition() {
	std::unique_lock<std::mutex> lock(m_playerPositionMutex);
	if (!m_playerPositionValid) return;
	auto position = m_playerPosition;
	auto yaw = m_playerYaw;
	auto pitch = m_playerPitch;
	auto viewRadius = m_viewRadius;
	lock.unlock();
	sendPlayerPosition(position, yaw, pitch, viewRadius);
}

void ClientTransport::updatePlayerPosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
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
