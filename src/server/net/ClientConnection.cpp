#include <cstdio>
#include <cmath>
#include "ClientConnection.h"

void ClientConnection::updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	printf(
			"[Client %p] updatePosition(x=%0.2f, y=%0.2f, z=%0.2f, yaw=%0.1f, pitch=%0.1f, viewRadius=%i)\n",
			this, position.x, position.y, position.z, yaw, pitch, viewRadius
	);
	std::unique_lock<std::shared_mutex> lock(m_positionMutex);
	bool resetPosition = false;
	if (m_positionValid) {
		auto delta = position - m_position;
		static const float MAX_DELTA = 0.05f;
		if (fabsf(delta.x) >= MAX_DELTA || fabsf(delta.y) >= MAX_DELTA || fabs(delta.z) >= MAX_DELTA) {
			printf("[Client %p] player is moving too fast\n", this);
			resetPosition = true;
		}
	}
	if (!resetPosition) {
		m_position = position;
	}
	m_yaw = yaw;
	m_pitch = pitch;
	m_viewRadius = viewRadius;
	m_positionValid = true;
	auto newPosition = m_position;
	lock.unlock();
	if (resetPosition) {
		setPosition(newPosition);
	}
}
