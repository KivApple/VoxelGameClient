#include "VoxelTypes.h"
#include "VoxelWorld.h"

std::string AirVoxelType::toString(const Voxel &voxel) {
	return "air";
}

bool AirVoxelType::hasDensity(const Voxel &voxel) {
	return false;
}

GrassVoxelType::GrassVoxelType(): VoxelTypeHelper("grass", "assets/textures/grass.png", true) {
}

void GrassVoxelType::link(VoxelTypeRegistry &registry) {
	m_dirt = &registry.get("dirt");
	m_lava = &registry.get("lava");
}

void GrassVoxelType::slowUpdate(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &voxel,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	if (chunk.extendedAt(location.x, location.y + 1, location.z).lightLevel() <= 4) {
		chunk.at(location).setType(*m_dirt);
		invalidatedLocations.emplace(location);
		return;
	}
	for (int dx = -1; dx <= 1; dx++) {
		for (int dy = -2; dy <= 2; dy++) {
			for (int dz = -1; dz <= 1; dz++) {
				if (dx == 0 && dy == 0 && dz == 0) continue;
				InChunkVoxelLocation nLocation(
						location.x + dx,
						location.y + dy - 1,
						location.z + dz
				);
				auto &n = chunk.extendedAt(nLocation);
				if (&n.type() != m_dirt) continue;
				auto &nTop = chunk.extendedAt({
					nLocation.x,
					nLocation.y + 1,
					nLocation.z
				});
				if (nTop.lightLevel() < 9) continue;
				if (nTop.shaderProvider() != nullptr) continue;
				n.setType(*this);
				invalidatedLocations.emplace(nLocation);
				break;
			}
		}
	}
}

bool GrassVoxelType::update(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &voxel,
		unsigned long deltaTime,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	bool die = false;
	if (chunk.extendedAt(location.x, location.y + 1, location.z).hasDensity()) {
		die = true;
	}
	for (int dz = -1; dz <= 1; dz++) {
		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				if (dx == 0 && dy == 0 && dz == 0) continue;
				auto &n = chunk.extendedAt(location.x + dx, location.y + dy, location.z + dz);
				if (&n.type() == m_lava) {
					die = true;
				}
			}
		}
	}
	if (die) {
		chunk.at(location).setType(*m_dirt);
		invalidatedLocations.emplace(location);
		return true;
	}
	return false;
}

WaterVoxelType::WaterVoxelType(): VoxelType(8, 5, true, "water", "assets/textures/water.png", false, -2, true, false) {
}

void registerVoxelTypes(VoxelTypeRegistry &registry) {
	registry.make<AirVoxelType>("air");
	registry.make<GrassVoxelType>("grass");
	registry.make<SimpleVoxelType>("dirt", "dirt", "assets/textures/mud.png");
	registry.make<SimpleVoxelType>("stone", "stone", "assets/textures/stone.png");
	registry.make<WaterVoxelType>("water");
	registry.make<SimpleVoxelType>(
			"lava",
			"lava",
			"assets/textures/lava.png",
			false,
			MAX_VOXEL_LIGHT_LEVEL - 1
	);
	registry.make<SimpleVoxelType>(
			"glass",
			"glass",
			"assets/textures/glass.png",
			false,
			0,
			true
	);
	registry.link();
}
