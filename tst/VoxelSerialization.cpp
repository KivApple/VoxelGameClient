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
		
		VoxelSerializer serializer(typeSerializationContext, buffer);
		chunk.at(0, 0, 1).doSerialize(serializer);
		chunk.at(0, 0, 0).doSerialize(serializer);
		buffer.resize(serializer.adapter().currentWritePos());
	}
	
	printf("Serialized voxels (empty, test): ");
	for (auto c : buffer) {
		printf("%02X ", (unsigned char) c);
	}
	printf("\n");
	
	{
		VoxelChunk chunk({0, 0, 0});
		
		VoxelDeserializer deserializer(typeSerializationContext, buffer.cbegin(), buffer.cend());
		chunk.at(0, 0, 1).doDeserialize(deserializer);
		chunk.at(0, 0, 0).doDeserialize(deserializer);
		EXPECT_EQ(deserializer.adapter().currentReadPos(), buffer.size());
		
		EXPECT_EQ(chunk.at(0, 0, 1).toString(), "empty");
		EXPECT_EQ(chunk.at(0, 0, 0).toString(), "test");
		EXPECT_EQ(((MyVoxel&) chunk.at(0, 0, 0)).a, 100);
		EXPECT_EQ(((MyVoxel&) chunk.at(0, 0, 0)).b, 20);
	}
}
