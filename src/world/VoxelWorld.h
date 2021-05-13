#pragma once

#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include "VoxelChunk.h"

class VoxelWorld;

enum class VoxelChunkLightState {
	PENDING_INITIAL,
	PENDING_INCREMENTAL,
	COMPUTING,
	READY,
	COMPLETE
};

class SharedVoxelChunk: public VoxelChunk {
	VoxelWorld &m_world;
	SharedVoxelChunk *m_neighbors[3 * 3 * 3] = {};
	std::shared_mutex m_mutex;
	VoxelChunkLightState m_lightState = VoxelChunkLightState::PENDING_INITIAL;
	std::unordered_set<InChunkVoxelLocation> m_dirtyLocations;
	std::unordered_set<InChunkVoxelLocation> m_pendingLocations;
	bool m_pendingInitialUpdate = true;
	bool m_unloading = false;
	unsigned long m_updatedAt = 0;
	long m_storedAt = 0;
	
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
	
	[[nodiscard]] VoxelChunkLightState lightState() const {
		return m_lightState;
	}
	
	void setLightState(VoxelChunkLightState state) {
		m_lightState = state;
	}
	
	void invalidateLight() {
		switch (m_lightState) {
			case VoxelChunkLightState::PENDING_INITIAL:
			case VoxelChunkLightState::PENDING_INCREMENTAL:
				break;
			case VoxelChunkLightState::COMPUTING:
			case VoxelChunkLightState::READY:
			case VoxelChunkLightState::COMPLETE:
				m_lightState = VoxelChunkLightState::PENDING_INCREMENTAL;
				break;
		}
	}
	
	void invalidateStorage() {
		if (m_storedAt >= (long) m_updatedAt) {
			m_storedAt = m_updatedAt - 1;
		}
	}
	
	[[nodiscard]] const std::unordered_set<InChunkVoxelLocation> &dirtyLocations() const {
		return m_dirtyLocations;
	}
	
	void markDirty(const InChunkVoxelLocation &location) {
		m_dirtyLocations.emplace(location);
		invalidateStorage();
		invalidateLight();
	}
	
	void clearDirtyLocations() {
		m_dirtyLocations.clear();
	}
	
	void markPending(const InChunkVoxelLocation &location) {
		m_pendingLocations.emplace(location);
	}
	
	void clearPending() {
		m_pendingLocations.clear();
	}
	
	const std::unordered_set<InChunkVoxelLocation> &pendingLocations() const {
		return m_pendingLocations;
	}
	
	std::unordered_set<InChunkVoxelLocation> takePendingLocations() {
		return std::move(m_pendingLocations);
	}
	
	[[nodiscard]] bool pendingInitialUpdate() const {
		return m_pendingInitialUpdate;
	}
	
	void setPendingInitialUpdate(bool pendingInitialUpdate) {
		m_pendingInitialUpdate = pendingInitialUpdate;
	}
	
	[[nodiscard]] unsigned long updatedAt() const {
		return m_updatedAt;
	}
	
	void setUpdatedAt(unsigned long updatedAt) {
		m_updatedAt = updatedAt;
	}
	
	[[nodiscard]] long storedAt() const {
		return m_storedAt;
	}
	
	void setStoredAt(long storedAt) {
		m_storedAt = storedAt;
	}
	
	[[nodiscard]] bool unloading() const {
		return m_unloading;
	}
	
