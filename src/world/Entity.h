#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "VoxelLocation.h"
#include "EntityPhysics.h"
#ifndef HEADLESS
#include "client/world/Model.h"
#endif

class VoxelWorld;

class VoxelChunkRef;
class VoxelChunkMutableRef;
class VoxelChunkExtendedRef;
class VoxelChunkExtendedMutableRef;

struct EntityOrientation {
	float yaw;
	float pitch;
	float roll;
	
	constexpr EntityOrientation(): yaw(0.0f), pitch(0.0f), roll(0.0f) {
	}
	
	constexpr EntityOrientation(float yaw, float pitch, float roll): yaw(yaw), pitch(pitch), roll(roll) {
	}
	
	template<typename S> void serialize(S &s) {
		s.value4b(yaw);
		s.value4b(pitch);
		s.value4b(roll);
	}
};

class Entity;

class EntityTypeInterface {
public:
	virtual ~EntityTypeInterface() = default;
	virtual Entity *invokeNew(
			const VoxelLocation &location,
			const EntityOrientation &orientation = EntityOrientation()
	) = 0;
	virtual Entity *invokeNew(
			const VoxelChunkLocation &chunkLocation,
			const glm::vec3 &inChunkLocation,
			const EntityOrientation &orientation = EntityOrientation()
	) = 0;
	virtual const EntityPhysics &invokePhysics(const Entity &entity) = 0;
	virtual void invokeUpdate(VoxelChunkExtendedMutableRef &chunk, Entity &entity) = 0;
#ifndef HEADLESS
	virtual void invokeRender(const Entity &entity, const glm::mat4 &view, const glm::mat4 &projection) = 0;
#endif
	
};

class Entity {
	EntityTypeInterface &m_type;
	VoxelChunkLocation m_chunkLocation;
	glm::vec3 m_inChunkLocation;
	EntityOrientation m_orientation;
	std::shared_mutex m_chunkLocationMutex;
	
	void applyMovementConstraint(const glm::vec3 &prevPosition, const VoxelChunkExtendedRef &chunk);
	void updateContainedChunk(VoxelChunkExtendedMutableRef &chunk);
	
	friend class VoxelChunkExtendedMutableRef;

public:
	Entity(
			EntityTypeInterface &type,
			const VoxelLocation &location,
			const EntityOrientation &orientation = EntityOrientation()
	);
	Entity(
			EntityTypeInterface &type,
			const VoxelChunkLocation &chunkLocation,
			const glm::vec3 &inChunkLocation,
			const EntityOrientation &orientation = EntityOrientation()
	);
	virtual ~Entity() = default;
	
	VoxelChunkRef chunk(VoxelWorld &world, bool load);
	VoxelChunkMutableRef mutableChunk(VoxelWorld &world, bool load);
	VoxelChunkExtendedMutableRef extendedMutableChunk(VoxelWorld &world, bool load);
	
	[[nodiscard]] glm::vec3 direction(bool usePitch) const;
	[[nodiscard]] glm::vec3 upDirection() const;
	
	[[nodiscard]] glm::vec3 position() const;
	[[nodiscard]] const EntityOrientation &orientation() const {
		return m_orientation;
	}
	[[nodiscard]] const EntityPhysics &physics() const {
		return m_type.invokePhysics(*this);
	}
	
	void setPosition(VoxelChunkExtendedMutableRef &chunk, const glm::vec3 &position);
	void setRotation(float yaw, float pitch);
	void adjustRotation(float dYaw, float dPitch);

	void move(VoxelWorld &world, const glm::vec3 &delta);
	void move(VoxelChunkExtendedMutableRef &chunk, const glm::vec3 &delta);
	
	void update(VoxelChunkExtendedMutableRef &chunk);
#ifndef HEADLESS
	void render(const glm::mat4 &view, const glm::mat4 &projection) {
		m_type.invokeRender(*this, view, projection);
	}
#endif
	
};

struct EmptyEntityTypeTraitState {
	template<typename S> void serialize(S &s) {
	}
};

template<typename StateType=EmptyEntityTypeTraitState> class EntityTypeTrait {
public:
	struct StateWrapper: public StateType {
	};
	
	typedef StateWrapper State;
	
	EntityTypeTrait() = default;
	EntityTypeTrait(const EntityTypeTrait &trait) = delete;
	EntityTypeTrait(EntityTypeTrait &&trait) noexcept = default;
	virtual ~EntityTypeTrait() = default;
	
};

template<
        typename T,
        typename BaseState=EmptyEntityTypeTraitState,
        typename ...Traits
