#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "client/OpenGL.h"
#include "client/ShaderProgram.h"

class Asset;

class Model {
	const CommonShaderProgram &m_program;
	GL::Buffer m_buffer;
	unsigned int m_vertexCount;
	const GL::Texture *m_texture;
	glm::vec3 m_dimensions;

public:
	Model(Asset fileName, const CommonShaderProgram &program, const GL::Texture *texture = nullptr);
	[[nodiscard]] const glm::vec3 &dimensions() const {
		return m_dimensions;
	}
	void render(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const;

};
