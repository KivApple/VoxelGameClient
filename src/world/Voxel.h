#pragma once

#include <climits>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <functional>
#include <utility>
#include <type_traits>
#include <typeinfo>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
#ifndef HEADLESS
#include "../client/ShaderProgram.h"
#endif

class Asset;
struct InChunkVoxelLocation;
class VoxelChunkExtendedRef;
class VoxelChunkExtendedMutableRef;
class VoxelTypeRegistry;

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
	std::variant<std::unique_ptr<GL::Texture>, std::reference_wrapper<const GL::Texture>> m_texture;
#endif

public:
	explicit VoxelTextureShaderProvider(Asset asset);
#ifndef HEADLESS
	explicit VoxelTextureShaderProvider(const GL::Texture &texture);
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

class VoxelTypeInterface {
public:
	virtual ~VoxelTypeInterface() = default;
	virtual void invokeHandleRegistration(const std::string &name, VoxelTypeRegistry &registry) = 0;
	virtual void invokeLink(VoxelTypeRegistry &registry) = 0;
	virtual Voxel &invokeInit(void *ptr) = 0;
	virtual Voxel &invokeInit(void *ptr, const Voxel &voxel) = 0;
	virtual Voxel &invokeInit(void *ptr, Voxel &&voxel) = 0;
	virtual void invokeDestroy(Voxel &voxel) = 0;
	virtual const void *invokeTraitState(const Voxel &voxel, const std::type_info &typeInfo) = 0;
	virtual void invokeSerialize(const Voxel &voxel, VoxelSerializer &serializer) = 0;
	virtual void invokeDeserialize(Voxel &voxel, VoxelDeserializer &deserializer) = 0;
	virtual std::string invokeToString(const Voxel &voxel) = 0;
	virtual const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) = 0;
	virtual void invokeBuildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const Voxel &voxel,
			std::vector<VoxelVertexData> &data
	) = 0;
	virtual VoxelLightLevel invokeLightLevel(const Voxel &voxel) = 0;
	virtual void invokeSlowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) = 0;
	virtual bool invokeUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) = 0;
	virtual bool invokeHasDensity(const Voxel &voxel) = 0;
	
};

class VoxelTypeSerializationContext {
	VoxelTypeRegistry &m_registry;
	std::vector<std::pair<std::string, std::reference_wrapper<VoxelTypeInterface>>> m_types;
	std::unordered_map<const VoxelTypeInterface*, int> m_typeMap;

public:
	explicit VoxelTypeSerializationContext(VoxelTypeRegistry &registry);
	int typeId(const VoxelTypeInterface &type) const;
	VoxelTypeInterface &findTypeById(int id) const;
	std::vector<std::string> names() const;
	void setTypeId(int id, const std::string &name);
	[[nodiscard]] int size() const {
		return m_types.size();
	}
	void update();
	
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
	VoxelTypeInterface *type;
	VoxelLightLevel lightLevel = MAX_VOXEL_LIGHT_LEVEL;
	
	template<typename S> void serialize(S& s) {
		s.ext(type, VoxelTypeSerializationHelper {});
		s.value1b(lightLevel);
	}
	
};

static const size_t MAX_VOXEL_DATA_SIZE = sizeof(Voxel) + 16;

struct EmptyVoxelTypeTraitState {
	template<typename S> void serialize(S &s) {
	}
};

template<typename StateType=EmptyVoxelTypeTraitState> class VoxelTypeTrait {
	VoxelTypeInterface *m_type = nullptr;
	
	void setType(VoxelTypeInterface *type) {
		m_type = type;
	}
	
	template<typename T, typename BaseState, typename ...Traits> friend class VoxelType;
	
protected:
	[[nodiscard]] VoxelTypeInterface &type() const {
		return *m_type;
	}
	
public:
	struct StateWrapper: public StateType {
	};
	
	typedef StateWrapper State;
	
	VoxelTypeTrait() = default;
	VoxelTypeTrait(const VoxelTypeTrait&) = delete;
	VoxelTypeTrait(VoxelTypeTrait&&) noexcept = default;
	const void *traitState(const State &state, const std::type_info &typeInfo) {
		if (typeInfo == typeid(State)) {
			return &state;
		}
		return nullptr;
	}
	
};

