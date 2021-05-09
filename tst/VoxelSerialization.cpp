#include "gtest/gtest.h"
#include "world/Voxel.h"
#include "world/VoxelTypeRegistry.h"
#include "world/VoxelWorld.h"

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
	
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel) {
		return nullptr;
	}
	
	void buildVertexData(const MyVoxel &voxel, std::vector<VoxelVertexData> &data) {
	}

	VoxelLightLevel lightLevel(const MyVoxel &voxel) {
		return 0;
	}
	
	void slowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
	}
	
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		return false;
	}
	
};

TEST(VoxelSerialization, contextBasic) {
	VoxelTypeRegistry typeRegistry;
	VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
	
	EXPECT_EQ(typeSerializationContext.size(), 1);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	
	typeSerializationContext.setTypeId(2, "test3");
	EXPECT_EQ(typeSerializationContext.size(), 3);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	
	typeSerializationContext.setTypeId(0, "empty");
	EXPECT_EQ(typeSerializationContext.size(), 3);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	
	typeSerializationContext.setTypeId(1, "test1");
	EXPECT_EQ(typeSerializationContext.size(), 3);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test1"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	
	typeSerializationContext.setTypeId(3, "test3");
	EXPECT_EQ(typeSerializationContext.size(), 4);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test1"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 3);
	EXPECT_EQ(&typeSerializationContext.findTypeById(3), &typeRegistry.get("test3"));
	
	typeSerializationContext.setTypeId(2, "test2");
	EXPECT_EQ(typeSerializationContext.size(), 4);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test1"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test2")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test2"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 3);
	EXPECT_EQ(&typeSerializationContext.findTypeById(3), &typeRegistry.get("test3"));
	
	typeSerializationContext.setTypeId(1, "test2");
	typeSerializationContext.setTypeId(2, "test3");
	typeSerializationContext.setTypeId(3, "test1");
	EXPECT_EQ(typeSerializationContext.size(), 4);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test2")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test2"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 3);
	EXPECT_EQ(&typeSerializationContext.findTypeById(3), &typeRegistry.get("test1"));
	
	typeSerializationContext.setTypeId(1, "test4");
	EXPECT_EQ(typeSerializationContext.size(), 5);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 3);
	EXPECT_EQ(&typeSerializationContext.findTypeById(3), &typeRegistry.get("test1"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test2")), 4);
	EXPECT_EQ(&typeSerializationContext.findTypeById(4), &typeRegistry.get("test2"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test4")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test4"));
	
	typeSerializationContext.setTypeId(5, "test5");
	EXPECT_EQ(typeSerializationContext.size(), 6);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 3);
	EXPECT_EQ(&typeSerializationContext.findTypeById(3), &typeRegistry.get("test1"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test2")), 4);
	EXPECT_EQ(&typeSerializationContext.findTypeById(4), &typeRegistry.get("test2"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test4")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test4"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test5")), 5);
	EXPECT_EQ(&typeSerializationContext.findTypeById(5), &typeRegistry.get("test5"));
	
	typeRegistry.get("test6");
	
	typeSerializationContext.update();
	EXPECT_EQ(typeSerializationContext.size(), 7);
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), 0);
	EXPECT_EQ(&typeSerializationContext.findTypeById(0), &typeRegistry.get("empty"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test1")), 3);
	EXPECT_EQ(&typeSerializationContext.findTypeById(3), &typeRegistry.get("test1"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test2")), 4);
	EXPECT_EQ(&typeSerializationContext.findTypeById(4), &typeRegistry.get("test2"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test3")), 2);
	EXPECT_EQ(&typeSerializationContext.findTypeById(2), &typeRegistry.get("test3"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test4")), 1);
	EXPECT_EQ(&typeSerializationContext.findTypeById(1), &typeRegistry.get("test4"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test5")), 5);
	EXPECT_EQ(&typeSerializationContext.findTypeById(5), &typeRegistry.get("test5"));
	EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test6")), 6);
	EXPECT_EQ(&typeSerializationContext.findTypeById(6), &typeRegistry.get("test6"));
}

TEST(VoxelSerialization, context) {
	std::string buffer;
	
	int emptyTypeId, testTypeId;
	
	{
		VoxelTypeRegistry typeRegistry;
		typeRegistry.add("test", std::make_unique<MyVoxelType>());
		VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
		emptyTypeId = typeSerializationContext.typeId(typeRegistry.get("empty"));
		testTypeId = typeSerializationContext.typeId(typeRegistry.get("test"));
		
		bitsery::Serializer<bitsery::OutputBufferAdapter<std::string>> serializer(buffer);
		serializer.object(typeSerializationContext);
		buffer.resize(serializer.adapter().currentWritePos());
	}
	
	printf("Serialized voxel serialization context (empty=%i, test=%i): ", emptyTypeId, testTypeId);
	for (auto c : buffer) {
		printf("%02X ", (unsigned char) c);
	}
	printf("\n");
	
	{
		VoxelTypeRegistry typeRegistry;
		VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
		
		bitsery::Deserializer<bitsery::InputBufferAdapter<std::string>> deserializer(buffer.cbegin(), buffer.cend());
		deserializer.object(typeSerializationContext);
		EXPECT_EQ(deserializer.adapter().currentReadPos(), buffer.size());
		
		EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("empty")), emptyTypeId);
		EXPECT_EQ(typeSerializationContext.typeId(typeRegistry.get("test")), testTypeId);
	}
}

TEST(VoxelSerialization, holder) {
	VoxelTypeRegistry typeRegistry;
	typeRegistry.add("test", std::make_unique<MyVoxelType>());
	VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
	
	std::string buffer;
	
	{
		VoxelHolder voxel1, voxel2(typeRegistry.get("test"));
		voxel2.get<MyVoxel>().a = 100;
		
		VoxelSerializer serializer(typeSerializationContext, buffer);
		serializer.object(voxel1);
		serializer.object(voxel2);
		buffer.resize(serializer.adapter().currentWritePos());
	}
	
	printf("Serialized voxels (empty, test): ");
	for (auto c : buffer) {
		printf("%02X ", (unsigned char) c);
	}
	printf("\n");
	
	{
		VoxelHolder voxel1, voxel2;
		
		VoxelDeserializer deserializer(typeSerializationContext, buffer.cbegin(), buffer.cend());
		deserializer.object(voxel1);
		deserializer.object(voxel2);
		EXPECT_EQ(deserializer.adapter().currentReadPos(), buffer.size());
		
		EXPECT_EQ(voxel1.toString(), "empty");
		EXPECT_EQ(voxel2.toString(), "test");
		EXPECT_EQ(voxel2.get<MyVoxel>().a, 100);
		EXPECT_EQ(voxel2.get<MyVoxel>().b, 20);
	}
}

TEST(VoxelSerialization, chunk) {
	VoxelTypeRegistry typeRegistry;
	typeRegistry.add("test", std::make_unique<MyVoxelType>());
	VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
	
	std::string buffer;
	
	{
		VoxelChunk chunk({0, 0, 0});
		chunk.at(7, 11, 13).setType(typeRegistry.get("test"));
		chunk.at(7, 11, 13).get<MyVoxel>().a = 100;
		
		VoxelSerializer serializer(typeSerializationContext, buffer);
		serializer.object(chunk);
		buffer.resize(serializer.adapter().currentWritePos());
	}
	
	printf("Serialized chunk size: %zi bytes\n", buffer.size());
	
	{
		VoxelChunk chunk({0, 0, 0});
		
		VoxelDeserializer deserializer(typeSerializationContext, buffer.cbegin(), buffer.cend());
		deserializer.object(chunk);
		EXPECT_EQ(deserializer.adapter().currentReadPos(), buffer.size());
		
		EXPECT_EQ(chunk.at(6, 11, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 10, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 11, 12).toString(), "empty");
		EXPECT_EQ(chunk.at(8, 11, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 12, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 11, 14).toString(), "empty");
		
		EXPECT_EQ(chunk.at(7, 11, 13).toString(), "test");
		EXPECT_EQ(chunk.at(7, 11, 13).get<MyVoxel>().a, 100);
	}
}

TEST(VoxelSerialization, world) {
	VoxelTypeRegistry typeRegistry;
	typeRegistry.add("test", std::make_unique<MyVoxelType>());
	VoxelTypeSerializationContext typeSerializationContext(typeRegistry);
	
	std::string buffer;
	
	{
		VoxelWorld world;
		
		{
			auto chunk = world.mutableChunk({0, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
			chunk.at(7, 11, 13).setType(typeRegistry.get("test"));
			chunk.at(7, 11, 13).get<MyVoxel>().a = 100;
		}
		
		{
			auto chunk = world.chunk({0, 0, 0});
			VoxelSerializer serializer(typeSerializationContext, buffer);
			serializer.object(chunk);
			buffer.resize(serializer.adapter().currentWritePos());
		}
	}
	
	printf("Serialized chunk size: %zi bytes\n", buffer.size());
	
	{
		VoxelWorld world;
		auto chunk = world.mutableChunk({0, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
		
		VoxelDeserializer deserializer(typeSerializationContext, buffer.cbegin(), buffer.cend());
		deserializer.object(chunk);
		EXPECT_EQ(deserializer.adapter().currentReadPos(), buffer.size());
		
		EXPECT_EQ(chunk.at(6, 11, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 10, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 11, 12).toString(), "empty");
		EXPECT_EQ(chunk.at(8, 11, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 12, 13).toString(), "empty");
		EXPECT_EQ(chunk.at(7, 11, 14).toString(), "empty");
		
		EXPECT_EQ(chunk.at(7, 11, 13).toString(), "test");
		EXPECT_EQ(chunk.at(7, 11, 13).get<MyVoxel>().a, 100);
	}
}
