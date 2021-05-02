#pragma once

#include <atomic>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include "world/VoxelLocation.h"
#include "world/Voxel.h"

class VoxelWorld;
class VoxelChunkMutableRef;

class VoxelLightComputer {
	struct Job {
		VoxelWorld *world;
		VoxelChunkLocation location;
		
		Job(VoxelWorld *world, const VoxelChunkLocation &location): world(world), location(location) {
		}
	};
	
	struct ChunkQueue {
		std::deque<InChunkVoxelLocation> queue;
		std::unordered_set<InChunkVoxelLocation> set;
		std::deque<InChunkVoxelLocation> nextQueue;
		std::unordered_set<InChunkVoxelLocation> nextSet;
		
		bool empty() const;
		void push(const InChunkVoxelLocation &location);
		void pushNext(const InChunkVoxelLocation &location);
		InChunkVoxelLocation pop();
		void swap();
	};
	
	std::atomic<bool> m_running = true;
	std::deque<Job> m_queue;
	std::mutex m_queueMutex;
	std::condition_variable m_queueCondVar;
	std::thread m_thread;
	std::unordered_map<VoxelChunkLocation, std::unique_ptr<ChunkQueue>> m_chunkQueues;
	std::unordered_set<VoxelChunkLocation> m_completeChunks;
	
	ChunkQueue &chunkQueue(const VoxelChunkLocation &location);
	constexpr static VoxelLightLevel computeLightLevel(VoxelLightLevel cur, VoxelLightLevel neighbor, int dy);
	void computeLightLevel(
			VoxelChunkMutableRef &chunk,
			const InChunkVoxelLocation &location,
			ChunkQueue &queue,
			bool load
	);
	void computeInitialLightLevels(VoxelChunkMutableRef &chunk);
	void run();

public:
	VoxelLightComputer();
	~VoxelLightComputer();
	void computeAsync(VoxelWorld &world, const VoxelChunkLocation &location);
	
};