template<typename ...Traits> struct traitsHaveVoxelInterface {
	template<typename Trait, typename ...RestTraits> constexpr static bool performCheck(
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (std::is_base_of<VoxelTypeInterface, Trait>::value) {
			return true;
		}
		return performCheck(restTraits...);
	}
	constexpr static bool performCheck() {
		return false;
	}
	
	static constexpr bool value = performCheck(static_cast<Traits*>(0)...);
	
};

template<
		typename T,
		typename BaseState=EmptyVoxelTypeTraitState,
		typename ...Traits
> class VoxelType: public std::conditional<
		traitsHaveVoxelInterface<Traits...>::value,
		VoxelTypeTrait<>,
		VoxelTypeInterface
>::type, public Traits... {
	template<typename Args> struct ExpandStates { // Workaround for MSVC compiler bug
		typedef typename Args::State State;
	};
	
	struct BaseStateWrapper: public BaseState {
	};
	
	VoxelTypeInterface *m_type = nullptr;
	
	void setType(VoxelTypeInterface *type) {
		m_type = type;
		traitsSetType(type, static_cast<Traits*>(this)...);
	}
	
	template<typename T2, typename BaseState2, typename ...Traits2> friend class VoxelType;

protected:
	[[nodiscard]] VoxelTypeInterface &type() const {
		return *m_type;
	}

public:
	struct State: public BaseStateWrapper, public ExpandStates<Traits>::State... {
		template<typename S> void serialize(S &s) {
			s.object(*static_cast<BaseStateWrapper*>(this));
			traitsSerialize(s, static_cast<typename ExpandStates<Traits>::State*>(this)...);
		}
	
	private:
		template<typename S, typename TraitState, typename ...RestTraitStates> void traitsSerialize(
				S &s,
				TraitState *state,
				RestTraitStates... restTraits
		) {
			s.object(*state);
			traitsSerialize(s, restTraits...);
		}
		
		template<typename S> void traitsSerialize(S &s) {
		}
		
	};
	
	struct Data: public Voxel, public State {
		template<typename S> void serialize(S &s) {
			s.object(*static_cast<Voxel*>(this));
			s.object(*static_cast<State*>(this));
		}
	};

private:
	template<typename Trait, typename ...RestTraits> void traitsSetType(
			VoxelTypeInterface *type,
			Trait *trait,
			RestTraits... restTraits
	) {
		trait->setType(type);
		traitsSetType(type, restTraits...);
	}
	void traitsSetType(VoxelTypeInterface *type) {
	}
	
	template<typename Trait> static constexpr auto traitHasHandleRegistration(
			Trait*,
			const std::string *name,
			VoxelTypeRegistry *registry
	) -> decltype(std::declval<Trait>().handleRegistration(*name, *registry), true) {
		return true;
	}
	static constexpr auto traitHasHandleRegistration(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsHandleRegistration(
			const std::string &name,
			VoxelTypeRegistry &registry,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasHandleRegistration(
				(Trait*) 0,
				(const std::string*) 0,
				(VoxelTypeRegistry*) 0
		)) {
			trait->handleRegistration(name, registry);
		}
		traitsHandleRegistration(name, registry, restTraits...);
	}
	void traitsHandleRegistration(const std::string &name, VoxelTypeRegistry &registry) {
	}
	
	template<typename Trait> static constexpr auto traitHasLink(
			Trait*,
			VoxelTypeRegistry *registry
	) -> decltype(std::declval<Trait>().link(*registry), true) {
		return true;
	}
	static constexpr auto traitHasLink(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsLink(
			VoxelTypeRegistry &registry,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasLink((Trait*) 0, (VoxelTypeRegistry*) 0)) {
			trait->link(registry);
		}
		traitsLink(registry, restTraits...);
	}
	void traitsLink(VoxelTypeRegistry &registry) {
	}
	
	template<typename Trait, typename ...RestTraits> const void *traitsTraitState(
			const State &state,
			const std::type_info &typeInfo,
			Trait *trait,
			RestTraits... restTraits
	) {
		auto ptr = trait->traitState(state, typeInfo);
		if (ptr != nullptr) {
			return ptr;
		}
		return traitsTraitState(state, typeInfo, restTraits...);
	}
	const void *traitsTraitState(const State &state, const std::type_info &typeInfo) {
		return nullptr;
	}
	
	template<typename Trait> static constexpr auto traitHasToString(
			Trait*,
			const State* voxel
	) -> decltype(std::declval<Trait>().toString(*voxel), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasToString(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsToString(
			const State &voxel,
			std::string &result,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasToString<Trait>((Trait*) 0, (const State*) 0)) {
			result += trait->toString(voxel);
		}
		traitsToString(voxel, result, restTraits...);
	}
	void traitsToString(const State &voxel, std::string &result) {
	}
	
	template<typename Trait> static constexpr auto traitHasShaderProvider(
			Trait*,
			const State *voxel
	) -> decltype(std::declval<Trait>().shaderProvider(*voxel), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasShaderProvider(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> const VoxelShaderProvider *traitsShaderProvider(
			const State &voxel,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasShaderProvider<Trait>((Trait*) 0, (const State*) 0)) {
			return trait->shaderProvider(voxel);
		}
		return traitsShaderProvider(voxel, restTraits...);
	}
	const VoxelShaderProvider *traitsShaderProvider(const State &voxel) {
		return nullptr;
	}
	
	template<typename Trait> static constexpr auto traitHasBuildVertexData(
			Trait*,
			const VoxelChunkExtendedRef *chunk,
			const InChunkVoxelLocation *location,
			const State *voxel,
			std::vector<VoxelVertexData> *data
	) -> decltype(std::declval<Trait>().buildVertexData(*chunk, *location, *voxel, *data), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasBuildVertexData(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsBuildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasBuildVertexData<Trait>(
				(Trait*) 0,
				(const VoxelChunkExtendedRef*) 0,
				(const InChunkVoxelLocation*) 0,
				(const State*) 0,
				(std::vector<VoxelVertexData>*) 0
		)) {
			trait->buildVertexData(chunk, location, voxel, data);
		}
		traitsBuildVertexData(chunk, location, voxel, data, restTraits...);
	}
	void traitsBuildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data
	) {
	}
	
	template<typename Trait> static constexpr auto traitHasLightLevel(
			Trait*,
			State *voxel
	) -> decltype(std::declval<Trait>().lightLevel(*voxel), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasLightLevel(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> VoxelLightLevel traitsLightLevel(
			const State &voxel,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasLightLevel<Trait>((Trait*) 0, (const State*) 0)) {
			return trait->lightLevel(voxel);
		}
		return traitsLightLevel(voxel, restTraits...);
	}
	VoxelLightLevel traitsLightLevel(const State &voxel) {
		return 0;
	}
	
	template<typename Trait> static constexpr auto traitHasHasDensity(
			Trait*,
			const State *voxel
	) -> decltype(std::declval<Trait>().hasDensity(*voxel), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasHasDensity(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> bool traitsHasDensity(
			const State &voxel,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasHasDensity<Trait>((Trait*) 0, (const State*) 0)) {
			return trait->hasDensity(voxel);
		}
		return traitsHasDensity(voxel, restTraits...);
	}
	bool traitsHasDensity(const State &voxel) {
		return true;
	}
	
	template<typename Trait> static constexpr auto traitHasUpdate(
			Trait*,
			const VoxelChunkExtendedMutableRef *chunk,
			const InChunkVoxelLocation *location,
			Voxel *rawVoxel,
			State *voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> *invalidatedLocations
	) -> decltype(std::declval<Trait>().update(
			*chunk,
			*location,
			*rawVoxel,
			*voxel,
			deltaTime,
			*invalidatedLocations
	), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasUpdate(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> bool traitsUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasUpdate<Trait>(
				(Trait*) 0,
				(const VoxelChunkExtendedMutableRef*) 0,
				(const InChunkVoxelLocation*) 0,
				(Voxel*) 0,
				(State*) 0,
				0,
				(std::unordered_set<InChunkVoxelLocation>*) 0
		)) {
			auto a = trait->update(chunk, location, rawVoxel, voxel, deltaTime, invalidatedLocations);
			if (rawVoxel.type != &type()) {
				return true;
			}
			auto b = traitsUpdate(chunk, location, rawVoxel, voxel, deltaTime, invalidatedLocations, restTraits...);
			return a || b;
		}
		return traitsUpdate(chunk, location, rawVoxel, voxel, deltaTime, invalidatedLocations, restTraits...);
	}
	bool traitsUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		return false;
	}
	
	template<typename Trait> static constexpr auto traitHasSlowUpdate(
			Trait *trait,
			const VoxelChunkExtendedMutableRef *chunk,
			const InChunkVoxelLocation *location,
			Voxel *rawVoxel,
			State *voxel,
			std::unordered_set<InChunkVoxelLocation> *invalidatedLocations
	) -> decltype(std::declval<Trait>().slowUpdate(*chunk, *location, *rawVoxel, *voxel, *invalidatedLocations), true) {
		return true;
	}
	template<typename Trait> static constexpr auto traitHasSlowUpdate(...) {
		return false;
	}
	
	template<typename Trait, typename ...RestTraits> void traitsSlowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations,
			Trait *trait,
			RestTraits... restTraits
	) {
		if constexpr (traitHasSlowUpdate<Trait>(
				(Trait*) 0,
				(const VoxelChunkExtendedMutableRef*) 0,
				(const InChunkVoxelLocation*) 0,
				(Voxel*) 0,
				(State*) 0,
				(std::unordered_set<InChunkVoxelLocation>*) 0
		)) {
			trait->slowUpdate(chunk, location, rawVoxel, voxel, invalidatedLocations);
			if (rawVoxel.type != &type()) {
				return;
			}
		}
		return traitsSlowUpdate(chunk, location, rawVoxel, voxel, invalidatedLocations, restTraits...);
	}
	void traitsSlowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
	}
	
public:
	explicit VoxelType(Traits&&... traits): Traits(std::forward<Traits>(traits))... {
		setType(this);
	}
	
	void handleRegistration(const std::string &name, VoxelTypeRegistry &registry) {
		traitsHandleRegistration(name, registry, static_cast<Traits*>(this)...);
	}
	
	void invokeHandleRegistration(const std::string &name, VoxelTypeRegistry &registry) override {
		static_cast<T*>(this)->T::handleRegistration(name, registry);
	}
	
	void link(VoxelTypeRegistry &registry) {
		traitsLink(registry, static_cast<Traits*>(this)...);
	}
	
	void invokeLink(VoxelTypeRegistry &registry) override {
		static_cast<T*>(this)->T::link(registry);
	}
	
	Voxel &invokeInit(void *ptr) override {
		static_assert(sizeof(Data) <= MAX_VOXEL_DATA_SIZE);
		return *(new (ptr) Data { this });
	}
	
	Voxel &invokeInit(void *ptr, const Voxel &voxel) override {
		static_assert(sizeof(Data) <= MAX_VOXEL_DATA_SIZE);
		return *(new (ptr) Data(static_cast<const Data&>(voxel)));
	}
	
	Voxel &invokeInit(void *ptr, Voxel &&voxel) override {
		static_assert(sizeof(Data) <= MAX_VOXEL_DATA_SIZE);
		return *(new (ptr) Data(std::move(static_cast<Data&>(voxel))));
	}
	
	void invokeDestroy(Voxel &voxel) override {
		(static_cast<Data&>(voxel)).~Data();
	}
	
	const void *traitState(const State &voxel, const std::type_info &typeInfo) {
		if (typeInfo == typeid(State)) {
			return &voxel;
		}
		return traitsTraitState(voxel, typeInfo, static_cast<Traits*>(this)...);
	}
	
	const void *invokeTraitState(const Voxel &voxel, const std::type_info &typeInfo) override {
		return static_cast<T*>(this)->T::traitState(static_cast<const Data&>(voxel), typeInfo);
	}
	
	void invokeSerialize(const Voxel &voxel, VoxelSerializer &serializer) override {
		serializer.object(static_cast<const Data&>(voxel));
	}
	
	void invokeDeserialize(Voxel &voxel, VoxelDeserializer &deserializer) override {
		invokeInit(&voxel);
		deserializer.object(static_cast<Data&>(voxel));
	}
	
	void toString(const State &voxel, std::string &result) {
		traitsToString(voxel, result, static_cast<Traits*>(this)...);
	}
	
	std::string toString(const State &voxel) {
		std::string result;
		toString(voxel, result);
		return result;
	}
	
	std::string invokeToString(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::toString(static_cast<const Data&>(voxel));
	}
	
	const VoxelShaderProvider *shaderProvider(const Voxel &voxel) {
		return traitsShaderProvider(static_cast<const Data&>(voxel), static_cast<Traits*>(this)...);
	}
	
	const VoxelShaderProvider *invokeShaderProvider(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::shaderProvider(static_cast<const Data&>(voxel));
	}
	
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data
	) {
		traitsBuildVertexData(chunk, location, voxel, data, static_cast<Traits*>(this)...);
	}
	
	void invokeBuildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const Voxel &voxel,
			std::vector<VoxelVertexData> &data
	) override {
		static_cast<T*>(this)->T::buildVertexData(chunk, location, static_cast<const Data&>(voxel), data);
	}
	
	VoxelLightLevel lightLevel(const Voxel &voxel) {
		return traitsLightLevel(static_cast<const Data&>(voxel), static_cast<Traits*>(this)...);
	}
	
	VoxelLightLevel invokeLightLevel(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::lightLevel(static_cast<const Data&>(voxel));
	}
	
	void slowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		traitsSlowUpdate(chunk, location, rawVoxel, voxel, invalidatedLocations, static_cast<Traits*>(this)...);
	}
	
	void invokeSlowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) override {
		static_cast<T*>(this)->T::slowUpdate(chunk, location, voxel, static_cast<Data&>(voxel), invalidatedLocations);
	}
	
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &rawVoxel,
			State &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		return traitsUpdate(
				chunk,
				location,
				rawVoxel,
				voxel,
				deltaTime,
				invalidatedLocations,
				static_cast<Traits*>(this)...
		);
	}
	
	bool invokeUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			Voxel &voxel,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) override {
		return static_cast<T*>(this)->T::update(
				chunk,
				location,
				voxel,
				static_cast<Data&>(voxel),
				deltaTime,
				invalidatedLocations
		);
	}
	
	bool hasDensity(const Voxel &voxel) {
		return traitsHasDensity(static_cast<const Data&>(voxel), static_cast<Traits*>(this)...);
	}
	
	bool invokeHasDensity(const Voxel &voxel) override {
		return static_cast<T*>(this)->T::hasDensity(static_cast<const Data&>(voxel));
	}
	
};

