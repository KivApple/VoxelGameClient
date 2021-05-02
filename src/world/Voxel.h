#pragma once

#include <climits>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>
#include <utility>
#include <typeinfo>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
#ifndef HEADLESS
#include "../client/ShaderProgram.h"
#endif

struct VoxelVertexData {
	float x, y, z;
	float u, v;
};

static const int MAX_VOXEL_SHADER_PRIORITY = INT_MAX;

class VoxelShaderProvider {
public:
	virtual ~VoxelShaderProvider() = default;
	[[nodiscard]] virtual int priority() const {
		return MAX_VOXEL_SHADER_PRIORITY;
	}
#ifndef HEADLESS
	[[nodiscard]] virtual const CommonShaderProgram &get() const = 0;
	virtual void setup(const CommonShaderProgram &program) const = 0;
#endif

};

class VoxelTextureShaderProvider: public VoxelShaderProvider {
#ifndef HEADLESS
	std::variant<std::unique_ptr<GLTexture>, std::reference_wrapper<const GLTexture>> m_texture;
#endif

public:
	explicit VoxelTextureShaderProvider(const std::string &fileName);
#ifndef HEADLESS
	explicit VoxelTextureShaderProvider(const GLTexture &texture);
	[[nodiscard]] const CommonShaderProgram &get() const override;
	void setup(const CommonShaderProgram &program) const override;
#endif

};

struct Voxel;
class VoxelTypeRegistry;
class VoxelTypeSerializationContext;

typedef bitsery::Serializer<bitsery::OutputBufferAdapter<std::string>, const VoxelTypeSerializationContext> VoxelSerializer;
typedef bitsery::Deserializer<bitsery::InputBufferAdapter<std::string>, const VoxelTypeSerializationContext> VoxelDeserializer;

typedef int8_t VoxelLightLevel;
static const VoxelLightLevel MAX_VOXEL_LIGHT_LEVEL = 16;

class VoxelType {
public:
	virtual ~VoxelType() = default;
	virtual Voxel &invokeInit(void *ptr) = 0;
	virtual void invokeDestroy(Voxel &voxel) = 0;
	virtual bool invokeCheckType(const std::type_info &typeInfo) {
		return false;
	}
	virtual void invokeSerialize(const Voxel &voxel, VoxelSerializer &serializer) = 0;
	virtual void invokeDeserialize(Voxel &voxel, VoxelDeserializer &deserializer) = 0;
	virtual std::string invokeToString(const Voxel &voxel) = 0;
	virtual const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) = 0;
	virtual void invokeBuildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) = 0;
	virtual VoxelLightLevel invokeLightLevel(const Voxel &voxel) = 0;
	
};

class VoxelTypeSerializationContext {
	VoxelTypeRegistry &m_registry;
	std::vector<std::pair<std::string, std::reference_wrapper<VoxelType>>> m_types;
	std::unordered_map<const VoxelType*, int> m_typeMap;
	
public:
	explicit VoxelTypeSerializationContext(VoxelTypeRegistry &registry);
	int typeId(const VoxelType &type) const;
	VoxelType &findTypeById(int id) const;
	std::vector<std::string> names() const;

	template<typename S> void serialize(S &s) const {
		s.ext(*this, SerializationHelper {});
	}
	template<typename S> void serialize(S &s) {
		s.ext(*this, SerializationHelper {});
	}
	
	class SerializationHelper {
	public:
		template<typename Ser, typename T, typename Fnc> void serialize(Ser& ser, const T& obj, Fnc&& fnc) const {
			ser.container(obj.names(), UINT16_MAX, [](auto &s, const std::string &name) {
				s.container1b(name, 127);
			});
		}
		
		template<typename Des, typename T, typename Fnc> void deserialize(Des& des, T& obj, Fnc&& fnc) const {
			std::vector<std::string> names;
			des.container(names, UINT16_MAX, [](auto &s, std::string &name) {
				s.container1b(name, 127);
			});
			obj.m_types.clear();
			obj.m_typeMap.clear();
			for (auto &name : names) {
				auto &type = obj.m_registry.get(name);
				obj.m_types.emplace_back(name, type);
				obj.m_typeMap.emplace(&type, (int) (obj.m_types.size() - 1));
			}
		}
	};
	
};

class VoxelTypeSerializationHelper {
public:
	template<typename Ser, typename T, typename Fnc> void serialize(Ser& ser, const T& obj, Fnc&& fnc) const {
		auto &ctx = ser.template context<const VoxelTypeSerializationContext>();
		uint16_t value = ctx.typeId(*obj);
		ser.value2b(value);
	}
	
	template<typename Des, typename T, typename Fnc> void deserialize(Des& des, T& obj, Fnc&& fnc) const {
		auto &ctx = des.template context<const VoxelTypeSerializationContext>();
		uint16_t value;
		des.value2b(value);
		obj = &ctx.findTypeById(value);
	}
	
};

namespace bitsery::traits {
	template<typename T> struct ExtensionTraits<VoxelTypeSerializationHelper, T> {
		using TValue = void;
		static constexpr bool SupportValueOverload = false;
		static constexpr bool SupportObjectOverload = true;
		static constexpr bool SupportLambdaOverload = false;
	};
	
