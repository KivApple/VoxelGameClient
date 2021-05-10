#include <easylogging++.h>
#include "VoxelWorldGenerator.h"

VoxelWorldGeneratorJob::VoxelWorldGeneratorJob(
		VoxelWorldGenerator *generator,
		VoxelWorld *world,
		const VoxelChunkLocation &location
): generator(generator), world(world), location(location) {
}

bool VoxelWorldGeneratorJob::operator==(const VoxelWorldGeneratorJob &job) const {
	return world == job.world && location == job.location;
}

void VoxelWorldGeneratorJob::operator()() const {
	bool created = false;
	auto chunk = world->mutableChunk(location, VoxelWorld::MissingChunkPolicy::CREATE, &created);
	if (!created) {
		return;
	}
	generator->load(chunk);
}

VoxelWorldGenerator::VoxelWorldGenerator(
		VoxelTypeRegistry &registry
): m_registry(registry), m_air(m_registry.get("air")), m_grass(m_registry.get("grass")),
	m_dirt(m_registry.get("dirt")), m_stone(m_registry.get("stone")),
	m_lava(m_registry.get("lava")), m_glass(m_registry.get("glass")),
	Worker("VoxelWorldGenerator") {
}

VoxelWorldGenerator::~VoxelWorldGenerator() {
	shutdown();
}

void VoxelWorldGenerator::loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	post(this, &world, location);
}

void VoxelWorldGenerator::cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	cancel(VoxelWorldGeneratorJob(this, &world, location), false);
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
		chunk.setLightState(VoxelChunkLightState::READY);
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
}
