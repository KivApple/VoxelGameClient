#pragma once

#include <climits>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <utility>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
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
class VoxelTypeRegistry;
class VoxelTypeSerializationContext;

class VoxelType {
public:
	virtual ~VoxelType() = default;
	virtual Voxel &invokeInit(void *ptr) = 0;
	virtual void invokeDestroy(Voxel &voxel) = 0;
	virtual void invokeSerialize(
			const Voxel &voxel,
			VoxelTypeSerializationContext &context,
			std::string &buffer
	) = 0;
	virtual void invokeDeserialize(
			Voxel &voxel,
			VoxelTypeSerializationContext &context,
			const std::string &buffer,
			size_t &offset
	) = 0;
	virtual std::string invokeToString(const Voxel &voxel) = 0;
#ifndef HEADLESS
	virtual const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) = 0;
#endif
	virtual void invokeBuildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) = 0;
	
};

class VoxelTypeSerializationContext {
	VoxelTypeRegistry &m_registry;
	std::vector<std::pair<std::string, std::reference_wrapper<VoxelType>>> m_types;
	std::unordered_map<const VoxelType*, int> m_typeMap;
	
public:
	explicit VoxelTypeSerializationContext(VoxelTypeRegistry &registry);
	int typeId(const VoxelType &type);
	VoxelType &findTypeById(int id);
	
};

class VoxelTypeSerializationExtension {
public:
	template<typename Ser, typename T, typename Fnc> void serialize(Ser& ser, const T& obj, Fnc&& fnc) const {
		auto &ctx = ser.template context<VoxelTypeSerializationContext>();
		uint16_t value = ctx.typeId(obj);
		ser.value2b(value);
	}
	
	template<typename Des, typename T, typename Fnc> void deserialize(Des& des, T& obj, Fnc&& fnc) const {
		auto &ctx = des.template context<VoxelTypeSerializationContext>();
		uint16_t value;
		des.value2b(value);
		obj = ctx.findTypeById(value);
	}
	
};

namespace bitsery::traits {
	template<typename T> struct ExtensionTraits<VoxelTypeSerializationExtension, T> {
		using TValue = void;
		static constexpr bool SupportValueOverload = false;
		static constexpr bool SupportObjectOverload = true;
		static constexpr bool SupportLambdaOverload = false;
	};
}

struct Voxel {
	std::reference_wrapper<VoxelType> type;
	
	[[nodiscard]] std::string toString() const {
		return type.get().invokeToString(*this);
	}

#ifndef HEADLESS
	[[nodiscard]] const VoxelShaderProvider *shaderProvider() const {
		return type.get().invokeShaderProvider(*this);
	}
#endif
	
	void buildVertexData(std::vector<VoxelVertexData> &data) const {
		type.get().invokeBuildVertexData(*this, data);
	}
	
	void setType(VoxelType &newType) {
		type.get().invokeDestroy(*this);
		newType.invokeInit(this);
	}
	
	void serialize(
			VoxelTypeSerializationContext &context,
			std::string &buffer
	) const {
		type.get().invokeSerialize(*this, context, buffer);
	}
	
	void deserialize(
			VoxelTypeSerializationContext &context,
			const std::string &buffer,
			size_t &offset
	);
	
	template<typename S> void serialize(S& s) {
		s.ext(type, VoxelTypeSerializationExtension {});
	}
	
};

static const size_t MAX_VOXEL_DATA_SIZE = sizeof(Voxel) + 16;

template<typename T, typename Data=Voxel, typename Base=VoxelType> class VoxelTypeHelper: public Base {
public:
	template<typename ...Args> explicit VoxelTypeHelper(Args&&... args): Base(std::forward<Args>(args)...) {
	}
	
	Voxel &invokeInit(void *ptr) override {
		static_assert(sizeof(Data) <= MAX_VOXEL_DATA_SIZE);
		return *(new (ptr) Data { *this });
	}
	
	void invokeDestroy(Voxel &voxel) override {
		(static_cast<Data&>(voxel)).~Data();
	}
	
	std::string invokeToString(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::toString(static_cast<const Data&>(voxel));
	}

#ifndef HEADLESS
	const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::shaderProvider(static_cast<const Data&>(voxel));
	}
#endif
	
	void invokeBuildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) override {
		static_cast<T*>(this)->T::buildVertexData(static_cast<const Data&>(voxel), data);
	}
	
	void invokeSerialize(
			const Voxel &voxel,
			VoxelTypeSerializationContext &context,
			std::string &buffer
	) override {
		auto pos = buffer.size();
		bitsery::OutputBufferAdapter<std::string> adapter(buffer);
		adapter.currentWritePos(pos);
		auto count = bitsery::quickSerialization(
				context,
				std::move(adapter),
				static_cast<const Data&>(voxel)
		);
		buffer.resize(count);
	}
	
	void invokeDeserialize(
			Voxel &voxel,
			VoxelTypeSerializationContext &context,
			const std::string &buffer,
			size_t &offset
	) override {
		bitsery::Deserializer<bitsery::InputBufferAdapter<std::string>, VoxelTypeSerializationContext> des(
				context,
				bitsery::InputBufferAdapter<std::string>(buffer.cbegin() + offset, buffer.cend())
		);
		invokeInit(&voxel);
		des.object(static_cast<Data&>(voxel));
		offset += des.adapter().currentReadPos();
	}
	
};

class EmptyVoxelType: public VoxelTypeHelper<EmptyVoxelType, Voxel> {
public:
	static EmptyVoxelType INSTANCE;
	
	std::string toString(const Voxel &voxel);
#ifndef HEADLESS
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel);
#endif
	void buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data);
	
};

class SimpleVoxelType: public VoxelTypeHelper<SimpleVoxelType>
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
	std::string toString(const Voxel &voxel);
#ifndef HEADLESS
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel);
#endif
	void buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data);
	
};
