#include <GL/glew.h>
#include <easylogging++.h>
#include "OpenGL.h"
#include "Asset.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb/stb_image.h>

/* BufferPointer */

void GL::BufferPointer::bind(int attribLocation, int size, bool normalized) const {
	if (attribLocation < 0) return;
	m_buffer.bind();
	glVertexAttribPointer(attribLocation, size, m_type, normalized, m_stride, (const void*) m_offset);
	glEnableVertexAttribArray(attribLocation);
}

/* Buffer */

GL::Buffer::Buffer(unsigned int target): m_id(0), m_target(target) {
	glGenBuffers(1, &m_id);
}

GL::Buffer::Buffer(Buffer &&buffer) noexcept: m_id(buffer.m_id), m_target(buffer.m_target) {
	buffer.m_id = 0;
}

GL::Buffer &GL::Buffer::operator=(Buffer &&buffer) noexcept {
	m_id = buffer.m_id;
	m_target = buffer.m_target;
	buffer.m_id = 0;
	return *this;
}

GL::Buffer::~Buffer() {
	if (m_id) {
		glDeleteBuffers(1, &m_id);
	}
}

void GL::Buffer::bind() const {
	glBindBuffer(m_target, m_id);
}

void GL::Buffer::setData(const void *data, size_t dataSize, unsigned int usage) const {
	bind();
	glBufferData(m_target, dataSize, data, usage);
}

/* Texture */

GL::Texture::Texture(): m_id(0), m_width(0), m_height(0) {
	glGenTextures(1, &m_id);
}

GL::Texture::Texture(Asset asset): Texture() {
	int channels;
	auto *data = stbi_load_from_memory(
			(const unsigned char*) asset.data(),
			asset.dataSize(),
			(int*) &m_width,
			(int*) &m_height,
			&channels,
			4
	);
	if (data) {
		bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	} else {
		LOG(ERROR) << "Failed to load texture asset " << asset.fileName() << ": " << stbi_failure_reason();
	}
}

GL::Texture::Texture(
		Texture &&texture
) noexcept: m_id(texture.m_id), m_width(texture.m_width), m_height(texture.m_height) {
	texture.m_id = 0;
}

GL::Texture::Texture(int width, int height, bool filter): Texture() {
	m_width = width;
	m_height = height;
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

GL::Texture &GL::Texture::operator=(Texture &&texture) noexcept {
	m_id = texture.m_id;
	m_width = texture.m_width;
	m_height = texture.m_height;
	texture.m_id = 0;
	return *this;
}

GL::Texture::~Texture() {
	if (m_id) {
		glDeleteTextures(1, &m_id);
	}
}

void GL::Texture::bind() const {
	glBindTexture(GL_TEXTURE_2D, m_id);
}

/* Framebuffer */

GL::Framebuffer::Framebuffer(
		unsigned int width,
		unsigned int height,
		bool depth
): m_id(0), m_renderBufferId(0), m_width(width), m_height(height) {
	glGenFramebuffers(1, &m_id);
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	if (depth) {
		glGenRenderbuffers(1, &m_renderBufferId);
		glBindRenderbuffer(GL_RENDERBUFFER, m_renderBufferId);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_width, m_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderBufferId);
	}
}

GL::Framebuffer::Framebuffer(Texture &texture, bool depth): Framebuffer(texture.width(), texture.height(), depth) {
	setTexture(texture);
}

GL::Framebuffer::Framebuffer(
		Framebuffer &&framebuffer
) noexcept: m_id(framebuffer.m_id), m_renderBufferId(framebuffer.m_renderBufferId),
	m_width(framebuffer.m_width), m_height(framebuffer.m_height)
{
	framebuffer.m_id = 0;
	framebuffer.m_renderBufferId = 0;
}

GL::Framebuffer &GL::Framebuffer::operator=(Framebuffer &&framebuffer) noexcept {
	m_id = framebuffer.m_id;
	m_renderBufferId = framebuffer.m_renderBufferId;
	m_width = framebuffer.m_width;
	m_height = framebuffer.m_height;
	framebuffer.m_id = 0;
	framebuffer.m_renderBufferId = 0;
	return *this;
}

GL::Framebuffer::~Framebuffer() {
	if (m_renderBufferId > 0) {
		glDeleteRenderbuffers(1, &m_renderBufferId);
	}
	if (m_id > 0) {
		glDeleteFramebuffers(1, &m_id);
	}
}