> class EntityType: public EntityTypeInterface, public Traits... {
	template<typename Args> struct ExpandStates { // Workaround for MSVC compiler bug
		typedef typename Args::State State;
	};
	
	struct BaseStateWrapper: public BaseState {
	};

public:
	struct State: public BaseStateWrapper, public ExpandStates<Traits>::State... {
		template<typename S> void serialize(S &s) {
			s.object(*static_cast<BaseStateWrapper*>(this));
			traitsSerialize(s, static_cast<typename ExpandStates<Traits>::State*>(this)...);
		}
	
	private:
		template<typename S, typename TraitState, typename ...RestTraitStates> void traitsSerialize(
				S &s,
				TraitState *state,
				RestTraitStates... restTraits
		) {
			s.object(*state);
			traitsSerialize(s, restTraits...);
		}
		
		template<typename S> void traitsSerialize(S &s) {
		}
		
	};
	
	class Data: public Entity, public State {
	public:
		template<typename ...Args> explicit Data(Args&&... args): Entity(std::forward<Args>(args)...) {
		}
		
		template<typename S> void serialize(S &s) {
			s.object(*static_cast<Entity*>(this));
			s.object(*static_cast<State*>(this));
		}
	};

private:
	template<typename Trait> static constexpr auto traitHasPhysics(
			Trait*,
			const Entity *entity,
			const State *state
	) -> decltype(std::declval<Trait>().physics(*entity, *state), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasPhysics(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> const EntityPhysics &traitsPhysics(
			const Entity &entity,
			const State &state,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasInit<Trait>(
				(Trait*) 0,
				(const Entity*) 0,
				(const State*) 0
		)) {
			auto physics = trait->physics(entity, state);
			if (physics != nullptr) {
				return *physics;
			}
		}
		return traitsPhysics(entity, state, restTraits...);
	}
	const EntityPhysics &traitsPhysics(const Entity &entity, const State &state) {
		return EntityPhysics::NONE;
	}
	
	template<typename Trait> static constexpr auto traitHasUpdate(
			Trait*,
			Entity *entity,
			State *state,
			VoxelChunkExtendedMutableRef *chunk
	) -> decltype(std::declval<Trait>().update(*entity, *state, *chunk), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasUpdate(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsUpdate(
			Entity &entity,
			State &state,
			VoxelChunkExtendedMutableRef &chunk,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasUpdate<Trait>(
				(Trait*) 0,
				(Entity*) 0,
				(State*) 0,
				(VoxelChunkExtendedMutableRef*) 0
		)) {
			trait->update(entity, state, chunk);
		}
		traitsUpdate(entity, state, chunk, restTraits...);
	}
	void traitsUpdate(Entity &entity, State &state, VoxelChunkExtendedMutableRef &chunk) {
	}

#ifndef HEADLESS
	template<typename Trait> static constexpr auto traitHasRender(
			Trait*,
			const Entity *entity,
			const State *state,
			const glm::mat4 *view,
			const glm::mat4 *projection
	) -> decltype(std::declval<Trait>().render(*entity, *state, *view, *projection), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasRender(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsRender(
			const Entity &entity,
			const State &state,
			const glm::mat4 &view,
			const glm::mat4 &projection,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasRender<Trait>(
				(Trait*) 0,
				(const Entity*) 0,
				(const State*) 0,
				(const glm::mat4*) 0,
				(const glm::mat4*) 0
		)) {
			trait->render(entity, state, view, projection);
		}
		traitsRender(entity, state, view, projection, restTraits...);
	}
	void traitsRender(const Entity &entity, const State &state, const glm::mat4 &view, const glm::mat4 &projection) {
	}
#endif

public:
	explicit EntityType(Traits&&... traits): Traits(std::forward<Traits>(traits))... {
	}
	
	Entity *invokeNew(
			const VoxelLocation &location,
			const EntityOrientation &orientation = EntityOrientation()
	) override {
		return new Data(*this, location, orientation);
	}
	Entity *invokeNew(
			const VoxelChunkLocation &chunkLocation,
			const glm::vec3 &inChunkLocation,
			const EntityOrientation &orientation = EntityOrientation()
	) override {
		return new Data(*this, chunkLocation, inChunkLocation, orientation);
	}
	
	const EntityPhysics &physics(const Entity &entity, const State &state) {
		return traitsPhysics(entity, state, static_cast<Traits*>(this)...);
	}
	const EntityPhysics &invokePhysics(const Entity &entity) override {
		return static_cast<T*>(this)->T::physics(entity, static_cast<const Data&>(entity));
	}
	
	void update(Entity &entity, State &state, VoxelChunkExtendedMutableRef &chunk) {
		traitsUpdate(entity, state, chunk, static_cast<Traits*>(this)...);
	}
	void invokeUpdate(VoxelChunkExtendedMutableRef &chunk, Entity &entity) override {
		static_cast<T*>(this)->T::update(entity, static_cast<Data&>(entity), chunk);
	}

#ifndef HEADLESS
	void render(const Entity &entity, const State &state, const glm::mat4 &view, const glm::mat4 &projection) {
		traitsRender(entity, state, view, projection, static_cast<Traits*>(this)...);
	}
	void invokeRender(const Entity &entity, const glm::mat4 &view, const glm::mat4 &projection) override {
		static_cast<T*>(this)->T::render(entity, static_cast<const Data&>(entity), view, projection);
	}
#endif

};

class SimpleEntityType: public EntityType<SimpleEntityType> {
	const EntityPhysics &m_physics;
#ifndef HEADLESS
	const Model *m_model;
#endif

public:
	explicit SimpleEntityType(
			const EntityPhysics &physics
#ifndef HEADLESS
			, const Model *model
#endif
	);
	const EntityPhysics &physics(const Entity &entity, const State &state);
#ifndef HEADLESS
	void render(const Entity &entity, const State &state, const glm::mat4 &view, const glm::mat4 &projection);
#endif

};
