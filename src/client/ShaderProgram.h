#pragma once

#include <glm/mat4x4.hpp>
#include "OpenGL.h"

class CommonShaderProgram: public ShaderProgram {
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
	CommonShaderProgram(std::string name, const std::initializer_list<Shader> &shaders);
	
	void setModel(const glm::mat4 &model) const;
	void setView(const glm::mat4 &view) const;
	void setProjection(const glm::mat4 &projection) const;
	
	void setTexImage(const GLTexture &texImage) const;
	void setColorUniform(const glm::vec4 &color) const;
	
	void setPositions(const GLBufferPointer &pointer) const;
	void setLightLevels(const GLBufferPointer &pointer) const;
	void setTexCoords(const GLBufferPointer &pointer) const;
	void setColors(const GLBufferPointer &pointer) const;
	
};

struct CommonShaderPrograms {
	const CommonShaderProgram texture;
	const CommonShaderProgram color;
	const CommonShaderProgram font;
	
	CommonShaderPrograms();
};
