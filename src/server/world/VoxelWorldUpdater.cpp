#include <chrono>
#include <easylogging++.h>
#include "VoxelWorldUpdater.h"
#include "world/VoxelWorld.h"

VoxelWorldUpdater::VoxelWorldUpdater(VoxelWorld &world): m_world(world) {
	m_thread = std::thread(&VoxelWorldUpdater::run, this);
}

VoxelWorldUpdater::~VoxelWorldUpdater() {
	shutdown();
}

void VoxelWorldUpdater::shutdown() {
	if (!m_running) return;
	m_running = false;
	m_thread.join();
}

void VoxelWorldUpdater::run() {
	LOG(INFO) << "Voxel world updater thread started";
	std::vector<VoxelChunkLocation> chunkLocations;
	unsigned long time = 0;
	while (m_running) {
		auto nextUpdateTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
		chunkLocations.clear();
		m_world.forEachChunkLocation([&chunkLocations](const VoxelChunkLocation &location) {
			chunkLocations.emplace_back(location);
		});
		unsigned int pendingVoxelCount = 0;
		for (auto &chunkLocation : chunkLocations) {
			auto chunk = m_world.extendedMutableChunk(chunkLocation);
			if (!chunk) continue;
			bool complete = chunk.lightState() == VoxelChunkLightState::COMPLETE;
			for (int dz = -1; dz <= 1 && complete; dz++) {
				for (int dy = -1; dy <= 1 && complete; dy++) {
					for (int dx = -1; dx <= 1 && complete; dx++) {
						if (dx == 0 && dy == 0 && dz == 0) continue;
						if (!chunk.hasNeighbor(dx, dy, dz)) {
							complete = false;
						}
					}
				}
			}
			if (!complete) {
				chunk.unlock();
				continue;
			}
			chunk.update(time);
			pendingVoxelCount += chunk.pendingVoxelCount();
		}
		time++;
		m_pendingVoxelCount = pendingVoxelCount;
		if (nextUpdateTime > std::chrono::steady_clock::now()) {
			std::this_thread::sleep_until(nextUpdateTime);
		} else {
			LOG(WARNING) << "Voxel world update took too long time";
		}
	}
	LOG(INFO) << "Voxel world updater thread stopped";
}
