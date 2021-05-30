#pragma once

#include "Entity.h"

class PlayerEntityType: public EntityType<PlayerEntityType> {
	static EntityPhysics PHYSICS;

public:
	const EntityPhysics &physics(const Entity &entity, const State &state);

};
