#include <cstdio>
#include "ClientConnection.h"

void ClientConnection::updatePosition(const glm::vec3 &position, float yaw, float pitch, int viewRadius) {
	printf(
			"[Client %p] updatePosition(x=%0.2f, y=%0.2f, z=%0.2f, yaw=%0.1f, pitch=%0.1f, viewRadius=%i)\n",
			this, position.x, position.y, position.z, yaw, pitch, viewRadius
	);
}
