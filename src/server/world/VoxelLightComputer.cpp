#include <easylogging++.h>
#include "VoxelLightComputer.h"
#include "world/VoxelWorld.h"

VoxelLightComputerJob::VoxelLightComputerJob(
		VoxelLightComputer *computer,
		VoxelWorld *world,
		const VoxelChunkLocation &location
): computer(computer), world(world), chunkLocation(location) {
}

bool VoxelLightComputerJob::operator==(const VoxelLightComputerJob &job) const {
	return world == job.world && job.chunkLocation == job.chunkLocation;
}

void VoxelLightComputerJob::operator()() const {
	computer->runJob(*this);
}

bool VoxelLightComputer::ChunkQueue::empty() const {
	return queue.empty();
}

void VoxelLightComputer::ChunkQueue::push(const InChunkVoxelLocation &location) {
	if (!set.emplace(location).second) return;
	queue.emplace_back(location);
}

InChunkVoxelLocation VoxelLightComputer::ChunkQueue::pop() {
	auto location = queue.front();
	queue.pop_front();
	set.erase(location);
	return location;
}

VoxelLightComputer::VoxelLightComputer(): Worker("VoxelLightComputer") {
}

VoxelLightComputer::~VoxelLightComputer() {
	shutdown();
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

	m_iterationCount++;
	auto &cur = chunk.at(location);
	auto typeLightLevel = cur.typeLightLevel();
	auto lightLevel = std::max(typeLightLevel, (VoxelLightLevel) 0);
	auto shaderProvider = cur.shaderProvider();
	if (shaderProvider == nullptr || shaderProvider->priority() < MAX_VOXEL_SHADER_PRIORITY) {
		for (auto &offset : offsets) {
			auto &n = chunk.extendedAt(
					location.x + offset[0],
					location.y + offset[1],
					location.z + offset[2]
			);
			if (&n.type() == &EmptyVoxelType::INSTANCE) continue;
			lightLevel = computeLightLevel(lightLevel, n.lightLevel(), offset[1]);
		}
		if (typeLightLevel < 0) {
			lightLevel = std::max(lightLevel + typeLightLevel, 0);
		}
	}
	auto prevLightLevel = cur.lightLevel();
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
									(computeLightLevel(0, prevLightLevel, -offset[1]) == nLightLevel)
							) || (computeLightLevel(0, lightLevel, -offset[1]) > nLightLevel)
					)
			) {
				queue.push(nLocation);
			}
		} else {
			bool exists = chunk.hasNeighbor(offset[0], offset[1], offset[2]);
			VoxelLocation gLocation;
			auto &n = chunk.extendedAt(nLocation, &gLocation);
			if (exists || load) {
				if (exists) {
					auto nLightLevel = n.lightLevel();
					auto nShaderProvider = n.shaderProvider();
					if (
							(nShaderProvider == nullptr || nShaderProvider->priority() < MAX_VOXEL_SHADER_PRIORITY) && (
									(
											(lightLevel != prevLightLevel) &&
											(computeLightLevel(0, prevLightLevel, -offset[1]) == nLightLevel)
									) || (computeLightLevel(0, lightLevel, -offset[1]) > nLightLevel)
							)
					) {
						chunkQueue(gLocation.chunk()).push(gLocation.inChunk());
					}
				} else {
					chunkQueue(gLocation.chunk()).push(gLocation.inChunk());
				}
			}
		}
	}
}

void VoxelLightComputer::computeInitialLightLevels(VoxelChunkMutableRef &chunk, bool load) {
	auto &l = chunk.location();
	LOG(DEBUG) << "Initial light levels computation for chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z;
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
				computeLightLevel(chunk, {x, y, z}, queue, load);
			}
		}
	}
}

void VoxelLightComputer::runJob(const VoxelLightComputerJob &job) {
	static const int offsets[][3] = {
			{-1, 0, 0}, {1, 0, 0},
			{0, -1, 0}, {0, 1, 0},
			{0, 0, -1}, {0, 0, 1}
	};
	
	m_iterationCount = 0;
	auto chunk = job.world->mutableChunk(job.chunkLocation, VoxelWorld::MissingChunkPolicy::LOAD);
	{
		auto &l = job.chunkLocation;
		LOG(TRACE) << "Got a new light computation job for chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z;
		if (chunk.lightState() == VoxelChunkLightState::READY || chunk.lightState() == VoxelChunkLightState::COMPLETE) {
			LOG(TRACE) << "Chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z << " has already computed light levels";
			return;
		}
	}
	while (chunk) {
		m_visitedChunks.emplace(chunk.location());
		switch (chunk.lightState()) {
			case VoxelChunkLightState::PENDING_INITIAL:
				computeInitialLightLevels(chunk, chunk.location() == job.chunkLocation);
				break;
			case VoxelChunkLightState::PENDING_INCREMENTAL: {
				auto &l = chunk.location();
				auto &queue = chunkQueue(l);
				LOG(DEBUG) << "Recompute light levels for " << chunk.dirtyLocations().size() <<
						   " voxel(s) in chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z;
				for (auto &location : chunk.dirtyLocations()) {
					queue.push(location);
				}
				chunk.clearDirtyLocations();
				break;
			}
			case VoxelChunkLightState::COMPUTING:
			case VoxelChunkLightState::READY:
			case VoxelChunkLightState::COMPLETE:
				break;
		}
		chunk.setLightState(VoxelChunkLightState::COMPUTING);
		auto it = m_chunkQueues.find(chunk.location());
		if (it != m_chunkQueues.end()) {
			auto &queue = *it->second;
			while (!queue.empty()) {
				computeLightLevel(chunk, queue.pop(), queue, chunk.location() == job.chunkLocation);
			}
		}
		chunk.unlock();
		it = m_chunkQueues.begin();
		while (it != m_chunkQueues.end()) {
			if (it->second->empty()) {
				it++;
				continue;
			}
			auto &l = it->first;
			LOG(TRACE) << "Changing chunk to x=" << l.x << ",y=" << l.y << ",z=" << l.z;
			chunk = job.world->mutableChunk(l, VoxelWorld::MissingChunkPolicy::LOAD);
			break;
		}
	}
	LOG(TRACE) << "Light computation completed (" << m_iterationCount << " iterations)";
	m_chunkQueues.clear();
	for (auto &location : m_visitedChunks) {
		chunk = job.world->mutableChunk(location);
		if (!chunk) continue;
		if (chunk.lightState() == VoxelChunkLightState::COMPUTING) {
			bool complete = true;
			for (int dz = -1; dz <= 1 && complete; dz++) {
				for (int dy = -1; dy <= 1 && complete; dy++) {
					for (int dx = -1; dx <= 1 && complete; dx++) {
						if (!chunk.hasNeighbor(dx, dy, dz)) {
							complete = false;
						}
					}
				}
			}
			chunk.setLightState(VoxelChunkLightState::READY);
			auto &l = chunk.location();
			LOG(TRACE) << "Chunk to x=" << l.x << ",y=" << l.y << ",z=" << l.z << " is ready";
		}
		chunk.unlock();
	}
	m_visitedChunks.clear();
}

void VoxelLightComputer::computeAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	post(this, &world, location);
}

void VoxelLightComputer::cancelComputeAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	cancel(VoxelLightComputerJob(this, &world, location), false);
}
