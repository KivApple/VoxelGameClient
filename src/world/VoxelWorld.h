#pragma once

#include <unordered_map>
#include <shared_mutex>
#include "VoxelChunk.h"

class VoxelWorld;

class SharedVoxelChunk: public VoxelChunk {
	VoxelWorld &m_world;
	SharedVoxelChunk *m_neighbors[3 * 3 * 3] = {};
	std::shared_mutex m_mutex;
	bool m_dirty = false;
	
public:
	SharedVoxelChunk(VoxelWorld &world, const VoxelChunkLocation &location): VoxelChunk(location), m_world(world) {
	}
	
	void setNeighbors(const std::unordered_map<VoxelChunkLocation, std::unique_ptr<SharedVoxelChunk>> &chunks);
	void unsetNeighbors();
	
	[[nodiscard]] SharedVoxelChunk *neighbor(int dx, int dy, int dz) const;
	
	[[nodiscard]] std::shared_mutex &mutex() {
		return m_mutex;
	}
	
	[[nodiscard]] VoxelWorld &world() const {
		return m_world;
	}
	
	[[nodiscard]] bool dirty() const {
		return m_dirty;
	}
	
	void markDirty() {
		m_dirty = true;
	}
	
	void clearDirty() {
		m_dirty = false;
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
	[[nodiscard]] const VoxelHolder &at(int x, int y, int z) const {
		return m_chunk->at(x, y, z);
	}
	[[nodiscard]] const VoxelHolder &at(const InChunkVoxelLocation &location) const {
		return m_chunk->at(location);
	}
	template<typename S> void serialize(S &s) const {
		s.object(*m_chunk);
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
	[[nodiscard]] const VoxelHolder &extendedAt(
			int x, int y, int z,
			VoxelLocation *outLocation = nullptr
	) const;
	[[nodiscard]] const VoxelHolder &extendedAt(
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
	VoxelChunkMutableRef(VoxelChunkMutableRef &&ref) noexcept = default;
	VoxelChunkMutableRef &operator=(VoxelChunkMutableRef &&ref) noexcept;
	~VoxelChunkMutableRef();
	void unlock();
	[[nodiscard]] VoxelHolder &at(int x, int y, int z) const;
	[[nodiscard]] VoxelHolder &at(const InChunkVoxelLocation &location) const;
	template<typename S> void serialize(S &s) {
		s.object(*m_chunk);
	}
	void markDirty() {
		m_chunk->markDirty();
	}
	
};

class VoxelChunkExtendedMutableRef: public VoxelChunkMutableRef {
public:
	constexpr VoxelChunkExtendedMutableRef() = default;
	explicit VoxelChunkExtendedMutableRef(SharedVoxelChunk &chunk);
	VoxelChunkExtendedMutableRef(VoxelChunkExtendedMutableRef &&ref) noexcept = default;
	VoxelChunkExtendedMutableRef &operator=(VoxelChunkExtendedMutableRef &&ref) noexcept;
	~VoxelChunkExtendedMutableRef();
	void unlock();
	[[nodiscard]] VoxelHolder &extendedAt(
			int x, int y, int z,
			VoxelLocation *outLocation = nullptr
	) const;
	[[nodiscard]] VoxelHolder &extendedAt(
			const InChunkVoxelLocation &location,
			VoxelLocation *outLocation = nullptr
	) const;
	
};

class VoxelChunkLoader {
public:
	virtual ~VoxelChunkLoader() = default;
	virtual void load(VoxelChunkMutableRef &chunk) = 0;
	virtual void loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) = 0;
	
};

class VoxelChunkListener {
public:
	virtual ~VoxelChunkListener() = default;
	virtual void chunkInvalidated(const VoxelChunkLocation &location) = 0;

};

class VoxelWorld {
	VoxelChunkLoader *m_chunkLoader;
	VoxelChunkListener *m_chunkListener;
	std::unordered_map<VoxelChunkLocation, std::unique_ptr<SharedVoxelChunk>> m_chunks;
	std::shared_mutex m_mutex;
	
	template<typename T> T createChunk(const VoxelChunkLocation &location);
	template<typename T> T createAndLoadChunk(const VoxelChunkLocation &location);
	
	friend class VoxelChunkMutableRef;
	
public:
	enum class MissingChunkPolicy {
		NONE,
		CREATE,
		LOAD,
		LOAD_ASYNC
	};
	
	explicit VoxelWorld(VoxelChunkLoader *chunkLoader = nullptr, VoxelChunkListener *chunkListener = nullptr);
	VoxelWorld(const VoxelWorld &world) = delete;
	VoxelWorld &operator=(const VoxelWorld &world) = delete;
	VoxelChunkRef chunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE
	);
	VoxelChunkExtendedRef extendedChunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE
	);
	VoxelChunkMutableRef mutableChunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE
	);
	VoxelChunkExtendedMutableRef extendedMutableChunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE
	);
	void unloadChunks(const std::vector<VoxelChunkLocation> &locations);
	size_t chunkCount() const {
		return m_chunks.size();
	}
	
};
