#include <gtest/gtest.h>
#include "world/VoxelLocation.h"

TEST(VoxelLocation, chunkLocation) {
	EXPECT_EQ(VoxelLocation(0, 0, 0).chunk(), VoxelChunkLocation(0, 0, 0));
	EXPECT_EQ(VoxelLocation(15, 15, 15).chunk(), VoxelChunkLocation(0, 0, 0));
	EXPECT_EQ(VoxelLocation(16, 16, 16).chunk(), VoxelChunkLocation(1, 1, 1));
	
	EXPECT_EQ(VoxelLocation(-1, -1, -1).chunk(), VoxelChunkLocation(-1, -1, -1));
	EXPECT_EQ(VoxelLocation(-16, -16, -16).chunk(), VoxelChunkLocation(-1, -1, -1));
	EXPECT_EQ(VoxelLocation(-17, -17, -17).chunk(), VoxelChunkLocation(-2, -2, -2));
}

TEST(VoxelLocation, inChunkLocation) {
	EXPECT_EQ(VoxelLocation(0, 0, 0).inChunk(), InChunkVoxelLocation(0, 0, 0));
	EXPECT_EQ(VoxelLocation(15, 15, 15).inChunk(), InChunkVoxelLocation(15, 15, 15));
	EXPECT_EQ(VoxelLocation(16, 16, 16).inChunk(), InChunkVoxelLocation(0, 0, 0));
	
	EXPECT_EQ(VoxelLocation(-1, -1, -1).inChunk(), InChunkVoxelLocation(15, 15, 15));
	EXPECT_EQ(VoxelLocation(-16, -16, -16).inChunk(), InChunkVoxelLocation(0, 0, 0));
	EXPECT_EQ(VoxelLocation(-17, -17, -17).inChunk(), InChunkVoxelLocation(15, 15, 15));
}

TEST(VoxelLocation, location) {
	EXPECT_EQ(
			VoxelLocation(VoxelChunkLocation(0, 0, 0), InChunkVoxelLocation(0, 0, 0)),
			VoxelLocation(0, 0, 0)
	);
	EXPECT_EQ(
			VoxelLocation(VoxelChunkLocation(0, 0, 0), InChunkVoxelLocation(15, 15, 15)),
			VoxelLocation(15, 15, 15)
	);
	EXPECT_EQ(
			VoxelLocation(VoxelChunkLocation(1, 1, 1), InChunkVoxelLocation(0, 0, 0)),
			VoxelLocation(16, 16, 16)
	);
	
	EXPECT_EQ(
			VoxelLocation(VoxelChunkLocation(-1, -1, -1), InChunkVoxelLocation(15, 15, 15)),
			VoxelLocation(-1, -1, -1)
	);
	EXPECT_EQ(
			VoxelLocation(VoxelChunkLocation(-1, -1, -1), InChunkVoxelLocation(0, 0, 0)),
			VoxelLocation(-16, -16, -16)
	);
	EXPECT_EQ(
			VoxelLocation(VoxelChunkLocation(-2, -2, -2), InChunkVoxelLocation(15, 15, 15)),
			VoxelLocation(-17, -17, -17)
	);
}