class EmptyVoxelType: public VoxelType<EmptyVoxelType> {
public:
	static EmptyVoxelType INSTANCE;
	
	std::string toString(const State &voxel);
	
};

template<typename T> static inline const void *getTraitState(const Voxel &voxel) {
	return voxel.type->invokeTraitState(voxel, typeid(T));
}

template<> const void *getTraitState<Voxel>(const Voxel &voxel) {
	return &voxel;
}

class VoxelHolder {
	char m_data[MAX_VOXEL_DATA_SIZE];

public:
	VoxelHolder(): VoxelHolder(EmptyVoxelType::INSTANCE) {
	}
	
	explicit VoxelHolder(VoxelTypeInterface &type) {
		type.invokeInit(m_data);
	}
	
	VoxelHolder(const VoxelHolder& holder) {
		holder.type().invokeInit(m_data, holder.get());
	}
	
	VoxelHolder(VoxelHolder &&holder) noexcept {
		holder.type().invokeInit(m_data, std::move(holder.get()));
		holder.setType(EmptyVoxelType::INSTANCE);
	}
	
	VoxelHolder &operator=(const VoxelHolder &holder) {
		if (this != &holder) {
			auto savedLightLevel = lightLevel();
			get().type->invokeDestroy(get());
			holder.type().invokeInit(m_data, holder.get());
			setLightLevel(savedLightLevel);
		}
		return *this;
	}
	
