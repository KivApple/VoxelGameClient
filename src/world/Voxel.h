#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#ifndef HEADLESS
#include "../client/ShaderProgram.h"
#endif

struct VoxelVertexData {
	float x, y, z;
	float u, v;
};

#ifndef HEADLESS
class VoxelShaderProvider {
public:
	virtual ~VoxelShaderProvider() = default;
	[[nodiscard]] virtual int priority() const {
		return INT_MAX;
	}
	[[nodiscard]] virtual const CommonShaderProgram &get() const = 0;
	virtual void setup(const CommonShaderProgram &program) const = 0;
	
};

class VoxelTextureShaderProvider: public VoxelShaderProvider {
	std::variant<std::unique_ptr<GLTexture>, std::reference_wrapper<const GLTexture>> m_texture;

public:
	explicit VoxelTextureShaderProvider(const std::string &fileName);
	explicit VoxelTextureShaderProvider(const GLTexture &texture);
	[[nodiscard]] const CommonShaderProgram &get() const override;
	void setup(const CommonShaderProgram &program) const override;
	
};
#endif

struct Voxel;

class VoxelType {
public:
	virtual ~VoxelType() = default;
	virtual Voxel &invokeInit(void *ptr) = 0;
	virtual void invokeDestroy(Voxel &voxel) = 0;
	virtual std::string invokeToString(const Voxel &voxel) = 0;
#ifndef HEADLESS
	virtual const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) = 0;
#endif
	virtual void invokeBuildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) = 0;
	
};

struct Voxel {
	VoxelType &type;
	
	[[nodiscard]] std::string toString() const {
		return type.invokeToString(*this);
	}

#ifndef HEADLESS
	[[nodiscard]] const VoxelShaderProvider *shaderProvider() const {
		return type.invokeShaderProvider(*this);
	}
#endif
	
	void buildVertexData(std::vector<VoxelVertexData> &data) const {
		type.invokeBuildVertexData(*this, data);
	}
	
	void setType(VoxelType &newType) {
		type.invokeDestroy(*this);
		newType.invokeInit(this);
	}
	
};

static const size_t VOXEL_DATA_SIZE = sizeof(Voxel) + 16;

template<typename T, typename S> class ConcreteVoxelType: public VoxelType {
public:
	Voxel &invokeInit(void *ptr) final {
		static_assert(sizeof(S) <= VOXEL_DATA_SIZE);
		return *(new (ptr) S { *this });
	}
	
	void invokeDestroy(Voxel &voxel) final {
		(static_cast<S&>(voxel)).~S();
	}
	
	virtual std::string toString(const S &voxel) = 0;
	
	std::string invokeToString(const Voxel &voxel) final {
		return static_cast<T*>(this)->T::toString(static_cast<const S&>(voxel));
	}

#ifndef HEADLESS
	virtual const VoxelShaderProvider *shaderProvider(const S &voxel) = 0;
	
	const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) final {
		return static_cast<T*>(this)->T::shaderProvider(static_cast<const S&>(voxel));
	}
#endif
	
	virtual void buildVertexData(const S &voxel, std::vector<VoxelVertexData> &data) = 0;
	
	void invokeBuildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) final {
		static_cast<T*>(this)->T::buildVertexData(static_cast<const S&>(voxel), data);
	}
	
};

class EmptyVoxelType: public ConcreteVoxelType<EmptyVoxelType, Voxel> {
public:
	static EmptyVoxelType INSTANCE;
	
	std::string toString(const Voxel &voxel) final;
#ifndef HEADLESS
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel) final;
#endif
	void buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) final;
	
};

class SimpleVoxelType: public ConcreteVoxelType<SimpleVoxelType, Voxel>
#ifndef HEADLESS
		, public VoxelTextureShaderProvider
#endif
{
	std::string m_name;

public:
	SimpleVoxelType(std::string name, const std::string &textureFileName);

#ifndef HEADLESS
	SimpleVoxelType(std::string name, const GLTexture &texture);
#endif
	std::string toString(const Voxel &voxel) final;
#ifndef HEADLESS
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel) final;
#endif
	void buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) final;
	
};
