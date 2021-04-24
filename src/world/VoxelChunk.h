#pragma once

#include <cstddef>
#include "Voxel.h"
#include "VoxelLocation.h"

class VoxelChunk {
	VoxelChunkLocation m_location;
	char m_data[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_DATA_SIZE];
	
	
	static size_t voxelDataOffset(int x, int y, int z) {
		return (z * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE + y * VOXEL_CHUNK_SIZE + z) * VOXEL_DATA_SIZE;
	}
	
	Voxel &initAtNoDestroy(int x, int y, int z, VoxelType &type);
	
public:
	explicit VoxelChunk(const VoxelChunkLocation &location);
	VoxelChunk(const VoxelChunk&) = delete;
	VoxelChunk &operator=(const VoxelChunk&) = delete;
	~VoxelChunk();
	
	[[nodiscard]] const VoxelChunkLocation &location() const {
		return m_location;
	}
	
	[[nodiscard]] const Voxel &at(int x, int y, int z) const {
		return *((const Voxel*) (m_data + voxelDataOffset(x, y, z)));
	}
	
	[[nodiscard]] const Voxel &at(const InChunkVoxelLocation &location) const {
		return at(location.x, location.y, location.z);
	}
	
	[[nodiscard]] Voxel &at(int x, int y, int z) {
		return *((Voxel*) (m_data + voxelDataOffset(x, y, z)));
	}
	
	[[nodiscard]] Voxel &at(const InChunkVoxelLocation &location) {
		return at(location.x, location.y, location.z);
	}
	
	Voxel &initAt(int x, int y, int z, VoxelType &type);

};