	VoxelHolder &operator=(VoxelHolder &&holder) noexcept {
		if (this != &holder) {
			auto savedLightLevel = lightLevel();
			get().type->invokeDestroy(get());
			holder.type().invokeInit(m_data, std::move(holder.get()));
			setLightLevel(savedLightLevel);
		}
		return *this;
	}
	
	~VoxelHolder() {
		get().type->invokeDestroy(get());
	}
	
	template<typename T=Voxel> [[nodiscard]] const T &get() const {
		auto state = getTraitState<T>(*reinterpret_cast<const Voxel*>(m_data));
		assert(state != nullptr);
		return *reinterpret_cast<const T*>(state);
	}
	
	template<typename T=Voxel> T &get() {
		auto state = getTraitState<T>(*reinterpret_cast<const Voxel*>(m_data));
		assert(state != nullptr);
		return *reinterpret_cast<T*>(const_cast<void*>(state));
	}
	
	[[nodiscard]] VoxelTypeInterface &type() const {
		return *get().type;
	}
	
	void setType(VoxelTypeInterface &newType) {
		auto savedLightLevel = lightLevel();
		get().type->invokeDestroy(get());
		newType.invokeInit(m_data);
		setLightLevel(savedLightLevel);
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
	
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			std::vector<VoxelVertexData> &data
	) const {
		get().type->invokeBuildVertexData(chunk, location, get(), data);
	}
	
