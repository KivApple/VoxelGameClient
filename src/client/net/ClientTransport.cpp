#include "ClientTransport.h"
#include "../GameEngine.h"

void ClientTransport::setPosition(const glm::vec3 &position) {
	GameEngine::instance().log("Player position set from the server");
	GameEngine::instance().setPlayerPosition(position);
}
