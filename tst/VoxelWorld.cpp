#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "world/VoxelWorld.h"

struct Observer {
	MOCK_METHOD0(invoke, void());
};

struct TestVoxel {
	Observer *destructorObserver = nullptr;
	
	~TestVoxel() {
		if (destructorObserver != nullptr) {
			destructorObserver->invoke();
		}
	}
	
	template<typename S> void serialize(S &s) {
	}
	
};

struct TestVoxelTraitState {
	int a = 10;
	
	template<typename S> void serialize(S &s) {
		s.value4b(a);
	}

};

class TestVoxelTrait: public VoxelTypeTrait<TestVoxelTraitState> {
	std::function<void()> m_callback;
	
public:
	explicit TestVoxelTrait(std::function<void()> callback): m_callback(std::move(callback)) {
	}
	
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const TestVoxelTraitState &voxel,
			std::vector<VoxelVertexData> &data
	) {
		m_callback();
	}
	
};

class EmptyVoxelTrait: public VoxelTypeTrait<> {
};

class TestVoxelType: public VoxelType<
        TestVoxelType,
        TestVoxel,
        TestVoxelTrait,
        EmptyVoxelTrait
> {
public:
	TestVoxelType(): VoxelType(TestVoxelTrait([this]() {
		traitBuildVertexDataCalled();
	}), EmptyVoxelTrait()) {
	}
	
	std::string toString(const State &voxel) {
		return "test";
	}
	
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data
	) {
		VoxelType::buildVertexData(chunk, location, voxel, data);
		buildVertexDataCalled();
	}

	MOCK_METHOD0(buildVertexDataCalled, void());
	
	MOCK_METHOD0(traitBuildVertexDataCalled, void());
	
};

TEST(VoxelWorld, voxelType) {
	Observer destructorObserver;
	TestVoxelType testVoxelType;
	VoxelChunk chunk({0, 0, 0});
	ASSERT_EQ(chunk.at(0, 0, 0).toString(), "empty");
	chunk.at(0, 0, 0).setType(testVoxelType);
	ASSERT_EQ(chunk.at(0, 0, 0).toString(), "test");
	
	EXPECT_CALL(testVoxelType, buildVertexDataCalled()).Times(1);
	EXPECT_CALL(testVoxelType, traitBuildVertexDataCalled()).Times(1);
	{
		std::vector<VoxelVertexData> data;
		chunk.at(0, 0, 0).buildVertexData(VoxelChunkExtendedRef(), InChunkVoxelLocation(), data);
	}
	
	((TestVoxelType::Data&) chunk.at(0, 0, 0)).destructorObserver = &destructorObserver;
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
		chunk.at(0, 0, 0).setType(testVoxelType);
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
		chunk.at(1, 0, 0).setType(testVoxelType);
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
		chunk.at(0, 1, 0).setType(testVoxelType);
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
		chunk.at(0, 0, 1).setType(testVoxelType);
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
		chunk.at(1, 1, 1).setType(testVoxelType);
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
		chunk.at(15, 15, 15).setType(testVoxelType);
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
		auto chunk = world.mutableChunk({0, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
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
		auto chunk = world.mutableChunk({0, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
		ASSERT_TRUE(chunk);
		ASSERT_FALSE(chunk.hasNeighbor(1, 0, 0));
		ASSERT_EQ(chunk.extendedAt(16, 0, 0).toString(), "empty");
		
		ASSERT_EQ(chunk.extendedAt(0, 0, 0).toString(), "empty");
		chunk.at(0, 0, 0).setType(testVoxelType);
		ASSERT_EQ(chunk.extendedAt(0, 0, 0).toString(), "test");
		ASSERT_EQ(chunk.extendedAt(1, 0, 0).toString(), "empty");
	}
	
	{
		auto chunk = world.mutableChunk({1, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
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
		auto chunk = world.extendedMutableChunk({0, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
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
