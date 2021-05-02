#include <easylogging++.h>
#include "VoxelWorldGenerator.h"
#include "world/VoxelTypes.h"

VoxelWorldGenerator::VoxelWorldGenerator(
		VoxelTypeRegistry &registry
): m_registry(registry), m_air(
		m_registry.make<AirVoxelType>("air")
), m_grass(
		m_registry.make<SimpleVoxelType>("grass", "grass", "assets/textures/grass_top.png")
), m_dirt(
		m_registry.make<SimpleVoxelType>("dirt", "dirt", "assets/textures/mud.png")
), m_stone(
		m_registry.make<SimpleVoxelType>("stone", "stone", "assets/textures/stone.png")
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
		bool created = false;
		auto chunk = job.world->mutableChunk(job.location, VoxelWorld::MissingChunkPolicy::CREATE, &created);
		if (!created) {
			chunk.unlock(false);
			continue;
		}
		load(chunk);
	}
	LOG(INFO) << "Voxel world generator thread stopped";
}

void VoxelWorldGenerator::loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queue.emplace_back(&world, location);
	m_queueCondVar.notify_one();
}

void VoxelWorldGenerator::cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	auto it = m_queue.begin();
	while (it != m_queue.end()) {
		if (it->world == &world && it->location == location) {
			it = m_queue.erase(it);
		} else {
			++it;
		}
	}
}

void VoxelWorldGenerator::load(VoxelChunkMutableRef &chunk) {
	auto &location = chunk.location();
	LOG(DEBUG) << "Generating chunk at x=" << location.x << ",y=" << location.y << ",z=" << location.z;
	if (location.y >= 0) {
		for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
			for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
				for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
					auto &v = chunk.at(x, y, z);
					v.setType(m_air);
					v.setLightLevel(MAX_VOXEL_LIGHT_LEVEL);
				}
			}
		}
		chunk.markDirty(true);
		return;
	}
	
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				VoxelLocation l(location, {x, y, z});
				auto &v = chunk.at(x, y, z);
				if (l.x == 3 && l.y == -1 && l.z == -4) {
					v.setType(m_stone);
				} else if (l.y < -3) {
					v.setType(m_stone);
				} else if (l.y < -1) {
					v.setType(m_dirt);
				} else if (l.y == -1) {
					v.setType(m_grass);
				} else {
					v.setType(m_air);
				}
			}
		}
	}
	chunk.markDirty();
}
