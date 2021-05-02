#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "world/VoxelWorld.h"
#include "world/VoxelTypeRegistry.h"

class VoxelWorldGenerator: public VoxelChunkLoader {
	struct Job {
		VoxelWorld *world;
		VoxelChunkLocation location;
		
		constexpr Job(VoxelWorld *world, const VoxelChunkLocation &location): world(world), location(location) {
		}
	};
	
	VoxelTypeRegistry &m_registry;
	VoxelType &m_air;
	VoxelType &m_grass;
	VoxelType &m_dirt;
	VoxelType &m_stone;
	std::atomic<bool> m_running = true;
	std::deque<Job> m_queue;
	std::mutex m_queueMutex;
	std::condition_variable m_queueCondVar;
	std::thread m_thread;
	
	void run();

public:
	explicit VoxelWorldGenerator(VoxelTypeRegistry &registry);
	~VoxelWorldGenerator() override;
	void load(VoxelChunkMutableRef &chunk) override;
	void loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	void cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	VoxelType &air() {
		return m_air;
	}
	
};
