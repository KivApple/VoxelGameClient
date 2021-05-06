#pragma once

enum class ClientMessageType: uint16_t {
	UPDATE_POSITION,
	UPDATE_ACTIVE_INVENTORY_ITEM,
	DIG_VOXEL,
	PLACE_VOXEL
};

template<typename T> struct ClientMessage {
	ClientMessageType type;
	T data;
	
	ClientMessage() {
	}
	
	constexpr explicit ClientMessage(const T &data): type(T::TYPE), data(data) {
	}
	
	template<typename S> void serialize(S &s) {
		s.value2b(type);
		s.object(data);
	}
};

namespace ClientMessageData {
	struct Empty {
		template<typename S> void serialize(S &s) {
		}
	};
	
	struct UpdatePosition {
		static const ClientMessageType TYPE = ClientMessageType::UPDATE_POSITION;
		
		float x, y, z;
		float yaw, pitch;
		uint8_t viewRadius;
		
		template<typename S> void serialize(S &s) {
			s.value4b(x);
			s.value4b(y);
			s.value4b(z);
			s.value4b(yaw);
			s.value4b(pitch);
			s.value1b(viewRadius);
		}
	};
	
	struct UpdateActiveInventoryItem {
		static const ClientMessageType TYPE = ClientMessageType::UPDATE_ACTIVE_INVENTORY_ITEM;
		
		uint8_t active;
		
		template<typename S> void serialize(S &s) {
			s.value1b(active);
		}
	};
	
	struct DigVoxel {
		static const ClientMessageType TYPE = ClientMessageType::DIG_VOXEL;
		
		VoxelLocation location;
		
		template<typename S> void serialize(S &s) {
			s.object(location);
		}
	};
	
	struct PlaceVoxel {
		static const ClientMessageType TYPE = ClientMessageType::PLACE_VOXEL;
		
		VoxelLocation location;
		
		template<typename S> void serialize(S &s) {
			s.object(location);
		}
	};
}
