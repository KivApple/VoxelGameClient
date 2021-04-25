#include "VoxelWorld.h"

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

SharedVoxelChunk *SharedVoxelChunk::neighbor(int dx, int dy, int dz) const {
	return m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3];
}

/* VoxelChunkRef */

VoxelChunkRef::VoxelChunkRef(SharedVoxelChunk &chunk, bool lock): m_chunk(&chunk) {
	if (lock) {
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

void VoxelChunkRef::unlock() {
	if (m_chunk) {
		m_chunk->mutex().unlock_shared();
		m_chunk = nullptr;
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

void VoxelChunkExtendedRef::unlock() {
	for (auto &&neighbor : m_neighbors) {
		if (neighbor == nullptr) continue;
		neighbor->mutex().unlock_shared();
		neighbor = nullptr;
	}
	VoxelChunkRef::unlock();
}

bool VoxelChunkExtendedRef::hasNeighbor(int dx, int dy, int dz) const {
	return m_neighbors[(dx + 1) + (dy + 1) * 3 + (dz + 1) * 3 * 3] != nullptr;
}

const Voxel &VoxelChunkExtendedRef::extendedAt(int x, int y, int z, VoxelLocation *outLocation) const {
	return extendedAt({x, y, z}, outLocation);
}

const Voxel &VoxelChunkExtendedRef::extendedAt(const InChunkVoxelLocation &location, VoxelLocation *outLocation) const {
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
	static const Voxel empty { EmptyVoxelType::INSTANCE };
	return empty;
}

/* VoxelChunkMutableRef */

VoxelChunkMutableRef::VoxelChunkMutableRef(
		SharedVoxelChunk &chunk,
		bool lockNeighbors
): VoxelChunkExtendedRef(chunk, false, lockNeighbors) {
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

void VoxelChunkMutableRef::unlock() {
	m_chunk->mutex().unlock();
	m_chunk = nullptr;
	VoxelChunkExtendedRef::unlock();
}

Voxel &VoxelChunkMutableRef::at(int x, int y, int z) const {
	return m_chunk->at(x, y, z);
}

Voxel &VoxelChunkMutableRef::at(const InChunkVoxelLocation &location) const {
	return m_chunk->at(location);
}

/* VoxelChunkExtendedMutableRef */

VoxelChunkExtendedMutableRef::VoxelChunkExtendedMutableRef(
		SharedVoxelChunk &chunk
): VoxelChunkMutableRef(chunk, false) {
	for (auto neighbor : m_neighbors) {
		if (neighbor == nullptr) continue;
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

void VoxelChunkExtendedMutableRef::unlock() {
	for (auto &&neighbor : m_neighbors) {
		if (neighbor == nullptr) continue;
		neighbor->mutex().unlock();
		neighbor = nullptr;
	}
	VoxelChunkMutableRef::unlock();
}

Voxel &VoxelChunkExtendedMutableRef::extendedAt(int x, int y, int z, VoxelLocation *outLocation) const {
	return extendedAt({x, y, z}, outLocation);
}

Voxel &VoxelChunkExtendedMutableRef::extendedAt(
		const InChunkVoxelLocation &location,
		VoxelLocation *outLocation
) const {
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
	static Voxel empty { EmptyVoxelType::INSTANCE };
	return empty;
}

/* VoxelWorld */

VoxelChunkRef VoxelWorld::chunk(const VoxelChunkLocation &location) {
	std::shared_lock<std::shared_mutex> lock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		return VoxelChunkRef();
	}
	return VoxelChunkRef(*it->second);
}

VoxelChunkExtendedRef VoxelWorld::extendedChunk(const VoxelChunkLocation &location) {
	std::shared_lock<std::shared_mutex> lock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		return VoxelChunkExtendedRef();
	}
	return VoxelChunkExtendedRef(*it->second);
}

VoxelChunkMutableRef VoxelWorld::mutableChunk(const VoxelChunkLocation &location, bool create) {
	std::shared_lock<std::shared_mutex> sharedLock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		if (!create) {
			return VoxelChunkMutableRef();
		}
		sharedLock.unlock();
		std::unique_lock<std::shared_mutex> lock(m_mutex);
		it = m_chunks.find(location);
		if (it != m_chunks.end()) {
			return VoxelChunkMutableRef(*it->second);
		}
		auto chunkPtr = std::make_unique<SharedVoxelChunk>(location);
		auto &chunk = *chunkPtr;
		chunk.setNeighbors(m_chunks);
		m_chunks.emplace(location, std::move(chunkPtr));
		return VoxelChunkMutableRef(chunk);
	}
	return VoxelChunkMutableRef(*it->second);
}

VoxelChunkExtendedMutableRef VoxelWorld::extendedMutableChunk(const VoxelChunkLocation &location, bool create) {
	std::shared_lock<std::shared_mutex> sharedLock(m_mutex);
	auto it = m_chunks.find(location);
	if (it == m_chunks.end()) {
		if (!create) {
			return VoxelChunkExtendedMutableRef();
		}
		sharedLock.unlock();
		std::unique_lock<std::shared_mutex> lock(m_mutex);
		it = m_chunks.find(location);
		if (it != m_chunks.end()) {
			return VoxelChunkExtendedMutableRef(*it->second);
		}
		auto chunkPtr = std::make_unique<SharedVoxelChunk>(location);
		auto &chunk = *chunkPtr;
		chunk.setNeighbors(m_chunks);
		m_chunks.emplace(location, std::move(chunkPtr));
		return VoxelChunkExtendedMutableRef(chunk);
	}
	return VoxelChunkExtendedMutableRef(*it->second);
}
