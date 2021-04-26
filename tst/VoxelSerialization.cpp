#include "gtest/gtest.h"
#include "world/Voxel.h"
#include "world/VoxelTypeRegistry.h"
#include "world/VoxelChunk.h"

struct MyVoxel: public Voxel {
	int a = 10;
	int b = 20;
	
	template<typename S> void serialize(S &s) {
		Voxel::serialize(s);
		s.value4b(a);
	}
};

class MyVoxelType: public VoxelTypeHelper<MyVoxelType, MyVoxel> {
public:
	std::string toString(const MyVoxel &voxel) {
		return "test";
	}
	
	void buildVertexData(const MyVoxel &voxel, std::vector<VoxelVertexData> &data) {
	}
	
};

TEST(VoxelSerialization, simple) {
	VoxelTypeRegistry typeRegistry;
	typeRegistry.add("test", std::make_unique<MyVoxelType>());
	VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
	
	std::string buffer;
	
	{
		VoxelChunk chunk({0, 0, 0});
		chunk.initAt(0, 0, 0, typeRegistry.get("test"));
		((MyVoxel&) chunk.at(0, 0, 0)).a = 100;
		chunk.at(0, 0, 1).serialize(typeSerializationContext, buffer);
		chunk.at(0, 0, 0).serialize(typeSerializationContext, buffer);
	}
	
	printf("Serialized voxels (empty, test): ");
	for (auto c : buffer) {
		printf("%02X ", (unsigned char) c);
	}
	printf("\n");
	
	{
		VoxelChunk chunk({0, 0, 0});
		size_t offset = 0;
		chunk.at(0, 0, 1).deserialize(typeSerializationContext, buffer, offset);
		chunk.at(0, 0, 0).deserialize(typeSerializationContext, buffer, offset);
		EXPECT_EQ(offset, buffer.size());
		EXPECT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		EXPECT_EQ(chunk.at(0, 0, 0).toString(), "test");
		EXPECT_EQ(((MyVoxel&) chunk.at(0, 0, 0)).a, 100);
		EXPECT_EQ(((MyVoxel&) chunk.at(0, 0, 0)).b, 20);
	}
}
