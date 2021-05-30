#pragma once

#include <cstddef>
#include <cmath>
#include <glm/vec3.hpp>

static const int VOXEL_CHUNK_SIZE = 16;

struct VoxelLocation;

struct VoxelChunkLocation {
	int x, y, z;
	
	VoxelChunkLocation() {
	}
	
	constexpr VoxelChunkLocation(int x, int y, int z): x(x), y(y), z(z) {
	}
	
	constexpr explicit VoxelChunkLocation(const VoxelLocation &location);
	
	constexpr bool operator==(const VoxelChunkLocation &location) const {
		return x == location.x && y == location.y && z == location.z;
	}
	
	constexpr bool operator!=(const VoxelChunkLocation &location) const {
		return !(*this == location);
	}
	
	template<typename S> void serialize(S &s) {
		s.value4b(x);
		s.value4b(y);
		s.value4b(z);
	}
	
};

struct InChunkVoxelLocation {
	int x, y, z;
	
	InChunkVoxelLocation() {
	}
	
	constexpr InChunkVoxelLocation(int x, int y, int z): x(x), y(y), z(z) {
	}
	
	explicit constexpr InChunkVoxelLocation(
			const glm::vec3 &v
	): x(roundf(v.x)), y(roundf(v.y)), z(roundf(v.z)) {
	}
	
	constexpr explicit InChunkVoxelLocation(const VoxelLocation &location);
	
	constexpr bool operator==(const InChunkVoxelLocation &location) const {
		return x == location.x && y == location.y && z == location.z;
	}
	
	constexpr bool operator!=(const InChunkVoxelLocation &location) const {
		return !(*this == location);
	}
	
	[[nodiscard]] glm::vec3 toVec3() const {
		return glm::vec3((float) x, (float) y, (float) z);
	}
	
	template<typename S> void serialize(S &s) {
		s.value4b(x);
		s.value4b(y);
		s.value4b(z);
	}
	
};

struct VoxelLocation {
	int x, y, z;
	
	VoxelLocation() {
	}
	
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
	
	constexpr bool operator!=(const VoxelLocation &location) const {
		return !(*this == location);
	}
	
	[[nodiscard]] constexpr VoxelChunkLocation chunk() const {
		return VoxelChunkLocation(*this);
	}
	
	[[nodiscard]] constexpr InChunkVoxelLocation inChunk() const {
		return InChunkVoxelLocation(*this);
	}
	
	template<typename S> void serialize(S &s) {
		s.value4b(x);
		s.value4b(y);
		s.value4b(z);
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
