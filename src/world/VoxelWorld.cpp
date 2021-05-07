#include <vector>
#include <easylogging++.h>
#include "VoxelWorld.h"

#define TRACE_LOCKS 0

class VoxelInvalidationNotifier {
	VoxelWorld &m_world;
	VoxelChunkLocation m_chunkLocation;
	bool m_dirty;
	std::vector<InChunkVoxelLocation> m_locations;
	bool m_lightComputed;
	
public:
	explicit VoxelInvalidationNotifier(
			SharedVoxelChunk &chunk, bool enabled
	): m_world(chunk.world()), m_chunkLocation(chunk.location()), m_dirty(
			enabled && chunk.dirty()
	), m_lightComputed(chunk.lightComputed() && chunk.dirtyLocations().empty()) {
		if (enabled) {
			m_locations.reserve(chunk.dirtyLocations().size());
			for (auto &location : chunk.dirtyLocations()) {
				m_locations.emplace_back(location);
			}
			chunk.clearDirty();
			chunk.setLightComputed(m_lightComputed);
		}
	}
	
	void notify() {
		if (!m_dirty) return;
		auto &l = m_chunkLocation;
		LOG(TRACE) << "Chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z << " invalidated";
		m_world.m_chunkListener->chunkInvalidated(m_chunkLocation, std::move(m_locations), m_lightComputed);
		m_dirty = false;
	}
	
};

/* SharedVoxelChunk */

void SharedVoxelChunk::setNeighbors(
		const std::unordered_map<VoxelChunkLocation, std::unique_ptr<SharedVoxelChunk>> &chunks
) {
	for (int dz = -1; dz <= 1; dz++) {
		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				SharedVoxelChunk *chunk = nullptr;
				if (dx != 0 || dy != 0 || dz != 0) {
					auto it = chunks.find({
						location().x + dx,
						location().y + dy,
						location().z + dz
					});
					if (it == chunks.end()) continue;
					chunk = it->second.get();
					chunk->m_neighbors[(-dx + 1) + (-dy + 1) * 3 + (-dz + 1) * 3 * 3] = this;
				}
				m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3] = chunk;
			}
		}
	}
}

void SharedVoxelChunk::unsetNeighbors() {
	for (int dz = -1; dz <= 1; dz++) {
		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				auto chunk = m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3];
				if (chunk == nullptr) continue;
				chunk->m_neighbors[(-dx + 1) + (-dy + 1) * 3 + (-dz + 1) * 3 * 3] = nullptr;
			}
		}
	}
}

SharedVoxelChunk *SharedVoxelChunk::neighbor(int dx, int dy, int dz) const {
	return m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3];
}

/* VoxelChunkRef */

VoxelChunkRef::VoxelChunkRef(SharedVoxelChunk &chunk, bool lock): m_chunk(&chunk) {
	if (lock) {
		auto &l = chunk.location();
		LOG_IF(TRACE_LOCKS, TRACE) << "Lock shared x=" << l.x << ",y=" << l.y << ",z=" << l.z;
		chunk.mutex().lock_shared();
	}
}

VoxelChunkRef::VoxelChunkRef(SharedVoxelChunk &chunk): VoxelChunkRef(chunk, true) {
}

VoxelChunkRef::VoxelChunkRef(VoxelChunkRef &&ref) noexcept: VoxelChunkRef(*ref.m_chunk, false) {
	ref.m_chunk = nullptr;
}

VoxelChunkRef &VoxelChunkRef::operator=(VoxelChunkRef &&ref) noexcept {
	unlock();
	m_chunk = ref.m_chunk;
	ref.m_chunk = nullptr;
	return *this;
}

VoxelChunkRef::~VoxelChunkRef() {
	unlock();
}

void VoxelChunkRef::unlock(bool notify) {
	if (m_chunk) {
		VoxelInvalidationNotifier notifier(*m_chunk, notify);
		{
			auto &l2 = m_chunk->location();
			LOG_IF(TRACE_LOCKS, TRACE) << "Unlock shared x=" << l2.x << ",y=" << l2.y << ",z=" << l2.z;
		}
		m_chunk->mutex().unlock_shared();
		m_chunk = nullptr;
		notifier.notify();
	}
}

