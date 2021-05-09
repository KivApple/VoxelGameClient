#include <fstream>
#include <vector>
#include <sstream>
#include <easylogging++.h>
#include <GL/glew.h>
#include "Model.h"

Model::Model(
		const std::string &fileName,
		const CommonShaderProgram &program,
		const GL::Texture *texture
): m_program(program), m_buffer(GL_ARRAY_BUFFER), m_texture(texture) {
	std::vector<glm::vec3> vertexes;
	std::vector<glm::vec2> texCoords;
	std::vector<float> vertexData;
	glm::vec3 minCoords(INFINITY), maxCoords(-INFINITY);
	
	std::ifstream file(fileName);
	if (file.good()) {
		std::vector<std::string> tokens;
		std::string line, token;
		while (!file.eof()) {
			tokens.clear();
			std::getline(file, line);
			line = line.substr(0, line.find('#'));
			std::istringstream ss(line);
			while (ss >> token) {
				tokens.push_back(token);
			}
			if (tokens.empty()) continue;
			if (tokens[0] == "v") {
				vertexes.emplace_back(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
			} else if (tokens[0] == "vt") {
				texCoords.emplace_back(std::stof(tokens[1]), 1.0f - std::stof(tokens[2]));
			} else if (tokens[0] == "f") {
				for (int i = 0; i < 3; i++) {
					auto &part = tokens[i + 1];
					auto vStopPos = part.find('/');
					int vi = std::stoi(part.substr(0, vStopPos)) - 1;
					auto &v = vertexes[vi];
					auto tStopPos = part.find('/', vStopPos + 1);
					int ti = std::stoi(part.substr(vStopPos + 1, tStopPos - vStopPos - 1)) - 1;
					auto &t = texCoords[ti];
	
					vertexData.insert(vertexData.end(), {v.x, v.y, v.z, t.x, t.y, 1.0f});
					
					minCoords.x = std::min(minCoords.x, v.x);
					minCoords.y = std::min(minCoords.y, v.y);
					minCoords.z = std::min(minCoords.z, v.z);
					
					maxCoords.x = std::max(maxCoords.x, v.x);
					maxCoords.y = std::max(maxCoords.y, v.y);
					maxCoords.z = std::max(maxCoords.z, v.z);
				}
			}
		}
	} else {
		LOG(ERROR) << "Failed to load model " << fileName;
	}
	
	m_buffer.setData(vertexData.data(), vertexData.size() * sizeof(float), GL_STATIC_DRAW);
	m_vertexCount = vertexData.size() / 6;
	m_dimensions = maxCoords - minCoords;
}

void Model::render(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const {
	m_program.use();
	
	m_program.setModel(model);
	m_program.setView(view);
	m_program.setProjection(projection);
	if (m_texture != nullptr) {
		m_program.setTexImage(*m_texture);
	}

	m_program.setPositions(m_buffer.pointer(
			GL_FLOAT,
			0,
			6 * sizeof(float)
	));
	m_program.setLightLevels(m_buffer.pointer(
			GL_FLOAT,
			5 * sizeof(float),
			6 * sizeof(float)
	));
	m_program.setTexCoords(m_buffer.pointer(
			GL_FLOAT,
			3 * sizeof(float),
			6 * sizeof(float)
	));
	
	glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}
