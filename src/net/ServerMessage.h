#pragma once

#include <bitsery/ext/std_map.h>
#include "world/VoxelLocation.h"

class VoxelTypeSerializationContext;

enum class ServerMessageType: uint16_t {
	SET_POSITION,
	SET_VOXEL_TYPES,
	SET_CHUNK,
	DISCARD_CHUNKS,
	SET_INVENTORY
};

template<typename T> struct ServerMessage {
	ServerMessageType type;
	T data;
	
	ServerMessage() {
	}
	
	constexpr explicit ServerMessage(const T &data): type(T::TYPE), data(data) {
	}
	
	template<typename S> void serialize(S &s) {
		s.value2b(type);
		s.object(data);
	}
};

namespace ServerMessageData {
	struct Empty {
		template<typename S> void serialize(S &s) {
		}
	};
	
	struct SetPosition {
		static const ServerMessageType TYPE = ServerMessageType::SET_POSITION;
		
		float x, y, z;
		
		template<typename S> void serialize(S &s) {
			s.value4b(x);
			s.value4b(y);
			s.value4b(z);
		}
	};
	
	struct SetVoxelTypes {
		static const ServerMessageType TYPE = ServerMessageType::SET_VOXEL_TYPES;
		
		VoxelTypeSerializationContext &voxelSerializationContext;
		
		template<typename S> void serialize(S &s) {
			s.object(voxelSerializationContext);
		}
	};
	
	struct SetChunk {
		static const ServerMessageType TYPE = ServerMessageType::SET_CHUNK;
		
		VoxelChunkLocation location;
		
		template<typename S> void serialize(S &s) {
			s.object(location);
		}
	};
	
	struct DiscardChunks {
		static const ServerMessageType TYPE = ServerMessageType::DISCARD_CHUNKS;
		
		std::vector<VoxelChunkLocation> locations;
		
		template<typename S> void serialize(S &s) {
			s.container(locations, 65535);
		}
	};
	
	struct SetInventory {
		static const ServerMessageType TYPE = ServerMessageType::SET_INVENTORY;
		
		std::unordered_map<uint8_t, VoxelHolder> voxels;
		uint8_t active;
		
		template<typename S> void serialize(S &s) {
			s.ext(voxels, bitsery::ext::StdMap(UINT8_MAX + 1), [](S &s, uint8_t &index, VoxelHolder &voxel) {
				s.value1b(index);
				s.object(voxel);
			});
			s.value1b(active);
		}
	};
	
}
