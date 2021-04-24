#include <fstream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "TextRenderer.h"
#include "GameEngine.h"

BitmapFont::BitmapFont(const std::string &fileName): m_texture(fileName) {
	std::ifstream file(fileName + ".txt");
	file >> m_sizeX >> m_sizeY >> m_defaultChar;
	int index = 0;
	while (!file.eof()) {
		int charCode;
		file >> charCode;
		if (!file.eof()) {
			m_charMap[charCode] = index++;
		}
	}
}

const GLTexture &BitmapFont::texture() const {
	return m_texture;
}

void BitmapFont::buildVertexData(std::vector<float> &data, float x, float y, const glm::vec4 &color, int chr) const {
	auto it = m_charMap.find(chr);
	if (it == m_charMap.end()) {
		it = m_charMap.find(m_defaultChar);
		if (it == m_charMap.end()) {
			it = m_charMap.begin();
			if (it == m_charMap.end()) {
				return;
			}
		}
	}
	int index = it->second;
	int rowSize = (int) m_texture.width() / m_sizeX;
	int row = index / rowSize;
	int col = index % rowSize;
	float x1 = (float) (col * m_sizeX) / (float) m_texture.width();
	float y1 = (float) (row * m_sizeY) / (float) m_texture.height();
	float x2 = (float) ((col + 1) * m_sizeX - 1) / (float) m_texture.width();
	float y2 = (float) ((row + 1) * m_sizeY - 1) / (float) m_texture.height();
	
	data.insert(data.end(), {
			x + 1.0f, y + 1.0f, 0.0f, x2, y1, color.r, color.g, color.b, color.a,
			x + 1.0f, y + 0.0f, 0.0f, x2, y2, color.r, color.g, color.b, color.a,
			x + 0.0f, y + 0.0f, 0.0f, x1, y2, color.r, color.g, color.b, color.a,
			x + 0.0f, y + 1.0f, 0.0f, x1, y1, color.r, color.g, color.b, color.a,
			x + 1.0f, y + 1.0f, 0.0f, x2, y1, color.r, color.g, color.b, color.a,
			x + 0.0f, y + 0.0f, 0.0f, x1, y2, color.r, color.g, color.b, color.a
	});
}

BitmapFontRenderer::BitmapFontRenderer(const BitmapFont &font): m_font(font), m_buffer(GL_ARRAY_BUFFER) {
}

void BitmapFontRenderer::setText(const std::string &text, const glm::vec4 &color) {
	m_bufferData.clear();
	float x = 0.0f;
	float y = 0.0f;
	for (char chr : text) {
		if (chr != '\n') {
			m_font.buildVertexData(m_bufferData, x, y, color, chr);
			x += 1.0f;
		} else {
			x = 0.0f;
			y -= 1.0f;
		}
	}
	m_buffer.setData(m_bufferData.data(), m_bufferData.size() * sizeof(float), GL_DYNAMIC_DRAW);
}

void BitmapFontRenderer::render(float x, float y, float size) const {
	auto &program = GameEngine::instance().commonShaderPrograms().font;
	
	program.use();
	
	float fontRatio = (float) m_font.sizeY() / (float) m_font.sizeX();
	float ratio = GameEngine::instance().viewportHeight() > 0 ?
				  (float) GameEngine::instance().viewportWidth() / (float) GameEngine::instance().viewportHeight() :
			1.0f;
	
	auto transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(x, y - size, 0.0f));
	transform = glm::scale(transform, glm::vec3(size / ratio / fontRatio, size, 1.0f));
	program.setModel(transform);
	program.setView(glm::mat4(1.0f));
	program.setProjection(glm::mat4(1.0f));
	program.setTexImage(m_font.texture());
	
	program.setPositions(m_buffer.pointer(GL_FLOAT, 0, 9 * sizeof(float)));
	program.setTexCoords(m_buffer.pointer(GL_FLOAT, 3 * sizeof(float), 9 * sizeof(float)));
	program.setColors(m_buffer.pointer(GL_FLOAT, 5 * sizeof(float), 9 * sizeof(float)));
	
	glDrawArrays(GL_TRIANGLES, 0, m_bufferData.size() / 9);
}
