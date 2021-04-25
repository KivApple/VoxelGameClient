#pragma once

#include <unordered_map>
#include <shared_mutex>
#include "VoxelChunk.h"

class SharedVoxelChunk: public VoxelChunk {
	SharedVoxelChunk *m_neighbors[3 * 3 * 3] = {};
	std::shared_mutex m_mutex;
	
public:
	explicit SharedVoxelChunk(const VoxelChunkLocation &location): VoxelChunk(location) {
	}
	
	void setNeighbors(const std::unordered_map<VoxelChunkLocation, std::unique_ptr<SharedVoxelChunk>> &chunks);
	
	[[nodiscard]] SharedVoxelChunk *neighbor(int dx, int dy, int dz) const;
	
	[[nodiscard]] std::shared_mutex &mutex() {
		return m_mutex;
	}
	
};

class VoxelChunkRef {

protected:
	SharedVoxelChunk *m_chunk;
	
	VoxelChunkRef(SharedVoxelChunk &chunk, bool lock);
	
public:
	constexpr VoxelChunkRef(): m_chunk(nullptr) {
	}
	explicit VoxelChunkRef(SharedVoxelChunk &chunk);
	VoxelChunkRef(VoxelChunkRef &&ref) noexcept;
	VoxelChunkRef &operator=(VoxelChunkRef &&ref) noexcept;
	~VoxelChunkRef();
	void unlock();
	operator bool() const {
		return m_chunk != nullptr;
	}
	[[nodiscard]] const VoxelChunkLocation &location() const {
		return m_chunk->location();
	}
	[[nodiscard]] const Voxel &at(int x, int y, int z) const {
		return m_chunk->at(x, y, z);
	}
	[[nodiscard]] const Voxel &at(const InChunkVoxelLocation &location) const {
		return m_chunk->at(location);
	}
	
};

class VoxelChunkExtendedRef: public VoxelChunkRef {
protected:
	SharedVoxelChunk *m_neighbors[3 * 3 * 3];

	VoxelChunkExtendedRef(SharedVoxelChunk &chunk, bool lock, bool lockNeighbors);
	
public:
	constexpr VoxelChunkExtendedRef(): m_neighbors() {
	}
	explicit VoxelChunkExtendedRef(SharedVoxelChunk &chunk);
	VoxelChunkExtendedRef(VoxelChunkExtendedRef &&ref) noexcept;
	VoxelChunkExtendedRef &operator=(VoxelChunkExtendedRef &&ref) noexcept;
	~VoxelChunkExtendedRef();
	void unlock();
	[[nodiscard]] bool hasNeighbor(int dx, int dy, int dz) const;
	[[nodiscard]] const Voxel &extendedAt(
			int x, int y, int z,
			VoxelLocation *outLocation = nullptr
	) const;
	[[nodiscard]] const Voxel &extendedAt(
			const InChunkVoxelLocation &location,
			VoxelLocation *outLocation = nullptr
	) const;

};

class VoxelChunkMutableRef: public VoxelChunkExtendedRef {
protected:
	VoxelChunkMutableRef(SharedVoxelChunk &chunk, bool lockNeighbors);

public:
	constexpr VoxelChunkMutableRef() = default;
	explicit VoxelChunkMutableRef(SharedVoxelChunk &chunk);
	VoxelChunkMutableRef &operator=(VoxelChunkMutableRef &&ref) noexcept;
	~VoxelChunkMutableRef();
	void unlock();
	[[nodiscard]] Voxel &at(int x, int y, int z) const;
	[[nodiscard]] Voxel &at(const InChunkVoxelLocation &location) const;
	
};

class VoxelChunkExtendedMutableRef: public VoxelChunkMutableRef {
public:
	constexpr VoxelChunkExtendedMutableRef() = default;
	explicit VoxelChunkExtendedMutableRef(SharedVoxelChunk &chunk);
	VoxelChunkExtendedMutableRef &operator=(VoxelChunkExtendedMutableRef &&ref) noexcept;
	~VoxelChunkExtendedMutableRef();
	void unlock();
	[[nodiscard]] Voxel &extendedAt(
			int x, int y, int z,
			VoxelLocation *outLocation = nullptr
	) const;
	[[nodiscard]] Voxel &extendedAt(
			const InChunkVoxelLocation &location,
			VoxelLocation *outLocation = nullptr
	) const;
	
};

class VoxelWorld {
	std::unordered_map<VoxelChunkLocation, std::unique_ptr<SharedVoxelChunk>> m_chunks;
	std::shared_mutex m_mutex;
	
public:
	VoxelWorld() = default;
	VoxelWorld(const VoxelWorld &world) = delete;
	VoxelWorld &operator=(const VoxelWorld &world) = delete;
	VoxelChunkRef chunk(const VoxelChunkLocation &location);
	VoxelChunkExtendedRef extendedChunk(const VoxelChunkLocation &location);
	VoxelChunkMutableRef mutableChunk(const VoxelChunkLocation &location, bool create = false);
	VoxelChunkExtendedMutableRef extendedMutableChunk(const VoxelChunkLocation &location, bool create = false);
	size_t chunkCount() const {
		return m_chunks.size();
	}
	
};
