#include "Voxel.h"
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
