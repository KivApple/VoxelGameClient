#include "LiquidVoxelType.h"
#include "VoxelWorld.h"
#include "VoxelWorldUtils.h"

void LiquidFlowVoxelTrait::setTrait(LiquidVoxelBaseTrait &trait) {
	m_trait = &trait;
}

std::string LiquidFlowVoxelTrait::toString(const State &voxel) {
	return "[level=" + std::to_string(voxel.level) + "]";
}

void LiquidFlowVoxelTrait::buildVertexData(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		const State &voxel,
		std::vector<VoxelVertexData> &data
) {
	assert(m_trait != nullptr);
	m_trait->flowBuildVertexData(chunk, location, voxel, data);
}

bool LiquidFlowVoxelTrait::update(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &rawVoxel,
		State &voxel,
		unsigned long deltaTime,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	assert(m_trait != nullptr);
	return m_trait->flowUpdate(chunk, location, rawVoxel, voxel, deltaTime, invalidatedLocations);
}

LiquidVoxelBaseTrait::LiquidVoxelBaseTrait(
		int maxFlowLevel,
		int flowSlowdown,
		bool canSpawn
): m_maxFlowLevel(maxFlowLevel), m_flowSlowdown(flowSlowdown), m_canSpawn(canSpawn) {
}

void LiquidVoxelBaseTrait::link(VoxelTypeRegistry &registry) {
	m_air = &registry.get("air");
}

float LiquidVoxelBaseTrait::calculateModelOffset(int level) const {
	if (level == INT_MAX) {
		level = m_maxFlowLevel - 1;
	}
	return static_cast<float>(m_maxFlowLevel - std::min(level, m_maxFlowLevel)) / (float) m_maxFlowLevel;
}

float LiquidVoxelBaseTrait::calculateModelOffset(const VoxelHolder &voxel) {
	if (&voxel.type() == &type()) {
		return calculateModelOffset(INT_MAX);
	} else if (&voxel.type() == &flowType()) {
		return calculateModelOffset(voxel.get<LiquidFlowVoxelTrait::State>().level);
	} else {
		return 1.0f;
	}
}

void LiquidVoxelBaseTrait::adjustMesh(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		int level,
		std::vector<VoxelVertexData> &data
) {
	auto &posY = chunk.extendedAt(location.x, location.y + 1, location.z);
	if (&posY.type() == &type() || &posY.type() == &flowType()) return;
	
	float offset = calculateModelOffset(level);
	
	float negXOffset = calculateModelOffset(chunk.extendedAt(location.x - 1, location.y, location.z));
	float posXOffset = calculateModelOffset(chunk.extendedAt(location.x + 1, location.y, location.z));
	float negZOffset = calculateModelOffset(chunk.extendedAt(location.x, location.y, location.z - 1));
	float posZOffset = calculateModelOffset(chunk.extendedAt(location.x, location.y, location.z + 1));
	
	float negXNegZOffset = calculateModelOffset(chunk.extendedAt(location.x - 1, location.y, location.z - 1));
	float negXPosZOffset = calculateModelOffset(chunk.extendedAt(location.x - 1, location.y, location.z + 1));
	float posXNegZOffset = calculateModelOffset(chunk.extendedAt(location.x + 1, location.y, location.z - 1));
	float posXPosZOffset = calculateModelOffset(chunk.extendedAt(location.x + 1, location.y, location.z + 1));
	
	for (auto &v : data) {
		if (almostEqual(v.y, 0.5f)) {
			if (almostEqual(v.x, -0.5f) && almostEqual(v.z, -0.5f)) {
				v.y -= std::min(
						std::min(offset, negXNegZOffset),
						std::min(negXOffset, negZOffset)
				);
			} else if (almostEqual(v.x, -0.5f) && almostEqual(v.z, 0.5f)) {
				v.y -= std::min(
						std::min(offset, negXPosZOffset),
						std::min(negXOffset, posZOffset)
				);
			} else if (almostEqual(v.x, 0.5f) && almostEqual(v.z, -0.5f)) {
				v.y -= std::min(
						std::min(offset, posXNegZOffset),
						std::min(posXOffset, negZOffset)
				);
			} else if (almostEqual(v.x, 0.5f) && almostEqual(v.z, 0.5f)) {
				v.y -= std::min(
						std::min(offset, posXPosZOffset),
						std::min(posXOffset, posZOffset)
				);
			}
		}
	}
}

void LiquidVoxelBaseTrait::buildVertexData(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		const State &voxel,
		std::vector<VoxelVertexData> &data
) {
	adjustMesh(chunk, location, INT_MAX, data);
}

void LiquidVoxelBaseTrait::flowBuildVertexData(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		const LiquidFlowVoxelTrait::State &voxel,
		std::vector<VoxelVertexData> &data
) {
	adjustMesh(chunk, location, voxel.level, data);
}