	void setUnloading(bool unloading) {
		m_unloading = unloading;
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
	[[nodiscard]] VoxelChunkLightState lightState() const {
		return m_chunk->lightState();
	}
	[[nodiscard]] unsigned long updatedAt() const {
		return m_chunk->storedAt();
	}
	[[nodiscard]] long storedAt() const {
		return m_chunk->storedAt();
	}
	[[nodiscard]] unsigned int pendingVoxelCount() const {
		return m_chunk->pendingLocations().size();
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
	const VoxelHolder &extendedAt(
			int x, int y, int z,
			VoxelLocation *outLocation = nullptr
	) const;
	const VoxelHolder &extendedAt(
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
	void setLightState(VoxelChunkLightState state) const {
		m_chunk->setLightState(state);
	}
	const std::unordered_set<InChunkVoxelLocation> &dirtyLocations() const {
		return m_chunk->dirtyLocations();
	}
	void clearDirtyLocations() {
		m_chunk->clearDirtyLocations();
	}
	void markDirty(const InChunkVoxelLocation &location, bool markPending = true);
	void markPending(const InChunkVoxelLocation &location);
	void extendedMarkPending(const InChunkVoxelLocation &location);
	void setUpdatedAt(unsigned long time) {
		m_chunk->setUpdatedAt(time);
	}
	void setStoredAt(long time) {
		m_chunk->setStoredAt(time);
	}
	void invalidateStorage() {
		m_chunk->invalidateStorage();
	}

	template<typename S> void serialize(S &s) {
		s.object(*m_chunk);
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
	VoxelHolder &extendedAt(
			int x, int y, int z,
			VoxelLocation *outLocation = nullptr
	) const;
	VoxelHolder &extendedAt(
			const InChunkVoxelLocation &location,
			VoxelLocation *outLocation = nullptr
	) const;
	void extendedMarkDirty(const InChunkVoxelLocation &location, bool markPending = true);
	void update(unsigned long time);
	
};

class VoxelChunkLoader {
public:
	virtual ~VoxelChunkLoader() = default;
	virtual void load(VoxelChunkMutableRef &chunk) = 0;
	virtual void loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) = 0;
	virtual void cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) = 0;
	virtual void storeChunkAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	}
	
};

class VoxelChunkListener {
public:
	virtual ~VoxelChunkListener() = default;
	virtual void chunkUnlocked(const VoxelChunkLocation &chunkLocation, VoxelChunkLightState lightState) = 0;

};

class VoxelWorld {
	VoxelChunkLoader *m_chunkLoader = nullptr;
	VoxelChunkListener *m_chunkListener;
	std::unordered_map<VoxelChunkLocation, std::unique_ptr<SharedVoxelChunk>> m_chunks;
	std::mutex m_mutex;
	
	template<typename T> T createChunk(const VoxelChunkLocation &location);
	template<typename T> T createAndLoadChunk(const VoxelChunkLocation &location, std::unique_lock<std::mutex> &lock);
	
	friend class VoxelInvalidationNotifier;
	
public:
	enum class MissingChunkPolicy {
		NONE,
		CREATE,
		LOAD,
		LOAD_ASYNC
	};
	
	explicit VoxelWorld(VoxelChunkListener *chunkListener = nullptr);
	VoxelWorld(const VoxelWorld &world) = delete;
	VoxelWorld &operator=(const VoxelWorld &world) = delete;
	void setChunkLoader(VoxelChunkLoader *chunkLoader);
	VoxelChunkRef chunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE,
			bool *created = nullptr
	);
	VoxelChunkExtendedRef extendedChunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE,
			bool *created = nullptr
	);
	VoxelChunkMutableRef mutableChunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE,
			bool *created = nullptr
	);
	VoxelChunkExtendedMutableRef extendedMutableChunk(
			const VoxelChunkLocation &location,
			MissingChunkPolicy policy = MissingChunkPolicy::NONE,
			bool *created = nullptr
	);
	void storeChunk(const VoxelChunkLocation &location);
	void unloadChunks(const std::vector<VoxelChunkLocation> &locations);
	void unload();
	size_t chunkCount() const {
		return m_chunks.size();
	}
	template<typename Callable> void forEachChunkLocation(Callable callable) {
		std::unique_lock<std::mutex> lock(m_mutex);
		for (auto &chunk : m_chunks) {
			if (chunk.second->unloading()) continue;
			callable(chunk.first);
		}
	}
	void chunkStored(const VoxelChunkLocation &location);
	
};
