#include "VoxelChunk.h"

VoxelChunk::VoxelChunk(const VoxelChunkLocation &location): m_location(location), m_data() {
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				initAtNoDestroy(x, y, z, EmptyVoxelType::INSTANCE);
			}
		}
	}
}

VoxelChunk::~VoxelChunk() {
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				auto &voxel = at(x, y, z);
				voxel.type.get().invokeDestroy(voxel);
			}
		}
	}
}

Voxel &VoxelChunk::initAtNoDestroy(int x, int y, int z, VoxelType &type) {
	return type.invokeInit(m_data + voxelDataOffset(x, y, z));
}

Voxel &VoxelChunk::initAt(int x, int y, int z, VoxelType &type) {
	auto &voxel = at(x, y, z);
	voxel.type.get().invokeDestroy(voxel);
	return initAtNoDestroy(x, y, z, type);
}
