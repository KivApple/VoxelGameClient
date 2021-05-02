#include "Voxel.h"
#include "VoxelTypeRegistry.h"
#ifndef HEADLESS
#include "../client/GameEngine.h"
#endif

#ifndef HEADLESS
VoxelTextureShaderProvider::VoxelTextureShaderProvider(const GLTexture &texture): m_texture(texture) {
}
#endif

VoxelTextureShaderProvider::VoxelTextureShaderProvider(
		const std::string &fileName
)
#ifndef HEADLESS
: m_texture(std::make_unique<GLTexture>(fileName))
#endif
{
}

#ifndef HEADLESS
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

int VoxelTypeSerializationContext::typeId(const VoxelType &type) const {
	auto it = m_typeMap.find(&type);
	return it != m_typeMap.end() ? it->second : -1;
}

VoxelType &VoxelTypeSerializationContext::findTypeById(int id) const {
	return id >= 0 && id < m_types.size() ?
		m_types[id].second.get() :
		m_registry.get("unknown_" + std::to_string(id));
}

std::vector<std::string> VoxelTypeSerializationContext::names() const {
	std::vector<std::string> names;
	for (auto &type : m_types) {
		names.emplace_back(type.first);
	}
	return names;
}

EmptyVoxelType EmptyVoxelType::INSTANCE;

std::string EmptyVoxelType::toString(const Voxel &voxel) {
	return "empty";
}

const VoxelShaderProvider *EmptyVoxelType::shaderProvider(const Voxel &voxel) {
	return nullptr;
}

void EmptyVoxelType::buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) {
}

VoxelLightLevel EmptyVoxelType::lightLevel(const Voxel &voxel) {
	return 0;
}

void VoxelHolder::serialize(VoxelDeserializer &deserializer) {
	auto &voxel = get();
	auto savedPosition = deserializer.adapter().currentReadPos();
	EmptyVoxelType::INSTANCE.invokeDeserialize(voxel, deserializer);
	deserializer.adapter().currentReadPos(savedPosition);
	voxel.type->invokeDeserialize(voxel, deserializer);
}

SimpleVoxelType::SimpleVoxelType(
		std::string name,
		const std::string &textureFileName,
		bool unwrap
): m_name(std::move(name)), m_unwrap(unwrap), VoxelTextureShaderProvider(textureFileName) {
}

#ifndef HEADLESS
SimpleVoxelType::SimpleVoxelType(
		std::string
		name, const GLTexture &texture,
		bool unwrap
): m_name(std::move(name)), m_unwrap(unwrap), VoxelTextureShaderProvider(texture) {
}
#endif

std::string SimpleVoxelType::toString(const Voxel &voxel) {
	return m_name;
}

const VoxelShaderProvider *SimpleVoxelType::shaderProvider(const Voxel &voxel) {
	return this;
}

void SimpleVoxelType::buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) {
	if (m_unwrap) {
		data.insert(data.end(),{
				{-0.5, 0.5, -0.5, 1.0, 0.0},
				{0.5, 0.5, 0.5, 0.666667, 0.333333},
				{0.5, 0.5, -0.5, 0.666667, 0.0},
				{0.5, 0.5, 0.5, 0.333333, 0.333333},
				{-0.5, -0.5, 0.5, 0.0, 0.0},
				{0.5, -0.5, 0.5, 0.333333, 0.0},
				{-0.5, 0.5, 0.5, 0.333333, 0.666667},
				{-0.5, -0.5, -0.5, 0.0, 0.333333},
				{-0.5, -0.5, 0.5, 0.333333, 0.333333},
				{0.5, -0.5, -0.5, 0.666667, 0.0},
				{-0.5, -0.5, 0.5, 0.333333, 0.333333},
				{-0.5, -0.5, -0.5, 0.333333, 0.0},
				{0.5, 0.5, -0.5, 0.666667, 0.666667},
				{0.5, -0.5, 0.5, 0.333333, 0.333333},
				{0.5, -0.5, -0.5, 0.666667, 0.333333},
				{-0.5, 0.5, -0.5, 0.333333, 1.0},
				{0.5, -0.5, -0.5, 0.0, 0.666667},
				{-0.5, -0.5, -0.5, 0.333333, 0.666667},
				{-0.5, 0.5, -0.5, 1.0, 0.0},
				{-0.5, 0.5, 0.5, 1.0, 0.333333},
				{0.5, 0.5, 0.5, 0.666667, 0.333333},
				{0.5, 0.5, 0.5, 0.333333, 0.333333},
				{-0.5, 0.5, 0.5, 0.0, 0.333333},
				{-0.5, -0.5, 0.5, 0.0, 0.0},
				{-0.5, 0.5, 0.5, 0.333333, 0.666667},
				{-0.5, 0.5, -0.5, 0.0, 0.666667},
				{-0.5, -0.5, -0.5, 0.0, 0.333333},
				{0.5, -0.5, -0.5, 0.666667, 0.0},
				{0.5, -0.5, 0.5, 0.666667, 0.333333},
				{-0.5, -0.5, 0.5, 0.333333, 0.333333},
				{0.5, 0.5, -0.5, 0.666667, 0.666667},
				{0.5, 0.5, 0.5, 0.333333, 0.666667},
				{0.5, -0.5, 0.5, 0.333333, 0.333333},
				{-0.5, 0.5, -0.5, 0.333333, 1.0},
				{0.5, 0.5, -0.5, 0.0, 1.0},
				{0.5, -0.5, -0.5, 0.0, 0.666667}
		});
	} else {
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
}

VoxelLightLevel SimpleVoxelType::lightLevel(const Voxel &voxel) {
	return 0;
}
