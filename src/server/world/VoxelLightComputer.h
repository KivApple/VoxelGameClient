#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "world/VoxelLocation.h"
#include "world/Voxel.h"
#include "Worker.h"

class VoxelWorld;
class VoxelChunkMutableRef;
class VoxelLightComputer;

struct VoxelLightComputerJob {
	VoxelLightComputer *computer;
	VoxelWorld *world;
	VoxelChunkLocation chunkLocation;
	
	VoxelLightComputerJob(VoxelLightComputer *computer, VoxelWorld *world, const VoxelChunkLocation &location);
	bool operator==(const VoxelLightComputerJob &job) const;
	void operator()() const;
};

class VoxelLightComputer: public Worker<VoxelLightComputerJob> {
	struct ChunkQueue {
		std::deque<InChunkVoxelLocation> queue;
		std::unordered_set<InChunkVoxelLocation> set;
		
		bool empty() const;
		void push(const InChunkVoxelLocation &location);
		InChunkVoxelLocation pop();
	};
	
	std::unordered_map<VoxelChunkLocation, std::unique_ptr<ChunkQueue>> m_chunkQueues;
	std::unordered_set<VoxelChunkLocation> m_visitedChunks;
	int m_iterationCount = 0;
	
	ChunkQueue &chunkQueue(const VoxelChunkLocation &location);
	constexpr static VoxelLightLevel computeLightLevel(VoxelLightLevel cur, VoxelLightLevel neighbor, int dy);
	void computeLightLevel(
			VoxelChunkMutableRef &chunk,
			const InChunkVoxelLocation &location,
			ChunkQueue &queue,
			bool load
	);
	void computeInitialLightLevels(VoxelChunkMutableRef &chunk, bool load);
	void runJob(const VoxelLightComputerJob &job);
	
	friend class VoxelLightComputerJob;

public:
	VoxelLightComputer();
	~VoxelLightComputer();
	void computeAsync(VoxelWorld &world, const VoxelChunkLocation &location);
	void cancelComputeAsync(VoxelWorld &world, const VoxelChunkLocation &location);
	
};