/* VoxelChunkExtendedRef */

VoxelChunkExtendedRef::VoxelChunkExtendedRef(
		SharedVoxelChunk &chunk,
		bool lock,
		bool lockNeighbors
): VoxelChunkRef(chunk, lock) {
	for (int dz = -1; dz <= 1; dz++) {
		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				auto neighbor = chunk.neighbor(dx, dy, dz);
				m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3] = neighbor;
				if (neighbor == nullptr || !lockNeighbors) continue;
				auto &l = neighbor->location();
				LOG_IF(TRACE_LOCKS, TRACE) << "Lock shared x=" << l.x << ",y=" << l.y << ",z=" << l.z;
				neighbor->mutex().lock_shared();
			}
		}
	}
}

VoxelChunkExtendedRef::VoxelChunkExtendedRef(
		SharedVoxelChunk &chunk
): VoxelChunkExtendedRef(chunk, true, true) {
}

VoxelChunkExtendedRef::VoxelChunkExtendedRef(VoxelChunkExtendedRef &&ref) noexcept: VoxelChunkRef(std::move(ref)) {
	for (int i = 0; i < sizeof(m_neighbors) / sizeof(m_neighbors[0]); i++) {
		m_neighbors[i] = ref.m_neighbors[i];
		ref.m_neighbors[i] = nullptr;
	}
}

VoxelChunkExtendedRef &VoxelChunkExtendedRef::operator=(VoxelChunkExtendedRef &&ref) noexcept {
	unlock();
	for (int i = 0; i < sizeof(m_neighbors) / sizeof(m_neighbors[0]); i++) {
		m_neighbors[i] = ref.m_neighbors[i];
		ref.m_neighbors[i] = nullptr;
	}
	VoxelChunkRef::operator=(std::move(ref));
	return *this;
}

VoxelChunkExtendedRef::~VoxelChunkExtendedRef() {
	unlock();
}

void VoxelChunkExtendedRef::unlock(bool notify) {
	for (auto &&neighbor : m_neighbors) {
		if (neighbor == nullptr) continue;
		auto &l = neighbor->location();
		LOG_IF(TRACE_LOCKS, TRACE) << "Unlock shared x=" << l.x << ",y=" << l.y << ",z=" << l.z;
		neighbor->mutex().unlock_shared();
		neighbor = nullptr;
	}
	VoxelChunkRef::unlock(notify);
}

bool VoxelChunkExtendedRef::hasNeighbor(int dx, int dy, int dz) const {
	return m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3] != nullptr;
}

const VoxelHolder &VoxelChunkExtendedRef::extendedAt(int x, int y, int z, VoxelLocation *outLocation) const {
	return extendedAt({x, y, z}, outLocation);
}

const VoxelHolder &VoxelChunkExtendedRef::extendedAt(const InChunkVoxelLocation &location, VoxelLocation *outLocation) const {
	assert(location.x >= -VOXEL_CHUNK_SIZE && location.x < 2 * VOXEL_CHUNK_SIZE);
	assert(location.y >= -VOXEL_CHUNK_SIZE && location.y < 2 * VOXEL_CHUNK_SIZE);
	assert(location.z >= -VOXEL_CHUNK_SIZE && location.z < 2 * VOXEL_CHUNK_SIZE);
	
	const SharedVoxelChunk *chunk = m_chunk;
	VoxelChunkLocation chunkLocation = this->location();
	InChunkVoxelLocation correctedLocation = location;
	if (location.x < 0) {
		chunkLocation.x--;
		correctedLocation.x += VOXEL_CHUNK_SIZE;
	} else if (location.x >= VOXEL_CHUNK_SIZE) {
		chunkLocation.x++;
		correctedLocation.x -= VOXEL_CHUNK_SIZE;
	}
	if (location.y < 0) {
		chunkLocation.y--;
		correctedLocation.y += VOXEL_CHUNK_SIZE;
	} else if (location.y >= VOXEL_CHUNK_SIZE) {
		chunkLocation.y++;
		correctedLocation.y -= VOXEL_CHUNK_SIZE;
	}
	if (location.z < 0) {
		chunkLocation.z--;
		correctedLocation.z += VOXEL_CHUNK_SIZE;
	} else if (location.z >= VOXEL_CHUNK_SIZE) {
		chunkLocation.z++;
		correctedLocation.z -= VOXEL_CHUNK_SIZE;
	}
	int dx = chunkLocation.x - this->location().x;
	int dy = chunkLocation.y - this->location().y;
	int dz = chunkLocation.z - this->location().z;
	if (dx != 0 || dy != 0 || dz != 0) {
		chunk = m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3];
	}
	if (outLocation != nullptr) {
		*outLocation = VoxelLocation(chunkLocation, correctedLocation);
	}
	if (chunk) {
		return chunk->at(correctedLocation);
	}
	static const VoxelHolder empty;
	return empty;
}

