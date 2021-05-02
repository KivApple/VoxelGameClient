#include <easylogging++.h>
#include "VoxelLightComputer.h"
#include "world/VoxelWorld.h"

bool VoxelLightComputer::ChunkQueue::empty() const {
	return queue.empty();
}

void VoxelLightComputer::ChunkQueue::push(const InChunkVoxelLocation &location) {
	if (!set.emplace(location).second) return;
	queue.emplace_back(location);
}

void VoxelLightComputer::ChunkQueue::pushNext(const InChunkVoxelLocation &location) {
	if (!nextSet.emplace(location).second) return;
	nextQueue.emplace_back(location);
}

InChunkVoxelLocation VoxelLightComputer::ChunkQueue::pop() {
	auto location = queue.front();
	queue.pop_front();
	set.erase(location);
	return location;
}

void VoxelLightComputer::ChunkQueue::swap() {
	std::swap(set, nextSet);
	std::swap(queue, nextQueue);
}

VoxelLightComputer::VoxelLightComputer() {
	m_thread = std::thread(&VoxelLightComputer::run, this);
}

VoxelLightComputer::~VoxelLightComputer() {
	m_running = false;
	m_thread.join();
}

VoxelLightComputer::ChunkQueue &VoxelLightComputer::chunkQueue(const VoxelChunkLocation &location) {
	auto it = m_chunkQueues.find(location);
	if (it == m_chunkQueues.end()) {
		auto l = location;
		//LOG(TRACE) << l.x << "," << l.y << "," << l.z;
		it = m_chunkQueues.emplace(location, std::make_unique<ChunkQueue>()).first;
	}
	return *it->second;
}

constexpr VoxelLightLevel VoxelLightComputer::computeLightLevel(VoxelLightLevel cur, VoxelLightLevel neighbor, int dy) {
	if (neighbor >= MAX_VOXEL_LIGHT_LEVEL) {
		if (dy > 0) {
			return neighbor;
		}
		neighbor = dy == 0 ? MAX_VOXEL_LIGHT_LEVEL : 0;
	}
	return cur >= neighbor ? cur : std::max(neighbor - 1, 0);
}

void VoxelLightComputer::computeLightLevel(
	VoxelChunkMutableRef &chunk,
	const InChunkVoxelLocation &location,
	ChunkQueue &queue,
	bool load
) {
	static const int offsets[][3] = {
		{-1, 0, 0}, {1, 0, 0},
		{0, -1, 0}, {0, 1, 0},
		{0, 0, -1}, {0, 0, 1}
	};
	auto &cur = chunk.at(location);
	auto typeLightLevel = cur.typeLightLevel();
	auto lightLevel = std::max(std::max(cur.lightLevel(), typeLightLevel), (VoxelLightLevel) 0);
	auto shaderProvider = cur.shaderProvider();
	if (shaderProvider == nullptr || shaderProvider->priority() < MAX_VOXEL_SHADER_PRIORITY) {
		for (auto &offset : offsets) {
			auto &n = chunk.extendedAt(
					location.x + offset[0],
					location.y + offset[1],
					location.z + offset[2]
			);
			lightLevel = computeLightLevel(lightLevel, n.lightLevel(), offset[1]);
		}
		if (typeLightLevel < 0) {
			lightLevel = std::max(lightLevel + typeLightLevel, 0);
		}
	}
	bool prevLightLevel = cur.lightLevel();
	cur.setLightLevel(lightLevel);
	for (auto &offset : offsets) {
		InChunkVoxelLocation nLocation(
				location.x + offset[0],
				location.y + offset[1],
				location.z + offset[2]
		);
		if (
				nLocation.x >= 0 && nLocation.x < VOXEL_CHUNK_SIZE &&
				nLocation.y >= 0 && nLocation.y < VOXEL_CHUNK_SIZE &&
				nLocation.z >= 0 && nLocation.z < VOXEL_CHUNK_SIZE
		) {
			auto &n = chunk.at(nLocation);
			auto nLightLevel = n.lightLevel();
			auto nShaderProvider = n.shaderProvider();
			if (
					(nShaderProvider == nullptr || nShaderProvider->priority() < MAX_VOXEL_SHADER_PRIORITY) && (
							(
									(lightLevel != prevLightLevel) &&
									(computeLightLevel(0, prevLightLevel, offset[1]) == nLightLevel)
							) || (computeLightLevel(0, lightLevel, offset[1]) > nLightLevel)
					)
			) {
				queue.push(nLocation);
			}
		} else {
			bool exists = chunk.hasNeighbor(offset[0], offset[1], offset[2]);
			if (exists || load) {
				VoxelLocation gLocation;
				auto &n = chunk.extendedAt(nLocation, &gLocation);
				if (exists) {
					auto nLightLevel = n.lightLevel();
					auto nShaderProvider = n.shaderProvider();
					if (
							(nShaderProvider == nullptr || nShaderProvider->priority() < MAX_VOXEL_SHADER_PRIORITY) && (
									(
											(lightLevel != prevLightLevel) &&
											(computeLightLevel(0, prevLightLevel, offset[1]) == nLightLevel)
									) || (computeLightLevel(0, lightLevel, offset[1]) > nLightLevel)
							)
					) {
						chunkQueue(gLocation.chunk()).push(gLocation.inChunk());
					}
				} else {
					chunkQueue(gLocation.chunk()).push(gLocation.inChunk());
					queue.pushNext(location);
				}
			}
		}
	}
}

