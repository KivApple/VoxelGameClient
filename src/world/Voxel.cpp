#include "Voxel.h"
#include "VoxelTypeRegistry.h"
#ifndef HEADLESS
#include "../client/GameEngine.h"
#endif

#ifndef HEADLESS
VoxelTextureShaderProvider::VoxelTextureShaderProvider(const GLTexture &texture): m_texture(texture) {
}

VoxelTextureShaderProvider::VoxelTextureShaderProvider(
		const std::string &fileName
): m_texture(std::make_unique<GLTexture>(fileName)) {
}

const CommonShaderProgram &VoxelTextureShaderProvider::get() const {
	return GameEngine::instance().commonShaderPrograms().texture;
}

void VoxelTextureShaderProvider::setup(const CommonShaderProgram &program) const {
	struct {
		const CommonShaderProgram &program;
		
		void operator()(const std::unique_ptr<GLTexture> &texture) {
			program.setTexImage(*texture);
		}
		
		void operator()(const std::reference_wrapper<const GLTexture> &texture) {
			program.setTexImage(texture);
		}
	} visitor { program };
	
	std::visit(visitor, m_texture);
}
#endif

VoxelTypeSerializationContext::VoxelTypeSerializationContext(VoxelTypeRegistry &registry): m_registry(registry) {
	registry.forEach([this] (const std::string &name, VoxelType &type) {
		m_types.emplace_back(name, type);
		m_typeMap.emplace(&type, (int) (m_types.size() - 1));
	});
}

int VoxelTypeSerializationContext::typeId(const VoxelType &type) {
	auto it = m_typeMap.find(&type);
	return it != m_typeMap.end() ? it->second : -1;
}

VoxelType &VoxelTypeSerializationContext::findTypeById(int id) {
	return id >= 0 && id <= m_types.size() ?
		m_types[id].second.get() :
		m_registry.get("unknown_" + std::to_string(id));
}

EmptyVoxelType EmptyVoxelType::INSTANCE;

std::string EmptyVoxelType::toString(const Voxel &voxel) {
	return "empty";
}

#ifndef HEADLESS
const VoxelShaderProvider *EmptyVoxelType::shaderProvider(const Voxel &voxel) {
	return nullptr;
}
#endif

void EmptyVoxelType::buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) {
}

void VoxelHolder::serialize(VoxelDeserializer &deserializer) {
	auto savedPosition = deserializer.adapter().currentReadPos();
	EmptyVoxelType::INSTANCE.invokeDeserialize(get(), deserializer);
	deserializer.adapter().currentReadPos(savedPosition);
	get().type->invokeDeserialize(get(), deserializer);
}

SimpleVoxelType::SimpleVoxelType(
		std::string name,
		const std::string &textureFileName
): m_name(std::move(name))
#ifndef HEADLESS
	, VoxelTextureShaderProvider(textureFileName)
#endif
{
}

#ifndef HEADLESS
SimpleVoxelType::SimpleVoxelType(
		std::string
		name, const GLTexture &texture
): m_name(std::move(name)), VoxelTextureShaderProvider(texture) {
}
#endif

std::string SimpleVoxelType::toString(const Voxel &voxel) {
	return m_name;
}

#ifndef HEADLESS
const VoxelShaderProvider *SimpleVoxelType::shaderProvider(const Voxel &voxel) {
	return this;
}
#endif

void SimpleVoxelType::buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) {
	data.insert(data.end(), {
			{-0.5, 0.5, -0.5, 1.0, 1.0},
			{0.5, 0.5, 0.5, 0.0, 0.0},
			{0.5, 0.5, -0.5, 1.0, 0.0},
			{0.5, 0.5, 0.5, 1.0, 1.0},
			{-0.5, -0.5, 0.5, 0.0, 0.0},
			{0.5, -0.5, 0.5, 1.0, 0.0},
			{-0.5, 0.5, 0.5, 1.0, 1.0},
			{-0.5, -0.5, -0.5, 0.0, 0.0},
			{-0.5, -0.5, 0.5, 1.0, 0.0},
			{0.5, -0.5, -0.5, 1.0, 1.0},
			{-0.5, -0.5, 0.5, 0.0, 0.0},
			{-0.5, -0.5, -0.5, 1.0, 0.0},
			{0.5, 0.5, -0.5, 1.0, 1.0},
			{0.5, -0.5, 0.5, 0.0, 0.0},
			{0.5, -0.5, -0.5, 1.0, 0.0},
			{-0.5, 0.5, -0.5, 1.0, 1.0},
			{0.5, -0.5, -0.5, 0.0, 0.0},
			{-0.5, -0.5, -0.5, 1.0, 0.0},
			{-0.5, 0.5, -0.5, 1.0, 1.0},
			{-0.5, 0.5, 0.5, 0.0, 1.0},
			{0.5, 0.5, 0.5, 0.0, 0.0},
			{0.5, 0.5, 0.5, 1.0, 1.0},
			{-0.5, 0.5, 0.5, 0.0, 1.0},
			{-0.5, -0.5, 0.5, 0.0, 0.0},
			{-0.5, 0.5, 0.5, 1.0, 1.0},
			{-0.5, 0.5, -0.5, 0.0, 1.0},
			{-0.5, -0.5, -0.5, 0.0, 0.0},
			{0.5, -0.5, -0.5, 1.0, 1.0},
			{0.5, -0.5, 0.5, 0.0, 1.0},
			{-0.5, -0.5, 0.5, 0.0, 0.0},
			{0.5, 0.5, -0.5, 1.0, 1.0},
			{0.5, 0.5, 0.5, 0.0, 1.0},
			{0.5, -0.5, 0.5, 0.0, 0.0},
			{-0.5, 0.5, -0.5, 1.0, 1.0},
			{0.5, 0.5, -0.5, 0.0, 1.0},
			{0.5, -0.5, -0.5, 0.0, 0.0}
	});
}
