#pragma once

#include <string>

class Asset {
	enum class State {
		PENDING,
		STATIC,
		DYNAMIC
	};
	
	std::string m_fileName;
	std::string m_filePath;
	const char *m_data;
	size_t m_dataSize;
	State m_state = State::PENDING;
	
	void load();
	
public:
	Asset(
			std::string fileName,
			std::string filePath
	): m_fileName(std::move(fileName)), m_filePath(std::move(filePath)), m_data(nullptr), m_dataSize(0) {
	}

	Asset(
			std::string fileName,
			std::string filePath,
			const char *data,
			size_t dataSize
	): m_fileName(std::move(fileName)), m_filePath(std::move(filePath)), m_data(data), m_dataSize(dataSize) {
	}

	Asset(
			Asset &&asset
	) noexcept: m_fileName(std::move(asset.m_fileName)), m_filePath(std::move(asset.m_filePath)),
		m_data(asset.m_data), m_dataSize(asset.m_dataSize), m_state(asset.m_state)
	{
		asset.m_state = State::STATIC;
		asset.m_data = nullptr;
		asset.m_dataSize = 0;
	}
	
	Asset(
			const Asset &asset
	): m_fileName(asset.m_fileName), m_filePath(asset.m_filePath),
		m_data(asset.m_data), m_dataSize(asset.m_dataSize), m_state(asset.m_state)
	{
		if (m_state == State::DYNAMIC) {
			m_state = State::PENDING;
			m_data = nullptr;
			m_dataSize = 0;
		}
	}
	
	~Asset();
	
	[[nodiscard]] const std::string &fileName() const {
		return m_fileName;
	}

	const char *data() {
		if (m_state == State::PENDING) {
			load();
		}
		return m_data;
	}

	size_t dataSize() {
		if (m_state == State::PENDING) {
			load();
		}
		return m_dataSize;
	}
	
};

class AssetLoader {
	std::string m_prefix;
	
public:
	explicit AssetLoader(std::string prefix);
	Asset load(std::string fileName);
	
};