	void serialize(VoxelSerializer &serializer) const {
		get().type->invokeSerialize(get(), serializer);
	}
	
	void serialize(VoxelDeserializer &deserializer);
	
	void slowUpdate(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		get().type->invokeSlowUpdate(chunk, location, get(), invalidatedLocations);
	}
	
	bool update(
			const VoxelChunkExtendedMutableRef &chunk,
			const InChunkVoxelLocation &location,
			unsigned long deltaTime,
			std::unordered_set<InChunkVoxelLocation> &invalidatedLocations
	) {
		return get().type->invokeUpdate(chunk, location, get(), deltaTime, invalidatedLocations);
	}
	
	[[nodiscard]] bool hasDensity() const {
		return get().type->invokeHasDensity(get());
	}
	
};

class SimpleVoxelType: public VoxelType<SimpleVoxelType>, public VoxelTextureShaderProvider {
	std::string m_name;
	bool m_unwrap;
	VoxelLightLevel m_lightLevel;
	bool m_transparent;
	bool m_hasDensity;

public:
	SimpleVoxelType(
			std::string name,
			Asset asset,
			bool unwrap = false,
			VoxelLightLevel lightLevel = 0,
			bool transparent = false,
			bool hasDensity = true
	);

#ifndef HEADLESS
	SimpleVoxelType(
			std::string name,
			const GL::Texture &texture,
			bool unwrap = false,
			VoxelLightLevel lightLevel = 0,
			bool transparent = false,
			bool hasDensity = true
	);
#endif
	std::string toString(const State &voxel);
	const VoxelShaderProvider *shaderProvider(const State &voxel);
	void buildVertexData(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			const State &voxel,
			std::vector<VoxelVertexData> &data
	);
	VoxelLightLevel lightLevel(const State &voxel);
	[[nodiscard]] int priority() const override;
	bool hasDensity(const State &voxel);
	
};
