#include <easylogging++.h>
#include "VoxelWorldGenerator.h"

VoxelWorldGenerator::VoxelWorldGenerator(
		VoxelTypeRegistry &registry
): m_registry(registry), m_grass(
		m_registry.make<SimpleVoxelType>("grass", "grass", "assets/textures/grass_top.png")
), m_dirt(
		m_registry.make<SimpleVoxelType>("dirt", "dirt", "assets/textures/mud.png")
) {
	m_thread = std::thread(std::bind(&VoxelWorldGenerator::run, this));
}

VoxelWorldGenerator::~VoxelWorldGenerator() {
	m_running = false;
	m_queueCondVar.notify_one();
	m_thread.join();
}

void VoxelWorldGenerator::run() {
	LOG(INFO) << "Voxel world generator thread started";
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
		auto chunk = job.world->mutableChunk(job.location, VoxelWorld::MissingChunkPolicy::CREATE);
		load(chunk);
	}
	LOG(INFO) << "Voxel world generator thread stopped";
}

void VoxelWorldGenerator::loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queue.emplace_back(&world, location);
	m_queueCondVar.notify_one();
}

void VoxelWorldGenerator::load(VoxelChunkMutableRef &chunk) {
	auto &l = chunk.location();
	LOG(DEBUG) << "Generating chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z;
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				auto &v = chunk.at(x, y, z);
				auto absoluteY = l.y * VOXEL_CHUNK_SIZE + y;
				if (absoluteY == -1) {
					v.setType(m_grass);
				} else if (absoluteY <= -3) {
					v.setType(m_dirt);
				}
			}
		}
	}
	chunk.markDirty();
}
