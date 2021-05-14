#include <string>
#include <tuple>
#include <unordered_map>
#include <fstream>
#include <easylogging++.h>
#include "Asset.h"

extern "C" const std::unordered_map<std::string, std::tuple<const void*, size_t>> packed_assets;

Asset::~Asset() {
	if (m_state == State::DYNAMIC && m_data && m_dataSize) {
		delete[] const_cast<char*>(m_data);
	}
}

void Asset::load() {
	if (m_state != State::PENDING) return;
	std::ifstream file(m_filePath, std::ios::binary | std::ios::ate);
	if (file.good()) {
		LOG(DEBUG) << "Using an override for asset \"" << m_fileName << "\"";
		m_state = State::DYNAMIC;
		m_dataSize = file.tellg();
		m_data = new char[m_dataSize];
		file.seekg(0);
		file.read(const_cast<char*>(m_data), m_dataSize);
	} else {
		m_state = State::STATIC;
		if (m_data == nullptr || m_dataSize == 0) {
			LOG(WARNING) << "Unable to load asset \"" << m_fileName << "\"";
		}
	}
}

AssetLoader::AssetLoader(std::string prefix): m_prefix(std::move(prefix)) {
	LOG(INFO) << "Assets prefix set to " << m_prefix;
}

Asset AssetLoader::load(std::string fileName) {
	auto path = m_prefix + fileName;
	auto it = packed_assets.find(fileName);
	if (it != packed_assets.end()) {
		auto [data, dataSize] = it->second;
		return Asset(std::move(fileName), std::move(path), (const char*) data, dataSize);
	}
	return Asset(std::move(fileName), std::move(path));
}
