#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "../OpenGL.h"
#include "../ShaderProgram.h"

class Model {
	const CommonShaderProgram &m_program;
	GLBuffer m_buffer;
	unsigned int m_vertexCount;
	const GLTexture *m_texture;
	glm::vec3 m_dimensions;

public:
	Model(const std::string &fileName, const CommonShaderProgram &program, const GLTexture *texture = nullptr);
	[[nodiscard]] const glm::vec3 &dimensions() const {
		return m_dimensions;
	}
	void render(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const;

};
