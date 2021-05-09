#pragma once

#include <glm/mat4x4.hpp>
#include "OpenGL.h"

class CommonShaderProgram: public GL::ShaderProgram {
	int m_modelLocation;
	int m_viewLocation;
	int m_projectionLocation;
	int m_texImageLocation;
	int m_colorUniformLocation;
	
	int m_positionLocation;
	int m_lightLevelLocation;
	int m_texCoordLocation;
	int m_colorLocation;

public:
	CommonShaderProgram(std::string name, const std::initializer_list<GL::Shader> &shaders);
	
	void setModel(const glm::mat4 &model) const;
	void setView(const glm::mat4 &view) const;
	void setProjection(const glm::mat4 &projection) const;
	
	void setTexImage(const GL::Texture &texImage) const;
	void setColorUniform(const glm::vec4 &color) const;
	
	void setPositions(const GL::BufferPointer &pointer) const;
	void setLightLevels(const GL::BufferPointer &pointer) const;
	void setTexCoords(const GL::BufferPointer &pointer) const;
	void setColors(const GL::BufferPointer &pointer) const;
	
};

struct CommonShaderPrograms {
	const CommonShaderProgram texture;
	const CommonShaderProgram color;
	const CommonShaderProgram font;
	
	CommonShaderPrograms();
};