void VoxelLightComputer::computeInitialLightLevels(VoxelChunkMutableRef &chunk) {
	auto &l = chunk.location();
	LOG(TRACE) << "Initial light levels computation for chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z;
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				chunk.at(x, y, z).setLightLevel(-1);
			}
		}
	}
	auto &queue = chunkQueue(chunk.location());
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = VOXEL_CHUNK_SIZE - 1; y >= 0; y--) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				computeLightLevel(chunk, {x, y, z}, queue, true);
			}
		}
	}
}

void VoxelLightComputer::run() {
	LOG(INFO) << "Voxel light computer thread started";
	std::unique_lock<std::mutex> lock(m_queueMutex, std::defer_lock);
	while (m_running) {
		lock.lock();
		if (m_queue.empty()) {
			m_queueCondVar.wait(lock);
		}
		if (m_queue.empty()) continue;
		auto job = m_queue.front();
		m_queue.erase(m_queue.begin());
		lock.unlock();

		auto chunk = job.world->mutableChunk(job.location);
		if (!chunk) {
			auto &l = job.location;
			LOG(WARNING) << "Missing voxel chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z
						 << " submitted for light computation";
			continue;
		}
		computeInitialLightLevels(chunk);
		while (chunk) {
			auto it = m_chunkQueues.find(chunk.location());
			ChunkQueue *queue = nullptr;
			if (it != m_chunkQueues.end()) {
				queue = it->second.get();
				while (!queue->empty()) {
					computeLightLevel(chunk, queue->pop(), *queue, false);
				}
				auto &l = it->first;
				if (queue->nextQueue.empty()) {
					if (m_completeChunks.emplace(chunk.location()).second) {
						chunk.markDirty(true);
						LOG(TRACE) << "Chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z << " completed";
					}
				} else {
					LOG(TRACE) << "Chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z << " needed " <<
						queue->nextQueue.size() << " voxels";
				}
			}
			chunk.unlock(false);
			for (it = m_chunkQueues.begin(); it != m_chunkQueues.end(); ++it) {
				if (it->second->empty()) continue;
				auto &l = it->first;
				LOG(TRACE) << "Changing chunk to x=" << l.x << ",y=" << l.y << ",z=" << l.z;
				chunk = job.world->extendedMutableChunk(l, VoxelWorld::MissingChunkPolicy::LOAD);
				break;
			}
			if (queue) {
				queue->swap();
			}
		}
		chunk = job.world->mutableChunk(job.location);
		chunk.markDirty(true);
		chunk.unlock();
		LOG(TRACE) << "Light computation completed";
		m_chunkQueues.clear();
		for (auto &location : m_completeChunks) {
			auto nChunk = job.world->chunk(location);
			nChunk.unlock(nChunk.lightComputed());
		}
		m_completeChunks.clear();
	}
	LOG(INFO) << "Voxel light computer thread stopped";
}

void VoxelLightComputer::computeAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queue.emplace_back(&world, location);
	m_queueCondVar.notify_one();
}
