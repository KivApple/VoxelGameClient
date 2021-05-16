#pragma once

#include <memory>
#include <utility>
#include "Voxel.h"
#include "VoxelTypeRegistry.h"

struct LiquidFlowVoxelState {
	uint8_t level = 0;
	uint8_t countdown = 0;
	
	template<typename S> void serialize(S &s) {
		s.value1b(level);
		s.value1b(countdown);
	}
};

class LiquidVoxelBaseTrait;

class LiquidFlowVoxelTrait: public VoxelTypeTrait<LiquidFlowVoxelState> {
	LiquidVoxelBaseTrait *m_trait = nullptr;
	
public:
	LiquidFlowVoxelTrait() = default;
	void setTrait(LiquidVoxelBaseTrait &trait);
	std::string toString(const State &voxel);
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data
	);
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	
};

struct LiquidVoxelState {
	uint8_t countdown = 0;
	
	template<typename S> void serialize(S &s) {
		s.value1b(countdown);
	}
};

class LiquidVoxelBaseTrait: public VoxelTypeTrait<LiquidVoxelState> {
	int m_maxFlowLevel;
	int m_flowSlowdown;
	bool m_canSpawn;
	VoxelTypeInterface *m_air = &EmptyVoxelType::INSTANCE;

public:
	LiquidVoxelBaseTrait(
			int maxFlowLevel,
			int flowSlowdown,
			bool canSpawn
	);
	virtual VoxelTypeInterface &flowType() = 0;
	void link(VoxelTypeRegistry &registry);
	float calculateModelOffset(int level) const;
	float calculateModelOffset(const VoxelHolder &voxel);
	void adjustMesh(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			int level,
			std::vector<VoxelVertexData> &data
	);
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data
	);
	void flowBuildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const LiquidFlowVoxelTrait::State &voxel,
			std::vector<VoxelVertexData> &data
	);
	bool canFlow(int sourceLevel, const VoxelHolder &target, int dy, bool strict);
	void setSource(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			LiquidFlowVoxelTrait::State &flow
	);
	void setFlow(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			int level
	);
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	bool flowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			LiquidFlowVoxelTrait::State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	);
	
};

template<typename ...Traits> class LiquidFlowVoxelType: public VoxelType<
        LiquidFlowVoxelType<Traits...>,
        EmptyVoxelTypeTraitState,
        Traits...,
        LiquidFlowVoxelTrait
> {
public:
	template<typename ...Args> explicit LiquidFlowVoxelType(Args&&... args): VoxelType<
        LiquidFlowVoxelType<Traits...>,
        EmptyVoxelTypeTraitState,
        Traits...,
        LiquidFlowVoxelTrait
	>(std::forward<Args>(args)..., LiquidFlowVoxelTrait()) {
	}
	
};

template<typename ...FlowTraits> class LiquidVoxelTrait: public LiquidVoxelBaseTrait {
	std::unique_ptr<LiquidFlowVoxelType<FlowTraits...>> m_flowTypePtr;
	VoxelTypeInterface *m_flowType;
	
public:
	template<typename ...Args> explicit LiquidVoxelTrait(
			int maxFlowLevel,
			int flowSlowdown,
			bool canSpawn,
			Args&&... args
	): LiquidVoxelBaseTrait(maxFlowLevel, flowSlowdown, canSpawn) {
		m_flowTypePtr = std::make_unique<LiquidFlowVoxelType<FlowTraits...>>(std::forward<Args>(args)...);
		m_flowType = m_flowTypePtr.get();
	}
	VoxelTypeInterface &flowType() final {
		return *m_flowType;
	}
	void handleRegistration(const std::string &name, VoxelTypeRegistry &registry) {
		assert(m_flowTypePtr);
		m_flowTypePtr->setTrait(*this);
		registry.add(name + "_flow", std::move(m_flowTypePtr));
	}

};
