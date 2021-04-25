#pragma once

enum class ServerMessageType: uint16_t {
	SET_POSITION
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
}
