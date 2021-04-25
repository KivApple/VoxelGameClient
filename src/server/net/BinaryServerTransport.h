#pragma once

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include "ServerTransport.h"

class BinaryServerTransport: public ServerTransport {
protected:
	template<typename T> void deserialize(const std::string &data, T& message) {
		bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::string>>(
				{data.begin(), data.size()},
				message
		);
	}
	
};