void GL::Framebuffer::setTexture(Texture &texture) const {
	assert(m_width == texture.width());
	assert(m_height == texture.height());
	
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.id(), 0);
	/* GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers); */
}

void GL::Framebuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	glViewport(0, 0, m_width, m_height);
}

/* Shader */

GL::Shader::Shader(unsigned int type, Asset asset): Shader(
		type,
		asset.fileName(),
		std::string(asset.data(), asset.dataSize())
) {
}

GL::Shader::Shader(
		unsigned int type,
		std::string name,
		const std::string &source
): m_id(glCreateShader(type)), m_name(std::move(name)) {
	const char *sources[] = { source.data() };
	const int sourceLengths[] = { (int) source.size() };
	glShaderSource(m_id, 1, sources, sourceLengths);
	glCompileShader(m_id);
	int success, infoLogLength;
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
	glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength) {
		char *infoLog = new char[infoLogLength];
		glGetShaderInfoLog(m_id, infoLogLength, nullptr, infoLog);
		if (infoLogLength > 1) {
			if (success) {
				LOG(WARNING) << "Shader " << m_name << " compiled with warnings:";
				LOG(WARNING) << infoLog;
			} else {
				LOG(ERROR) << "Failed to compile shader " << m_name << ":";
				LOG(ERROR) << infoLog;
			}
		}
		delete[] infoLog;
	} else if (!success) {
		LOG(ERROR) << "Failed to compile shader " << m_name;
	}
}

GL::Shader::Shader(Shader &&shader) noexcept: m_id(shader.m_id), m_name(std::move(shader.m_name)) {
	shader.m_id = 0;
}

GL::Shader &GL::Shader::operator=(Shader &&shader) noexcept {
	m_id = shader.m_id;
	m_name = std::move(shader.m_name);
	shader.m_id = 0;
	return *this;
}

GL::Shader::~Shader() {
	if (!m_id) return;
	glDeleteShader(m_id);
}

/* ShaderProgram */

GL::ShaderProgram::ShaderProgram(
		std::string name,
		const std::initializer_list<Shader> &shaders
): m_id(glCreateProgram()), m_name(std::move(name)) {
	for (auto &shader : shaders) {
		glAttachShader(m_id, shader.id());
	}
	glLinkProgram(m_id);
	int success, infoLogLength;
	glGetProgramiv(m_id, GL_LINK_STATUS, &success);
	glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength) {
		char *infoLog = new char[infoLogLength];
		glGetProgramInfoLog(m_id, infoLogLength, nullptr, infoLog);
		if (infoLogLength > 1) {
			if (success) {
				LOG(WARNING) << "Shader program " << m_name << " linked with warnings:";
				LOG(WARNING) << infoLog;
			} else {
				LOG(ERROR) << "Failed to link shader program " << m_name << ":";
				LOG(ERROR) << infoLog;
			}
		}
		delete[] infoLog;
	} else if (!success) {
		LOG(ERROR) << "Failed to link shader program " << m_name;
	}
}

GL::ShaderProgram::ShaderProgram(ShaderProgram &&program) noexcept: m_id(program.m_id), m_name(std::move(program.m_name)) {
	program.m_id = 0;
}

GL::ShaderProgram &GL::ShaderProgram::operator=(ShaderProgram &&program) noexcept {
	m_id = program.m_id;
	m_name = std::move(program.m_name);
	program.m_id = 0;
	return *this;
}

GL::ShaderProgram::~ShaderProgram() {
	if (m_id) {
		glDeleteProgram(m_id);
	}
}

int GL::ShaderProgram::attribLocation(const char *name, bool required) const {
	int result = glGetAttribLocation(m_id, name);
	if (required && result < 0) {
		LOG(WARNING) << "Failed to find attribute " << name << " location in shader program " << m_name;
	}
	return result;
}

int GL::ShaderProgram::uniformLocation(const char *name, bool required) const {
	int result = glGetUniformLocation(m_id, name);
	if (required && result < 0) {
		LOG(WARNING) << "Failed to find uniform " << name <<" location in shader program " << m_name;
	}
	return result;
}

void GL::ShaderProgram::use() const {
	glUseProgram(m_id);
}