/* VoxelChunkMutableRef */

VoxelChunkMutableRef::VoxelChunkMutableRef(
		SharedVoxelChunk &chunk,
		bool lockNeighbors
): VoxelChunkExtendedRef(chunk, false, lockNeighbors) {
	auto &l = chunk.location();
	LOG_IF(TRACE_LOCKS, TRACE) << "Lock x=" << l.x << ",y=" << l.y << ",z=" << l.z;
	chunk.mutex().lock();
}

VoxelChunkMutableRef::VoxelChunkMutableRef(SharedVoxelChunk &chunk): VoxelChunkMutableRef(chunk, true) {
}

VoxelChunkMutableRef &VoxelChunkMutableRef::operator=(VoxelChunkMutableRef &&ref) noexcept {
	unlock();
	VoxelChunkExtendedRef::operator=(std::move(ref));
	return *this;
}

VoxelChunkMutableRef::~VoxelChunkMutableRef() {
	unlock();
}

void VoxelChunkMutableRef::unlock(bool notify) {
	std::optional<VoxelInvalidationNotifier> notifier;
	if (m_chunk) {
		notifier.emplace(*m_chunk, notify);
		m_chunk->mutex().unlock();
		m_chunk = nullptr;
	}
	VoxelChunkExtendedRef::unlock();
	if (notifier.has_value()) {
		notifier->notify();
	}
}

VoxelHolder &VoxelChunkMutableRef::at(int x, int y, int z) const {
	return m_chunk->at(x, y, z);
}

VoxelHolder &VoxelChunkMutableRef::at(const InChunkVoxelLocation &location) const {
	return m_chunk->at(location);
}

/* VoxelChunkExtendedMutableRef */

VoxelChunkExtendedMutableRef::VoxelChunkExtendedMutableRef(
		SharedVoxelChunk &chunk
): VoxelChunkMutableRef(chunk, false) {
	for (auto neighbor : m_neighbors) {
		if (neighbor == nullptr) continue;
		auto &l = neighbor->location();
		LOG_IF(TRACE_LOCKS, TRACE) << "Lock x=" << l.x << ",y=" << l.y << ",z=" << l.z;
		neighbor->mutex().lock();
	}
}

VoxelChunkExtendedMutableRef &VoxelChunkExtendedMutableRef::operator=(VoxelChunkExtendedMutableRef &&ref) noexcept {
	unlock();
	VoxelChunkMutableRef::operator=(std::move(ref));
	return *this;
}

VoxelChunkExtendedMutableRef::~VoxelChunkExtendedMutableRef() {
	unlock();
}

void VoxelChunkExtendedMutableRef::unlock(bool notify) {
	for (auto &&neighbor : m_neighbors) {
		if (neighbor == nullptr) continue;
		auto &l = neighbor->location();
		LOG_IF(TRACE_LOCKS, TRACE) << "Unlock x=" << l.x << ",y=" << l.y << ",z=" << l.z;
		neighbor->mutex().unlock();
		neighbor = nullptr;
	}
	VoxelChunkMutableRef::unlock(notify);
}

VoxelHolder &VoxelChunkExtendedMutableRef::extendedAt(int x, int y, int z, VoxelLocation *outLocation) const {
	return extendedAt({x, y, z}, outLocation);
}

