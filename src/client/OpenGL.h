#pragma once

#include <string>
#include <initializer_list>

class GLBuffer;

class GLBufferPointer {
	const GLBuffer &m_buffer;
	unsigned int m_type;
	size_t m_offset;
	size_t m_stride;
	
public:
	GLBufferPointer(
			const GLBuffer &buffer,
			unsigned int type,
			size_t offset,
			size_t stride
	): m_buffer(buffer), m_type(type), m_offset(offset), m_stride(stride) {
	}
	void bind(int attribLocation, int size, bool normalized = false) const;
	
};

class GLBuffer {
	unsigned int m_id;
	unsigned int m_target;
	
public:
	explicit GLBuffer(unsigned int target);
	GLBuffer(GLBuffer &&buffer) noexcept;
	GLBuffer &operator=(GLBuffer &&buffer) noexcept;
	~GLBuffer();
	[[nodiscard]] unsigned int id() const {
		return m_id;
	}
	void bind() const;
	void setData(const void *data, size_t dataSize, unsigned int usage) const;
	[[nodiscard]] GLBufferPointer pointer(unsigned int type, size_t offset, size_t stride) const {
		return GLBufferPointer(*this, type, offset, stride);
	}
	
};

class GLTexture {
	unsigned int m_id;
	unsigned int m_width;
	unsigned int m_height;

public:
	GLTexture();
	explicit GLTexture(const std::string &fileName);
	GLTexture(int width, int height, bool filter);
	GLTexture(GLTexture &&texture) noexcept;
	GLTexture &operator=(GLTexture &&texture) noexcept;
	~GLTexture();
	[[nodiscard]] unsigned int id() const {
		return m_id;
	}
	[[nodiscard]] unsigned int width() const {
		return m_width;
	}
	[[nodiscard]] unsigned int height() const {
		return m_height;
	}
	void bind() const;

};

class Framebuffer {
	unsigned int m_id;
	unsigned int m_renderBufferId;
	unsigned int m_width;
	unsigned int m_height;

public:
	Framebuffer(unsigned int width, unsigned int height, bool depth);
	Framebuffer(GLTexture &texture, bool depth);
	Framebuffer(Framebuffer &&framebuffer) noexcept;
	Framebuffer &operator=(Framebuffer &&framebuffer) noexcept;
	~Framebuffer();
	void setTexture(GLTexture &texture) const;
	void bind() const;

};

class Shader {
	unsigned int m_id;
	std::string m_name;
	
public:
	Shader(unsigned int type, const std::string &fileName);
	Shader(unsigned int type, std::string name, const std::string &source);
	Shader(Shader &&shader) noexcept;
	Shader &operator=(Shader &&shader) noexcept;
	~Shader();
	[[nodiscard]] unsigned int id() const {
		return m_id;
	}
	[[nodiscard]] const std::string &name() const {
		return m_name;
	}
	
};

class ShaderProgram {
	unsigned int m_id;
	std::string m_name;
	
public:
	ShaderProgram(std::string name, const std::initializer_list<Shader> &shaders);
	ShaderProgram(ShaderProgram &&program) noexcept;
	ShaderProgram &operator=(ShaderProgram &&program) noexcept;
	~ShaderProgram();
	[[nodiscard]] unsigned int id() const {
		return m_id;
	}
	[[nodiscard]] const std::string &name() const {
		return m_name;
	}
	int attribLocation(const char *name, bool required = false) const;
	int uniformLocation(const char *name, bool required = false) const;
	void use() const;
	
};
