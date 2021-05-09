#pragma once

#include <atomic>
#include <thread>

class VoxelWorld;

class VoxelWorldUpdater {
	VoxelWorld &m_world;
	std::atomic<bool> m_running = true;
	std::thread m_thread;
	
	void run();
	
public:
	explicit VoxelWorldUpdater(VoxelWorld &world);
	~VoxelWorldUpdater();
	void shutdown();

};