VoxelHolder &VoxelChunkExtendedMutableRef::extendedAt(
		const InChunkVoxelLocation &location,
		VoxelLocation *outLocation
) const {
	assert(location.x >= -VOXEL_CHUNK_SIZE && location.x < 2 * VOXEL_CHUNK_SIZE);
	assert(location.y >= -VOXEL_CHUNK_SIZE && location.y < 2 * VOXEL_CHUNK_SIZE);
	assert(location.z >= -VOXEL_CHUNK_SIZE && location.z < 2 * VOXEL_CHUNK_SIZE);
	
	SharedVoxelChunk *chunk = m_chunk;
	VoxelChunkLocation chunkLocation = this->location();
	InChunkVoxelLocation correctedLocation = location;
	if (location.x < 0) {
		chunkLocation.x--;
		correctedLocation.x += VOXEL_CHUNK_SIZE;
	} else if (location.x >= VOXEL_CHUNK_SIZE) {
		chunkLocation.x++;
		correctedLocation.x -= VOXEL_CHUNK_SIZE;
	}
	if (location.y < 0) {
		chunkLocation.y--;
		correctedLocation.y += VOXEL_CHUNK_SIZE;
	} else if (location.y >= VOXEL_CHUNK_SIZE) {
		chunkLocation.y++;
		correctedLocation.y -= VOXEL_CHUNK_SIZE;
	}
	if (location.z < 0) {
		chunkLocation.z--;
		correctedLocation.z += VOXEL_CHUNK_SIZE;
	} else if (location.z >= VOXEL_CHUNK_SIZE) {
		chunkLocation.z++;
		correctedLocation.z -= VOXEL_CHUNK_SIZE;
	}
	int dx = chunkLocation.x - this->location().x;
	int dy = chunkLocation.y - this->location().y;
	int dz = chunkLocation.z - this->location().z;
	if (dx != 0 || dy != 0 || dz != 0) {
		chunk = m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3];
	}
	if (outLocation != nullptr) {
		*outLocation = VoxelLocation(chunkLocation, correctedLocation);
	}
	if (chunk) {
		return chunk->at(correctedLocation);
	}
	static VoxelHolder empty;
	return empty;
}

/* VoxelWorld */

VoxelWorld::VoxelWorld(VoxelChunkListener *chunkListener): m_chunkListener(chunkListener) {
}

void VoxelWorld::setChunkLoader(VoxelChunkLoader *chunkLoader) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_chunkLoader = chunkLoader;
}

template<typename T> T VoxelWorld::createChunk(const VoxelChunkLocation &location) {
	if (m_chunkLoader) {
		m_chunkLoader->cancelLoadAsync(*this, location);
	}
	auto it = m_chunks.find(location);
	if (it != m_chunks.end()) {
		return T(*it->second);
	}
	auto chunkPtr = std::make_unique<SharedVoxelChunk>(*this, location);
	auto &chunk = *chunkPtr;
	chunk.setNeighbors(m_chunks);
	m_chunks.emplace(location, std::move(chunkPtr));
	return T(chunk);
}

template<typename T> T VoxelWorld::createAndLoadChunk(const VoxelChunkLocation &location, std::unique_lock<std::mutex> &lock) {
	auto chunk = createChunk<T>(location);
	lock.unlock();
	m_chunkLoader->load(chunk);
	return chunk;
}

VoxelChunkRef VoxelWorld::chunk(
		const VoxelChunkLocation &location,
		MissingChunkPolicy policy,
		bool *created
) {
	if (created) {
		*created = false;
	}
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		switch (policy) {
			case MissingChunkPolicy::NONE:
				return VoxelChunkRef();
			case MissingChunkPolicy::CREATE:
				if (created) {
					*created = true;
				}
				return createChunk<VoxelChunkRef>(location);
			case MissingChunkPolicy::LOAD:
				if (created) {
					*created = true;
				}
				createAndLoadChunk<VoxelChunkMutableRef>(location, lock);
				return chunk(location, MissingChunkPolicy::NONE);
			case MissingChunkPolicy::LOAD_ASYNC:
				lock.unlock();
				m_chunkLoader->loadAsync(*this, location);
				return VoxelChunkRef();
		}
	}
	return VoxelChunkRef(*it->second);
}