bool LiquidVoxelBaseTrait::canFlow(int sourceLevel, const VoxelHolder &target, int dy, bool strict) {
	if (dy > 0) {
		return false;
	}
	if (sourceLevel <= 1 && dy == 0) {
		return false;
	}
	if (&target.type() == &flowType()) {
		auto &flow = target.get<LiquidFlowVoxelTrait::State>();
		if (strict) {
			if (sourceLevel > flow.level + 1 || (dy < 0 && flow.level < m_maxFlowLevel)) {
				return true;
			}
		} else {
			if (sourceLevel >= flow.level + 1 || dy < 0) {
				return true;
			}
		}
	}
	if (&target.type() == m_air) {
		return true;
	}
	return false;
}

void LiquidVoxelBaseTrait::setSource(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		LiquidFlowVoxelTrait::State &flow
) {
	chunk.extendedAt(location).setType(type());
}

void LiquidVoxelBaseTrait::setFlow(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		int level
) {
	assert(level >= 1);
	auto &voxel = chunk.extendedAt(location);
	voxel.setType(flowType());
	voxel.get<LiquidFlowVoxelTrait::State>().level = level;
}

bool LiquidVoxelBaseTrait::update(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &rawVoxel,
		State &voxel,
		unsigned long deltaTime,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	voxel.countdown = deltaTime < m_flowSlowdown ? voxel.countdown + deltaTime : m_flowSlowdown;
	if (voxel.countdown < m_flowSlowdown) {
		return true;
	}
	voxel.countdown = 0;
	static const int offsets[][3] = {
			{1, 0, 0}, {-1, 0, 0},
			{0, 0, 1}, {0, 0, -1}
	};
	InChunkVoxelLocation bottom(location.x, location.y - 1, location.z);
	if (canFlow(m_maxFlowLevel, chunk.extendedAt(bottom), -1, false)) {
		if (canFlow(m_maxFlowLevel, chunk.extendedAt(bottom), -1, true)) {
			setFlow(chunk, bottom, m_maxFlowLevel);
			invalidatedLocations.emplace(bottom);
		}
	} else {
		for (auto &offset : offsets) {
			InChunkVoxelLocation l(
					location.x + offset[0],
					location.y + offset[1],
					location.z + offset[2]
			);
			if (canFlow(m_maxFlowLevel, chunk.extendedAt(l), offset[1], true)) {
				setFlow(chunk, l, offset[1] >= 0 ? m_maxFlowLevel - 1 : m_maxFlowLevel);
				invalidatedLocations.emplace(l);
			}
		}
	}
	return false;
}

bool LiquidVoxelBaseTrait::flowUpdate(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &rawVoxel,
		LiquidFlowVoxelTrait::State &voxel,
		unsigned long deltaTime,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	voxel.countdown = deltaTime < (unsigned) m_flowSlowdown ?
					  voxel.countdown + deltaTime :
					  (unsigned) m_flowSlowdown;
	if (voxel.countdown < m_flowSlowdown) {
		return true;
	}
	voxel.countdown = 0;
	static const int offsets[][3] = {
			{1, 0, 0}, {-1, 0, 0},
			{0, 1, 0},
			{0, 0, 1}, {0, 0, -1}
	};
	bool sourceFound = false;
	int sourceCount = 0;
	auto &voxelHolder = chunk.extendedAt(location);
	for (auto &offset : offsets) {
		InChunkVoxelLocation l(location.x + offset[0], location.y + offset[1], location.z + offset[2]);
		auto &source = chunk.extendedAt(l);
		if (&source.type() == &type()) {
			if (canFlow(m_maxFlowLevel, voxelHolder, -offset[1], false)) {
				sourceFound = true;
				sourceCount++;
			}
		} else if (&source.type() == &flowType()) {
			if (canFlow(
					source.get<LiquidFlowVoxelTrait::State>().level,
					voxelHolder,
					-offset[1],
					false
			)) {
				sourceFound = true;
			}
		}
	}
	InChunkVoxelLocation bottom(location.x, location.y - 1, location.z);
	if (canFlow(m_maxFlowLevel, chunk.extendedAt(bottom), -1, false)) {
		if (canFlow(voxel.level, chunk.extendedAt(bottom), -1, true)) {
			setFlow(chunk, bottom, m_maxFlowLevel);
			invalidatedLocations.emplace(bottom);
		}
	} else {
		for (auto &offset : offsets) {
			InChunkVoxelLocation l(
					location.x + offset[0],
					location.y + offset[1],
					location.z + offset[2]
			);
			if (canFlow(voxel.level, chunk.extendedAt(l), offset[1], true)) {
				setFlow(chunk, l, voxel.level - 1);
				invalidatedLocations.emplace(l);
			}
		}
	}
	if (!sourceFound) {
		if (voxel.level > 1) {
			voxel.level--;
		} else {
			voxelHolder.setType(*m_air);
		}
		invalidatedLocations.emplace(location);
		return true;
	} else if (sourceCount >= 2 && m_canSpawn) {
		setSource(chunk, location, voxel);
		invalidatedLocations.emplace(location);
	}
	return false;
}
