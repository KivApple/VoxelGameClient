#include <cstdio>
#include "ClientConnection.h"

void ClientConnection::updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	printf(
			"[Client %p] updatePosition(x=%0.2f, y=%0.2f, z=%0.2f, yaw=%0.1f, pitch=%0.1f, viewRadius=%i)\n",
			this, position.x, position.y, position.z, yaw, pitch, viewRadius
	);
	if (m_positionValid) {
		auto delta = position - m_position;
		static const float MAX_DELTA = 0.05f;
		if (delta.x >= MAX_DELTA || delta.y >= MAX_DELTA || delta.z >= MAX_DELTA) {
			printf("[Client %p] player is moving too fast\n", this);
			setPosition(m_position);
		} else {
			m_position = position;
		}
	} else {
		m_position = position;
	}
	m_yaw = yaw;
	m_pitch = pitch;
	m_viewRadius = viewRadius;
	m_positionValid = true;
}
