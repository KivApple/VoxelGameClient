#pragma once

#include <cstddef>
#include "Voxel.h"
#include "VoxelLocation.h"

class VoxelChunk {
	VoxelChunkLocation m_location;
	VoxelHolder m_data[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE];
	bool m_lightComputed = false;
	
	static size_t voxelIndex(int x, int y, int z) {
		return z * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE + y * VOXEL_CHUNK_SIZE + x;
	}
	
public:
	explicit VoxelChunk(const VoxelChunkLocation &location);
	VoxelChunk(const VoxelChunk&) = delete;
	VoxelChunk &operator=(const VoxelChunk&) = delete;
	
	[[nodiscard]] const VoxelChunkLocation &location() const {
		return m_location;
	}
	
	[[nodiscard]] const VoxelHolder &at(int x, int y, int z) const {
		return m_data[voxelIndex(x, y, z)];
	}
	
	[[nodiscard]] const VoxelHolder &at(const InChunkVoxelLocation &location) const {
		return at(location.x, location.y, location.z);
	}
	
	[[nodiscard]] VoxelHolder &at(int x, int y, int z) {
		return m_data[voxelIndex(x, y, z)];
	}
	
	[[nodiscard]] VoxelHolder &at(const InChunkVoxelLocation &location) {
		return at(location.x, location.y, location.z);
	}

	[[nodiscard]] bool lightComputed() const {
		return m_lightComputed;
	}

	void setLightComputed(bool lightComputed) {
		m_lightComputed = lightComputed;
	}
	
	template<typename S> void serialize(S &s) const {
		s.container(m_data);
	}
	template<typename S> void serialize(S &s) {
		s.container(m_data);
	}

};