VoxelChunkExtendedRef VoxelWorld::extendedChunk(
		const VoxelChunkLocation &location,
		MissingChunkPolicy policy,
		bool *created
) {
	if (created) {
		*created = false;
	}
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		switch (policy) {
			case MissingChunkPolicy::NONE:
				return VoxelChunkExtendedRef();
			case MissingChunkPolicy::CREATE:
				if (created) {
					*created = true;
				}
				return createChunk<VoxelChunkExtendedRef>(location);
			case MissingChunkPolicy::LOAD:
				if (created) {
					*created = true;
				}
				createAndLoadChunk<VoxelChunkMutableRef>(location, lock);
				return extendedChunk(location, MissingChunkPolicy::NONE);
			case MissingChunkPolicy::LOAD_ASYNC:
				lock.unlock();
				m_chunkLoader->loadAsync(*this, location);
				return VoxelChunkExtendedRef();
		}
	}
	return VoxelChunkExtendedRef(*it->second);
}

VoxelChunkMutableRef VoxelWorld::mutableChunk(
		const VoxelChunkLocation &location,
		MissingChunkPolicy policy,
		bool *created
) {
	if (created) {
		*created = false;
	}
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		switch (policy) {
			case MissingChunkPolicy::NONE:
				return VoxelChunkMutableRef();
			case MissingChunkPolicy::CREATE:
				if (created) {
					*created = true;
				}
				return createChunk<VoxelChunkMutableRef>(location);
			case MissingChunkPolicy::LOAD:
				if (created) {
					*created = true;
				}
				return createAndLoadChunk<VoxelChunkMutableRef>(location, lock);
			case MissingChunkPolicy::LOAD_ASYNC:
				lock.unlock();
				m_chunkLoader->loadAsync(*this, location);
				return VoxelChunkMutableRef();
		}
	}
	return VoxelChunkMutableRef(*it->second);
}

VoxelChunkExtendedMutableRef VoxelWorld::extendedMutableChunk(
		const VoxelChunkLocation &location,
		MissingChunkPolicy policy,
		bool *created
) {
	if (created) {
		*created = false;
	}
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		switch (policy) {
			case MissingChunkPolicy::NONE:
				return VoxelChunkExtendedMutableRef();
			case MissingChunkPolicy::CREATE:
				if (created) {
					*created = true;
				}
				return createChunk<VoxelChunkExtendedMutableRef>(location);
			case MissingChunkPolicy::LOAD:
				if (created) {
					*created = true;
				}
				return createAndLoadChunk<VoxelChunkExtendedMutableRef>(location, lock);
			case MissingChunkPolicy::LOAD_ASYNC:
				lock.unlock();
				m_chunkLoader->loadAsync(*this, location);
				return VoxelChunkExtendedMutableRef();
		}
	}
	return VoxelChunkExtendedMutableRef(*it->second);
}

void VoxelWorld::unloadChunks(const std::vector<VoxelChunkLocation> &locations) {
	std::unique_lock<std::mutex> lock(m_mutex);
	for (auto &location : locations) {
		auto it = m_chunks.find(location);
		if (it != m_chunks.end()) {
			std::unique_lock<std::shared_mutex> chunkLock(it->second->mutex());
			it->second->unsetNeighbors();
			chunkLock.unlock();
			if (m_chunkLoader != nullptr) {
				m_chunkLoader->unloadChunkAsync(std::move(it->second));
			}
			m_chunks.erase(it);
		}
	}
}

void VoxelWorld::unload() {
	LOG(INFO) << "Unloading world";
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_chunks.begin();
	while (it != m_chunks.end()) {
		std::unique_lock<std::shared_mutex> chunkLock(it->second->mutex());
		it->second->unsetNeighbors();
		chunkLock.unlock();
		if (m_chunkLoader != nullptr) {
			m_chunkLoader->unloadChunkAsync(std::move(it->second));
		}
		it = m_chunks.erase(it);
	}
}