	template<typename T> struct ExtensionTraits<VoxelTypeSerializationContext::SerializationHelper, T> {
		using TValue = void;
		static constexpr bool SupportValueOverload = false;
		static constexpr bool SupportObjectOverload = true;
		static constexpr bool SupportLambdaOverload = false;
	};
}

struct Voxel {
	VoxelType *type;
	VoxelLightLevel lightLevel = MAX_VOXEL_LIGHT_LEVEL;
	
	template<typename S> void serialize(S& s) {
		s.ext(type, VoxelTypeSerializationHelper {});
		s.value1b(lightLevel);
	}
	
};

static const size_t MAX_VOXEL_DATA_SIZE = sizeof(Voxel) + 16;

template<typename T, typename Data=Voxel, typename Base=VoxelType> class VoxelTypeHelper: public Base {
public:
	template<typename ...Args> explicit VoxelTypeHelper(Args&&... args): Base(std::forward<Args>(args)...) {
	}
	
	Voxel &invokeInit(void *ptr) override {
		static_assert(sizeof(Data) <= MAX_VOXEL_DATA_SIZE);
		return *(new (ptr) Data { this });
	}
	
	void invokeDestroy(Voxel &voxel) override {
		(static_cast<Data&>(voxel)).~Data();
	}
	
	bool invokeCheckType(const std::type_info &typeInfo) override {
		return typeid(Data) == typeInfo || Base::invokeCheckType(typeInfo);
	}
	
	std::string invokeToString(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::toString(static_cast<const Data&>(voxel));
	}

	const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::shaderProvider(static_cast<const Data&>(voxel));
	}
	
	void invokeBuildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data) override {
		static_cast<T*>(this)->T::buildVertexData(static_cast<const Data&>(voxel), data);
	}

	VoxelLightLevel invokeLightLevel(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::lightLevel(static_cast<const Data&>(voxel));
	}
	
	void invokeSerialize(const Voxel &voxel, VoxelSerializer &serializer) override {
		serializer.object(static_cast<const Data&>(voxel));
	}
	
	void invokeDeserialize(Voxel &voxel, VoxelDeserializer &deserializer) override {
		invokeInit(&voxel);
		deserializer.object(static_cast<Data&>(voxel));
	}
	
};

class EmptyVoxelType: public VoxelTypeHelper<EmptyVoxelType, Voxel> {
public:
	static EmptyVoxelType INSTANCE;
	
	std::string toString(const Voxel &voxel);
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel);
	void buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data);
	VoxelLightLevel lightLevel(const Voxel &voxel);
	
};

template<typename T> static inline bool checkVoxelType(const Voxel &voxel) {
	return voxel.type->invokeCheckType(typeid(T));
}

template<> bool checkVoxelType<Voxel>(const Voxel &voxel) {
	return true;
}

class VoxelHolder {
	char m_data[MAX_VOXEL_DATA_SIZE];

public:
	VoxelHolder(): VoxelHolder(EmptyVoxelType::INSTANCE) {
	}
	
	explicit VoxelHolder(VoxelType &type) {
		type.invokeInit(m_data);
	}
	
	VoxelHolder(VoxelHolder&) = delete;
	VoxelHolder &operator=(VoxelHolder&) = delete;
	
	~VoxelHolder() {
		get().type->invokeDestroy(get());
	}
	
	template<typename T=Voxel> [[nodiscard]] const T &get() const {
		assert(checkVoxelType<T>(*reinterpret_cast<const Voxel*>(m_data)));
		return *reinterpret_cast<const T*>(m_data);
	}
	
	template<typename T=Voxel> T &get() {
		assert(checkVoxelType<T>(*reinterpret_cast<const Voxel*>(m_data)));
		return *reinterpret_cast<T*>(m_data);
	}

	[[nodiscard]] VoxelType &type() const {
		return *get().type;
	}

	void setType(VoxelType &newType) {
		get().type->invokeDestroy(get());
		newType.invokeInit(m_data);
	}

	[[nodiscard]] VoxelLightLevel lightLevel() const {
		return get().lightLevel;
	}

	void setLightLevel(VoxelLightLevel level) {
		get().lightLevel = level;
	}

	[[nodiscard]] VoxelLightLevel typeLightLevel() const {
		return get().type->invokeLightLevel(get());
	}
	
	[[nodiscard]] std::string toString() const {
		return get().type->invokeToString(get());
	}

	[[nodiscard]] const VoxelShaderProvider *shaderProvider() const {
		return get().type->invokeShaderProvider(get());
	}
	
	void buildVertexData(std::vector<VoxelVertexData> &data) const {
		get().type->invokeBuildVertexData(get(), data);
	}
	
	void serialize(VoxelSerializer &serializer) const {
		get().type->invokeSerialize(get(), serializer);
	}
	
	void serialize(VoxelDeserializer &deserializer);
	
};

class SimpleVoxelType: public VoxelTypeHelper<SimpleVoxelType>, public VoxelTextureShaderProvider {
	std::string m_name;

public:
	SimpleVoxelType(std::string name, const std::string &textureFileName);

#ifndef HEADLESS
	SimpleVoxelType(std::string name, const GLTexture &texture);
#endif
	std::string toString(const Voxel &voxel);
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel);
	void buildVertexData(const Voxel &voxel, std::vector<VoxelVertexData> &data);
	VoxelLightLevel lightLevel(const Voxel &voxel);
	
};
