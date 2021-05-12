#include <easylogging++.h>
#include "Voxel.h"
#include "VoxelTypeRegistry.h"
#ifndef HEADLESS
#include "../client/GameEngine.h"
#endif

#ifndef HEADLESS
VoxelTextureShaderProvider::VoxelTextureShaderProvider(const GL::Texture &texture): m_texture(texture) {
}
#endif

VoxelTextureShaderProvider::VoxelTextureShaderProvider(
		const std::string &fileName
)
#ifndef HEADLESS
: m_texture(std::make_unique<GL::Texture>(fileName))
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
		
		void operator()(const std::unique_ptr<GL::Texture> &texture) {
			program.setTexImage(*texture);
		}
		
		void operator()(const std::reference_wrapper<const GL::Texture> &texture) {
			program.setTexImage(texture);
		}
	} visitor { program };
	
	std::visit(visitor, m_texture);
}
#endif

VoxelTypeSerializationContext::VoxelTypeSerializationContext(VoxelTypeRegistry &registry): m_registry(registry) {
	update();
}

void VoxelTypeSerializationContext::update() {
	m_registry.forEach([this] (const std::string &name, VoxelTypeInterface &type) {
		if (m_typeMap.emplace(&type, (int) m_types.size()).second) {
			m_types.emplace_back(name, type);
		}
	});
}

void VoxelTypeSerializationContext::setTypeId(int id, const std::string &name) {
	if (id < 0 || id > UINT16_MAX) return;
	if (id == 0 && name != "empty") return;
	auto &type = m_registry.get(name);
	//LOG(TRACE) << "Registering type \"" << name << "\" " << &type << " with id=" << id;
	auto it = m_typeMap.find(&type);
	if (it != m_typeMap.end()) {
		if (it->second == id) {
			//LOG(TRACE) << "Type \"" << name << "\" already has id=" << id;
			return;
		} else {
			//LOG(TRACE) << "Type \"" << name << "\" has id=" << it->second;
		}
	}
	if (id >= m_types.size()) {
		m_types.reserve(id + 1);
		while (m_types.size() < id) {
			//LOG(TRACE) << "Adding empty voxel type with id=" << m_types.size();
			m_types.emplace_back("empty", EmptyVoxelType::INSTANCE);
		}
		//LOG(TRACE) << "Adding \"" << name << "\"" << " at the end (id=" << m_types.size() << ")";
		m_types.emplace_back(name, type);
		if (it != m_typeMap.end()) {
			auto prevId = it->second;
			//LOG(TRACE) << "Removing \"" << m_types[prevId].first << "\" with id=" << prevId;
			m_types[prevId].first = "empty";
			m_types[prevId].second = EmptyVoxelType::INSTANCE;
		}
	} else {
		auto &oldType = m_types[id].second.get();
		if (&oldType != &EmptyVoxelType::INSTANCE) {
			if (it != m_typeMap.end()) {
				auto prevId = it->second;
				//LOG(TRACE) << "Move \"" << m_types[id].first << "\" " << &type << " into id=" << prevId;
				m_typeMap[&oldType] = prevId;
				m_types[prevId] = std::move(m_types[id]);
			} else {
				auto newId = m_types.size();
				//LOG(TRACE) << "Move \"" << m_types[id].first << "\" into id=" << newId << " (push_back)";
				m_typeMap[&oldType] = newId;
				m_types.emplace_back(std::move(m_types[id]));
			}
		}
		m_types[id].first = name;
		m_types[id].second = type;
		//LOG(TRACE) << "Set \"" << name << "\" into id=" << id;
	}
	m_typeMap[&type] = id;
}

int VoxelTypeSerializationContext::typeId(const VoxelTypeInterface &type) const {
	auto it = m_typeMap.find(&type);
	return it != m_typeMap.end() ? it->second : -1;
}

VoxelTypeInterface &VoxelTypeSerializationContext::findTypeById(int id) const {
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

void EmptyVoxelType::buildVertexData(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		const Voxel &voxel,
		std::vector<VoxelVertexData> &data
) {
}

VoxelLightLevel EmptyVoxelType::lightLevel(const Voxel &voxel) {
	return 0;
}

void EmptyVoxelType::slowUpdate(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &voxel,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
}

bool EmptyVoxelType::update(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &voxel,
		unsigned long deltaTime,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	return false;
}

bool EmptyVoxelType::hasDensity(const Voxel &voxel) {
	return true;
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
		bool unwrap,
		VoxelLightLevel lightLevel,
		bool transparent,
		bool hasDensity
): m_name(std::move(name)), m_unwrap(unwrap), m_lightLevel(lightLevel), m_transparent(transparent),
	m_hasDensity(hasDensity), VoxelTextureShaderProvider(textureFileName)
{
}

#ifndef HEADLESS
SimpleVoxelType::SimpleVoxelType(
		std::string
		name, const GL::Texture &texture,
		bool unwrap,
		VoxelLightLevel lightLevel,
		bool transparent,
		bool hasDensity
): m_name(std::move(name)), m_unwrap(unwrap), m_lightLevel(lightLevel), m_transparent(transparent),
   m_hasDensity(hasDensity), VoxelTextureShaderProvider(texture)
{
}
#endif

std::string SimpleVoxelType::toString(const Voxel &voxel) {
	return m_name;
}

const VoxelShaderProvider *SimpleVoxelType::shaderProvider(const Voxel &voxel) {
	return this;
}

void SimpleVoxelType::buildVertexData(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		const Voxel &voxel,
		std::vector<VoxelVertexData> &data
) {
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
	return m_lightLevel;
}

int SimpleVoxelType::priority() const {
	return m_transparent ? MAX_VOXEL_SHADER_PRIORITY - 1 : MAX_VOXEL_SHADER_PRIORITY;
}

void SimpleVoxelType::slowUpdate(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &voxel,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
}

bool SimpleVoxelType::update(
		const VoxelChunkExtendedMutableRef &chunk,
		const InChunkVoxelLocation &location,
		Voxel &voxel,
		unsigned long deltaTime,
		std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
) {
	return false;
}

bool SimpleVoxelType::hasDensity(const Voxel &voxel) {
	return m_hasDensity;
}
