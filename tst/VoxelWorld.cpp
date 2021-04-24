#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "world/VoxelWorld.h"

struct Observer {
	MOCK_METHOD0(invoke, void());
};

struct TestVoxel: public Voxel {
	Observer *destructorObserver = nullptr;
	
	~TestVoxel() {
		if (destructorObserver != nullptr) {
			destructorObserver->invoke();
		}
	}
	
};

class TestVoxelType: public ConcreteVoxelType<TestVoxelType, TestVoxel> {
public:
	std::string toString(const TestVoxel &voxel) override {
		return "test";
	}
	
	const VoxelShaderProvider *shaderProvider(const TestVoxel &voxel) override {
		shaderProviderCalled();
		return nullptr;
	}
	
	void buildVertexData(const TestVoxel &voxel, std::vector<VoxelVertexData> &data) override {
		buildVertexDataCalled();
	}
	
	MOCK_METHOD0(shaderProviderCalled, void());
	
	MOCK_METHOD0(buildVertexDataCalled, void());
	
};

TEST(VoxelWorld, voxelType) {
	Observer destructorObserver;
	TestVoxelType testVoxelType;
	VoxelChunk chunk({0, 0, 0});
	ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
	chunk.initAt(0, 0, 0, testVoxelType);
	ASSERT_EQ(chunk.at(0, 0, 0).toString(), "test");
	
	EXPECT_CALL(testVoxelType, shaderProviderCalled()).Times(1);
	{
		auto result = chunk.at(0, 0, 0).shaderProvider();
		(void) result;
	}
	EXPECT_CALL(testVoxelType, buildVertexDataCalled()).Times(1);
	{
		std::vector<VoxelVertexData> data;
		chunk.at(0, 0, 0).buildVertexData(data);
	}
	
	((TestVoxel&) chunk.at(0, 0, 0)).destructorObserver = &destructorObserver;
	EXPECT_CALL(destructorObserver, invoke()).Times(1);
}

TEST(VoxelWorld, rawChunk) {
	TestVoxelType testVoxelType;
	
	{
		VoxelChunk chunk({0, 0, 0});
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
		chunk.initAt(0, 0, 0, testVoxelType);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
	}
	
	{
		VoxelChunk chunk({0, 0, 0});
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
		chunk.initAt(1, 0, 0, testVoxelType);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
	}
	
	{
		VoxelChunk chunk({0, 0, 0});
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
		chunk.initAt(0, 1, 0, testVoxelType);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "test");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
	}
	
	{
		VoxelChunk chunk({0, 0, 0});
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
		chunk.initAt(0, 0, 1, testVoxelType);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "test");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
	}
	
	{
		VoxelChunk chunk({0, 0, 0});
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "empty");
		chunk.initAt(1, 1, 1, testVoxelType);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		ASSERT_EQ(chunk.at(1, 1, 1).toString(), "test");
	}
	
	{
		VoxelChunk chunk({0, 0, 0});
		ASSERT_EQ(chunk.at(14, 14, 14).toString(), "empty");
		ASSERT_EQ(chunk.at(15, 14, 14).toString(), "empty");
		ASSERT_EQ(chunk.at(14, 15, 14).toString(), "empty");
		ASSERT_EQ(chunk.at(14, 14, 15).toString(), "empty");
		ASSERT_EQ(chunk.at(15, 15, 15).toString(), "empty");
		chunk.initAt(15, 15, 15, testVoxelType);
		ASSERT_EQ(chunk.at(14, 14, 14).toString(), "empty");
		ASSERT_EQ(chunk.at(15, 14, 14).toString(), "empty");
		ASSERT_EQ(chunk.at(14, 15, 14).toString(), "empty");
		ASSERT_EQ(chunk.at(14, 14, 15).toString(), "empty");
		ASSERT_EQ(chunk.at(15, 15, 15).toString(), "test");
	}
}

TEST(VoxelWorld, oneChunk) {
	TestVoxelType testVoxelType;
	VoxelWorld world;
	
	ASSERT_FALSE(world.chunk({0, 0, 0}));
	
	{
		auto chunk = world.mutableChunk({0, 0, 0}, true);
		ASSERT_TRUE(chunk);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		chunk.at(0, 0, 0).setType(testVoxelType);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "test");
	}
	
	{
		auto chunk = world.chunk({0, 0, 0});
		ASSERT_TRUE(chunk);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.at(0, 0, 1).toString(), "empty");
	}
}

TEST(VoxelWorld, twoChunks) {
	TestVoxelType testVoxelType;
	VoxelWorld world;
	
	ASSERT_FALSE(world.chunk({0, 0, 0}));
	
	{
		auto chunk = world.mutableChunk({0, 0, 0}, true);
		ASSERT_TRUE(chunk);
		ASSERT_FALSE(chunk.hasNeighbor(1, 0, 0));
		ASSERT_EQ(chunk.extendedAt(16, 0, 0).toString(), "empty");
		
		ASSERT_EQ(chunk.extendedAt(0, 0, 0).toString(), "empty");
		chunk.at(0, 0, 0).setType(testVoxelType);
		ASSERT_EQ(chunk.extendedAt(0, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.extendedAt(1, 0, 0).toString(), "empty");
	}
	
	{
		auto chunk = world.mutableChunk({1, 0, 0}, true);
		ASSERT_TRUE(chunk);
		ASSERT_TRUE(chunk.hasNeighbor(-1, 0, 0));
		ASSERT_EQ(chunk.extendedAt(-16, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.extendedAt(-15, 0, 0).toString(), "empty");
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
		chunk.at(0, 0, 0).setType(testVoxelType);
		ASSERT_EQ(chunk.extendedAt(0, 0, 0).toString(), "test");
	}
	
	{
		auto chunk = world.extendedChunk({0, 0, 0});
		ASSERT_TRUE(chunk);
		ASSERT_TRUE(chunk.hasNeighbor(1, 0, 0));
		ASSERT_EQ(chunk.extendedAt(16, 0, 0).toString(), "test");
	}
	
	{
		auto chunk = world.extendedMutableChunk({0, 0, 0}, true);
		ASSERT_TRUE(chunk);
		ASSERT_TRUE(chunk.hasNeighbor(1, 0, 0));
		ASSERT_EQ(chunk.extendedAt(16, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.extendedAt(16, 1, 0).toString(), "empty");
		chunk.extendedAt(16, 1, 0).setType(testVoxelType);
		ASSERT_EQ(chunk.extendedAt(16, 1, 0).toString(), "test");
	}
	
	{
		auto chunk = world.chunk({1, 0, 0});
		ASSERT_TRUE(chunk);
		ASSERT_EQ(chunk.at(0, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.at(0, 1, 0).toString(), "test");
		ASSERT_EQ(chunk.at(0, 2, 0).toString(), "empty");
	}
}
