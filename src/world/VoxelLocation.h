#pragma once

#include <cstddef>

static const int VOXEL_CHUNK_SIZE = 16;

struct VoxelLocation;

struct VoxelChunkLocation {
	int x, y, z;
	
	constexpr VoxelChunkLocation(int x, int y, int z): x(x), y(y), z(z) {
	}
	
	constexpr explicit VoxelChunkLocation(const VoxelLocation &location);
	
	constexpr bool operator==(const VoxelChunkLocation &location) const {
		return x == location.x && y == location.y && z == location.z;
	}
	
};

struct InChunkVoxelLocation {
	int x, y, z;
	
	constexpr InChunkVoxelLocation(int x, int y, int z): x(x), y(y), z(z) {
	}
	
	constexpr explicit InChunkVoxelLocation(const VoxelLocation &location);
	
	constexpr bool operator==(const InChunkVoxelLocation &location) const {
		return x == location.x && y == location.y && z == location.z;
	}
	
};

struct VoxelLocation {
	int x, y, z;
	
	constexpr VoxelLocation(int x, int y, int z): x(x), y(y), z(z) {
	}
	
	constexpr VoxelLocation(
			const VoxelChunkLocation &chunk,
			const InChunkVoxelLocation &inChunk
	): x(chunk.x * VOXEL_CHUNK_SIZE + inChunk.x), y(chunk.y * VOXEL_CHUNK_SIZE + inChunk.y),
		z(chunk.z * VOXEL_CHUNK_SIZE + inChunk.z) {
	}
	
	constexpr bool operator==(const VoxelLocation &location) const {
		return x == location.x && y == location.y && z == location.z;
	}
	
	[[nodiscard]] constexpr VoxelChunkLocation chunk() const {
		return VoxelChunkLocation(*this);
	}
	
	[[nodiscard]] constexpr InChunkVoxelLocation inChunk() const {
		return InChunkVoxelLocation(*this);
	}
	
};

constexpr VoxelChunkLocation::VoxelChunkLocation(const VoxelLocation &location): VoxelChunkLocation(
		(location.x < 0 ? location.x - VOXEL_CHUNK_SIZE + 1 : location.x) / VOXEL_CHUNK_SIZE,
		(location.y < 0 ? location.y - VOXEL_CHUNK_SIZE + 1 : location.y) / VOXEL_CHUNK_SIZE,
		(location.z < 0 ? location.z - VOXEL_CHUNK_SIZE + 1 : location.z) / VOXEL_CHUNK_SIZE
) {
}

constexpr InChunkVoxelLocation::InChunkVoxelLocation(const VoxelLocation &location): InChunkVoxelLocation(
		location.x >= 0 ?
		location. x % VOXEL_CHUNK_SIZE :
		(VOXEL_CHUNK_SIZE + location.x % VOXEL_CHUNK_SIZE) % VOXEL_CHUNK_SIZE,
		location.y >= 0 ?
		location.y % VOXEL_CHUNK_SIZE :
		(VOXEL_CHUNK_SIZE + location.y % VOXEL_CHUNK_SIZE) % VOXEL_CHUNK_SIZE,
		location.z >= 0 ?
		location.z % VOXEL_CHUNK_SIZE :
		(VOXEL_CHUNK_SIZE + location.z % VOXEL_CHUNK_SIZE) % VOXEL_CHUNK_SIZE
) {
}

namespace std {
	template<> struct hash<VoxelChunkLocation> {
		constexpr std::size_t operator()(const VoxelChunkLocation &key) const {
			return (key.x << 24) ^ (key.y << 16) ^ (key.z << 8);
		}
	};
	
	template<> struct hash<InChunkVoxelLocation> {
		constexpr std::size_t operator()(const InChunkVoxelLocation &key) const {
			return (key.x << 24) ^ (key.y << 16) ^ (key.z << 8);
		}
	};
	
	template<> struct hash<VoxelLocation> {
		constexpr std::size_t operator()(const VoxelLocation &key) const {
			return (key.x << 24) ^ (key.y << 16) ^ (key.z << 8);
		}
	};
}
