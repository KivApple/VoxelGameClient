#pragma once

#include <vector>
#include <unordered_map>
#include "../OpenGL.h"
#include <glm/vec4.hpp>

class TextRenderer {
public:
	virtual ~TextRenderer() = default;
	virtual void setText(const std::string &text, const glm::vec4 &color) = 0;
	virtual void render(float x, float y, float size) const = 0;
	
};

class BitmapFont {
	GLTexture m_texture;
	std::unordered_map<int, int> m_charMap;
	int m_defaultChar = 0;
	int m_sizeX = 16;
	int m_sizeY = 32;
	
public:
	explicit BitmapFont(const std::string &fileName);
	[[nodiscard]] const GLTexture &texture() const;
	[[nodiscard]] int sizeX() const {
		return m_sizeX;
	}
	[[nodiscard]] int sizeY() const {
		return m_sizeY;
	}
	void buildVertexData(std::vector<float> &data, float x, float y, const glm::vec4 &color, int chr) const;

};

class BitmapFontRenderer: public TextRenderer {
	const BitmapFont &m_font;
	GLBuffer m_buffer;
	std::vector<float> m_bufferData;
	
public:
	explicit BitmapFontRenderer(const BitmapFont &font);
	void setText(const std::string &text, const glm::vec4 &color) override;
	void render(float x, float y, float size) const override;
	
};
