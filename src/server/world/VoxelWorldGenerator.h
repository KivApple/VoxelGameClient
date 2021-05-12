#pragma once

#include "Worker.h"
#include "world/VoxelWorld.h"
#include "world/VoxelTypeRegistry.h"

class VoxelWorldGenerator;

struct VoxelWorldGeneratorJob {
	VoxelWorldGenerator *generator;
	VoxelWorld *world;
	VoxelChunkLocation location;
	
	VoxelWorldGeneratorJob(VoxelWorldGenerator *generator, VoxelWorld *world, const VoxelChunkLocation &location);
	bool operator==(const VoxelWorldGeneratorJob &job) const;
	void operator()() const;
};

class VoxelWorldGenerator: public VoxelChunkLoader, public Worker<VoxelWorldGeneratorJob> {
	VoxelTypeRegistry &m_registry;
	VoxelTypeInterface &m_air;
	VoxelTypeInterface &m_grass;
	VoxelTypeInterface &m_dirt;
	VoxelTypeInterface &m_stone;
	VoxelTypeInterface &m_lava;
	VoxelTypeInterface &m_glass;

public:
	explicit VoxelWorldGenerator(VoxelTypeRegistry &registry);
	~VoxelWorldGenerator() override;
	void load(VoxelChunkMutableRef &chunk) override;
	void loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	void cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	VoxelTypeInterface &air() {
		return m_air;
	}
	
};
