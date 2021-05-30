#include "Player.h"

EntityPhysics PlayerEntityType::PHYSICS(
		EntityPhysicsType::DYNAMIC,
		1,
		2,
		0.25f,
		0.05f
);

const EntityPhysics &PlayerEntityType::physics(const Entity &entity, const State &state) {
	return PHYSICS;
}
