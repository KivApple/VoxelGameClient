#pragma once

#include <memory>
#include <utility>
#include "Voxel.h"
#include "VoxelLocation.h"
#include "VoxelWorld.h"
#include "VoxelTypeRegistry.h"
#include "VoxelWorldUtils.h"

template<typename T, typename Base=VoxelType, typename Data=Voxel, typename FlowData=Data> struct Liquid {
	struct FlowState: public FlowData {
		uint8_t level = 0;
		uint8_t countdown = 0;
		
		template<typename S> void serialize(S &s) {
			FlowData::serialize(s);
			s.value1b(level);
			s.value1b(countdown);
		}
	};
	
	class VoxelType;
	
	class FlowVoxelType: public VoxelTypeHelper<FlowVoxelType, FlowState, Base> {
		VoxelType &m_type;
		
	public:
		template<typename ...Args> explicit FlowVoxelType(
				VoxelType &type, Args&&... args
		): VoxelTypeHelper<FlowVoxelType, FlowState, Base>(std::forward<Args>(args)...), m_type(type) {
		}
		
		void registerChildren(const std::string &name, VoxelTypeRegistry &registry) override {
		}
		
		const VoxelType &sourceType() const {
			return m_type;
		}
		
		VoxelType &sourceType() {
			return m_type;
		}
		
		std::string toString(const FlowState &voxel) {
			return static_cast<T&>(m_type).T::flowToString(voxel);
		}
		
		const VoxelShaderProvider *shaderProvider(const FlowState &voxel) {
			return static_cast<T&>(m_type).T::flowShaderProvider(voxel);
		}
		
		void buildVertexData(
				const VoxelChunkExtendedRef &chunk,
				const InChunkVoxelLocation &location,
				const FlowState &voxel,
				std::vector<VoxelVertexData> &data
		) {
			static_cast<T&>(m_type).T::flowBuildVertexData(chunk, location, voxel, data);
		}
		
		VoxelLightLevel lightLevel(const FlowState &voxel) {
			return static_cast<T&>(m_type).T::flowLightLevel(voxel);
		}
		
		void slowUpdate(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
				FlowState &voxel,
				std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
		) {
			static_cast<T&>(m_type).T::flowSlowUpdate(chunk, location, voxel, invalidatedLocations);
		}
		
		bool update(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
				FlowState &voxel,
				unsigned long deltaTime,
				std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
		) {
			return static_cast<T&>(m_type).T::flowUpdate(chunk, location, voxel, deltaTime, invalidatedLocations);
		}
	};
	
	struct State: public Data {
		uint8_t countdown = 0;
		
		template<typename S> void serialize(S &s) {
			Data::serialize(s);
			s.value1b(countdown);
		}
	};
	
	class VoxelType: public VoxelTypeHelper<T, State, Base> {
		FlowVoxelType *m_flowType;
		int m_maxFlowLevel;
		int m_flowSlowdown;
		bool m_canSpawn;
		bool m_flowRegistered = false;
		::VoxelType *m_air = nullptr;
		
	public:
		template<typename ...Args> explicit VoxelType(
				int maxFlowLevel, int flowSlowdown, bool canSpawn, Args&&... args
		): VoxelTypeHelper<T, State, Base>(args...), m_flowType(new FlowVoxelType(
				*this,
				std::forward<Args>(args)...
		)), m_maxFlowLevel(maxFlowLevel), m_flowSlowdown(flowSlowdown), m_canSpawn(canSpawn) {
			assert(maxFlowLevel < VOXEL_CHUNK_SIZE);
		}
		
		~VoxelType() override {
			if (m_flowRegistered) return;
			delete m_flowType;
		}
		
		void link(VoxelTypeRegistry &registry) override {
			VoxelTypeHelper<T, State, Base>::link(registry);
			m_air = &registry.get("air");
		}
		
		void registerChildren(const std::string &name, VoxelTypeRegistry &registry) override {
			assert(!m_flowRegistered);
			registry.add(name + "_flow", std::unique_ptr<FlowVoxelType>(m_flowType));
			m_flowRegistered = true;
		}
		
		[[nodiscard]] const FlowVoxelType &flowType() const {
			return m_flowType;
		}
		
		FlowVoxelType &flowType() {
			return m_flowType;
		}
		
		[[nodiscard]] int maxFlowLevel() const {
			return m_maxFlowLevel;
		}
		
		[[nodiscard]] int flowSlowdown() const {
			return m_flowSlowdown;
		}
		
		virtual std::string flowToString(const FlowState &voxel) {
			return static_cast<Base*>(m_flowType)->Base::toString(voxel) +
				"[level=" + std::to_string(voxel.level) + "]";
		}
		
		virtual const VoxelShaderProvider *flowShaderProvider(const FlowState &voxel) {
			return static_cast<Base*>(m_flowType)->Base::shaderProvider(voxel);
		}
		
		float calculateModelOffset(int level) {
			if (level == INT_MAX) {
				level = m_maxFlowLevel - 1;
			}
			return (float) (m_maxFlowLevel - (std::min(level, m_maxFlowLevel))) / m_maxFlowLevel;
		}
		
		float calculateModelOffset(const VoxelHolder &voxel) {
			if (&voxel.type() == this) {
				return calculateModelOffset(INT_MAX);
			} else if (&voxel.type() == m_flowType) {
				return calculateModelOffset(voxel.template get<FlowState>().level);
			} else {
				return 1.0f;
			}
		}
		
		void adjustMesh(
				const VoxelChunkExtendedRef &chunk,
				const InChunkVoxelLocation &location,
				int level,
				std::vector<VoxelVertexData> &data
		) {
			auto &posY = chunk.extendedAt(location.x, location.y + 1, location.z);
			if (&posY.type() == this || &posY.type() == m_flowType) return;
			
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
		
		void buildVertexData(
				const VoxelChunkExtendedRef &chunk,
				const InChunkVoxelLocation &location,
				const State &voxel,
				std::vector<VoxelVertexData> &data
		) {
			static_cast<Base*>(this)->Base::buildVertexData(chunk, location, voxel, data);
			adjustMesh(chunk, location, INT_MAX, data);
		}
		
		virtual void flowBuildVertexData(
				const VoxelChunkExtendedRef &chunk,
				const InChunkVoxelLocation &location,
				const FlowState &voxel,
				std::vector<VoxelVertexData> &data
		) {
			static_cast<Base*>(m_flowType)->Base::buildVertexData(chunk, location, voxel, data);
			adjustMesh(chunk, location, voxel.level, data);
		}
		
		virtual VoxelLightLevel flowLightLevel(const FlowState &voxel) {
			return static_cast<Base*>(m_flowType)->Base::lightLevel(voxel);
		}
		
		virtual void flowSlowUpdate(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
				FlowState &voxel,
				std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
		) {
			static_cast<Base*>(m_flowType)->Base::slowUpdate(chunk, location, voxel, invalidatedLocations);
		}
		
		virtual bool canFlow(
				Data &source,
				int sourceLevel,
				const VoxelHolder &target,
				int dy,
				bool strict
		) {
			if (dy > 0) {
				return false;
			}
			if (sourceLevel <= 1 && dy == 0) {
				return false;
			}
			if (&target.type() == m_flowType) {
				auto &flow = target.get<FlowState>();
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
		
		virtual void setFlow(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
				Data &source,
				int level
		) {
			assert(level >= 1);
			auto &voxel = chunk.extendedAt(location);
			voxel.setType(*m_flowType);
			voxel.template get<FlowState>().level = level;
		}
		
		virtual void setSource(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
				FlowState &flow
		) {
			chunk.extendedAt(location).setType(*this);
		}
		
		bool update(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
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
			if (canFlow(voxel, m_maxFlowLevel, chunk.extendedAt(bottom), -1, false)) {
				if (canFlow(voxel, m_maxFlowLevel, chunk.extendedAt(bottom), -1, true)) {
					setFlow(chunk, bottom, voxel, m_maxFlowLevel);
					invalidatedLocations.emplace(bottom);
				}
			} else {
				for (auto &offset : offsets) {
					InChunkVoxelLocation l(
							location.x + offset[0],
							location.y + offset[1],
							location.z + offset[2]
					);
					if (canFlow(voxel, m_maxFlowLevel, chunk.extendedAt(l), offset[1], true)) {
						setFlow(chunk, l, voxel, offset[1] >= 0 ? m_maxFlowLevel - 1 : m_maxFlowLevel);
						invalidatedLocations.emplace(l);
					}
				}
			}
			return false;
		}
		
		virtual bool flowUpdate(
				const VoxelChunkExtendedMutableRef &chunk,
				const InChunkVoxelLocation &location,
				FlowState &voxel,
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
					{0, 1, 0},
					{0, 0, 1}, {0, 0, -1}
			};
			bool sourceFound = false;
			int sourceCount = 0;
			auto &voxelHolder = chunk.extendedAt(location);
			for (auto &offset : offsets) {
				InChunkVoxelLocation l(location.x + offset[0], location.y + offset[1], location.z + offset[2]);
				auto &source = chunk.extendedAt(l);
				if (&source.type() == this) {
					if (canFlow(voxel, m_maxFlowLevel, voxelHolder, -offset[1], false)) {
						sourceFound = true;
						sourceCount++;
					}
				} else if (&source.type() == m_flowType) {
					if (canFlow(
							voxel,
							source.template get<FlowState>().level,
							voxelHolder,
							-offset[1],
							false
					)) {
						sourceFound = true;
					}
				}
			}
			InChunkVoxelLocation bottom(location.x, location.y - 1, location.z);
			if (canFlow(voxel, m_maxFlowLevel, chunk.extendedAt(bottom), -1, false)) {
				if (canFlow(voxel, voxel.level, chunk.extendedAt(bottom), -1, true)) {
					setFlow(chunk, bottom, voxel, m_maxFlowLevel);
					invalidatedLocations.emplace(bottom);
				}
			} else {
				for (auto &offset : offsets) {
					InChunkVoxelLocation l(location.x + offset[0], location.y + offset[1], location.z + offset[2]);
					if (canFlow(voxel, voxel.level, chunk.extendedAt(l), offset[1], true)) {
						setFlow(chunk, l, voxel, offset[1] >= 0 ? m_maxFlowLevel - 1 : m_maxFlowLevel);
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
				invalidatedLocations.template emplace(location);
			}
			return false;
		}
		
	};
	
};
