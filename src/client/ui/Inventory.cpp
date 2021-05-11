#include <easylogging++.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Inventory.h"
#include "../GameEngine.h"

const float InventoryItem::FILL_BUFFER_DATA[] = {
		-1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f, /**/ 0.0f, 1.0f, /**/ 1.0f,
		1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f, /**/ 1.0f, 1.0f, /**/ 1.0f,
		-1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f, /**/ 0.0f, 0.0f, /**/ 1.0f,
		
		-1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f, /**/ 0.0f, 0.0f, /**/ 1.0f,
		1.0f,  1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f, /**/ 1.0f, 1.0f, /**/ 1.0f,
		1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f, /**/ 1.0f, 0.0f, /**/ 1.0f
};

const float InventoryItem::OUTLINE_BUFFER_DATA[] = {
		-1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, 1.0f
};

InventoryItem::InventoryItem(): m_texture(256, 256, false) {
}

void InventoryItem::setActive(bool active) {
	m_active = active;
}

void InventoryItem::render(const glm::mat4 &transform) {
	auto &fillBuffer = staticBufferInstance(FILL_BUFFER_DATA, sizeof(FILL_BUFFER_DATA));
	auto &outlineBuffer = staticBufferInstance(OUTLINE_BUFFER_DATA, sizeof(OUTLINE_BUFFER_DATA));
	
	auto &colorProgram = GameEngine::instance().commonShaderPrograms().color;
	colorProgram.use();
	
	colorProgram.setModel(transform);
	colorProgram.setView(glm::mat4(1.0f));
	colorProgram.setProjection(glm::mat4(1.0f));
	
	colorProgram.setColorUniform(
			m_active ?
			glm::vec4(0.5f, 0.5f, 0.5f, 0.5f) :
			glm::vec4(0.5f, 0.5f, 0.5f, 0.25f)
	);
	
	colorProgram.setPositions(fillBuffer.pointer(GL_FLOAT, 0, 10 * sizeof(float)));
	colorProgram.setColors(fillBuffer.pointer(GL_FLOAT, 3 * sizeof(float), 10 * sizeof(float)));
	
	glDrawArrays(GL_TRIANGLES, 0, sizeof(FILL_BUFFER_DATA) / sizeof(float) / 10);
	
	if (m_voxel.shaderProvider() != nullptr) {
		auto &textureProgram = GameEngine::instance().commonShaderPrograms().texture;
		textureProgram.use();
		
		textureProgram.setModel(transform);
		textureProgram.setView(glm::mat4(1.0f));
		textureProgram.setProjection(glm::mat4(1.0f));
		
		textureProgram.setTexImage(m_texture);
		
		textureProgram.setPositions(fillBuffer.pointer(
				GL_FLOAT,
				0,
				10 * sizeof(float)
		));
		textureProgram.setTexCoords(fillBuffer.pointer(
				GL_FLOAT,
				7 * sizeof(float),
				10 * sizeof(float)
		));
		textureProgram.setLightLevels(fillBuffer.pointer(
				GL_FLOAT,
				9 * sizeof(float),
				10 * sizeof(float)
		));
		
		glDrawArrays(GL_TRIANGLES, 0, sizeof(FILL_BUFFER_DATA) / sizeof(float) / 10);
	}
	
	colorProgram.use();
	
	colorProgram.setModel(transform);
	colorProgram.setView(glm::mat4(1.0f));
	colorProgram.setProjection(glm::mat4(1.0f));
	
	colorProgram.setColorUniform(
			m_active ?
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) :
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	
	colorProgram.setPositions(outlineBuffer.pointer(GL_FLOAT, 0, 7 * sizeof(float)));
	colorProgram.setColors(outlineBuffer.pointer(GL_FLOAT, 3 * sizeof(float), 7 * sizeof(float)));
	
	glDrawArrays(GL_LINE_STRIP, 0, sizeof(OUTLINE_BUFFER_DATA) / sizeof(float) / 7);
}

void InventoryItem::updateTexture() {
	auto shaderProvider = m_voxel.shaderProvider();
	if (shaderProvider == nullptr) return;
	
	std::vector<VoxelVertexData> vertexData;
	{
		VoxelWorld dummyWorld;
		auto chunk = dummyWorld.mutableChunk({0, 0, 0}, VoxelWorld::MissingChunkPolicy::CREATE);
		InChunkVoxelLocation location(VOXEL_CHUNK_SIZE / 2, VOXEL_CHUNK_SIZE / 2, VOXEL_CHUNK_SIZE / 2);
		auto &voxel = chunk.at(location);
		voxel = m_voxel;
		m_voxel.buildVertexData(chunk, location, vertexData);
	}
	std::vector<float> bufferData;
	for (auto &vertex : vertexData) {
		bufferData.insert(bufferData.end(), {
				vertex.x, vertex.y, vertex.z,
				vertex.u, 1.0f - vertex.v,
				1.0f // Light level
		});
	}
	auto &buffer = sharedBufferInstance();
	buffer.setData(bufferData.data(), bufferData.size() * sizeof(float), GL_DYNAMIC_DRAW);
	
	auto &framebuffer = sharedFramebufferInstance(256, 256, true);
	framebuffer.setTexture(m_texture);
	framebuffer.bind();
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	auto &program = shaderProvider->get();
	program.use();
	
	auto transform = glm::mat4(1.0f);
	transform = glm::rotate(
			transform,
			glm::radians(45.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
	);
	transform = glm::rotate(
			transform,
			glm::radians(45.0f),
			glm::vec3(1.0f, 0.0f, 1.0f)
	);
	program.setModel(transform);
	program.setView(glm::mat4(1.0f));
	program.setProjection(glm::ortho(
			-1.0f, 1.0f,
			-1.0f, 1.0f,
			-1.0f, 1.0f
	));
	program.setPositions(buffer.pointer(
			GL_FLOAT,
			0,
			sizeof(float) * 6
	));
	program.setTexCoords(buffer.pointer(
			GL_FLOAT,
			sizeof(float) * 3,
			sizeof(float) * 6
	));
	program.setLightLevels(buffer.pointer(
			GL_FLOAT,
			sizeof(float) * 5,
			sizeof(float) * 6
	));
	shaderProvider->setup(program);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size());
}

void InventoryItem::setVoxel(const VoxelHolder &voxel) {
	m_voxel = voxel;
	m_voxel.setLightLevel(MAX_VOXEL_LIGHT_LEVEL);
	updateTexture();
}

void InventoryItem::setVoxel(VoxelHolder &&voxel) {
	m_voxel = std::move(voxel);
	m_voxel.setLightLevel(MAX_VOXEL_LIGHT_LEVEL);
	updateTexture();
}

Inventory::Inventory(int cols, int rows): m_cols(cols), m_rows(rows) {
	m_items.resize(cols * rows);
	float itemSizeX = 1.0f / (float) cols;
	float itemSizeY = 1.0f / (float) rows;
	for (int x = 0; x < cols; x++) {
		for (int y = 0; y < rows; y++) {
			m_items[y * m_cols + x] = std::make_unique<InventoryItem>();
			addChild(
					&item(x, y),
					glm::vec2(
							(((float) x + 0.5f) * itemSizeX) * 2.0f - 1.0f,
							(((float) y + 0.5f) * itemSizeY) * 2.0f - 1.0f
					),
					glm::vec2(itemSizeX, itemSizeY)
			);
		}
	}
}

void Inventory::render(const glm::mat4 &transform) {
	for (auto &child : children()) {
		auto &item = static_cast<InventoryItem&>(*child->element);
		if (item.active()) continue;
		child->element->render(
				glm::scale(
						glm::translate(
								transform,
								glm::vec3(child->position, 0.0f)
						),
						glm::vec3(child->size, 1.0f)
				)
		);
	}
	for (auto &child : children()) {
		auto &item = static_cast<InventoryItem&>(*child->element);
		if (!item.active()) continue;
		child->element->render(
				glm::scale(
						glm::translate(
								transform,
								glm::vec3(child->position, 0.0f)
						),
						glm::vec3(child->size, 1.0f)
				)
		);
	}
}

const InventoryItem &Inventory::item(int col, int row) const {
	return *m_items[row * m_cols + col];
}

InventoryItem &Inventory::item(int col, int row) {
	return *m_items[row * m_cols + col];
}

void Inventory::setActive(int index) {
	if (index == m_activeIndex) return;
	if (m_activeIndex >= 0) {
		item(m_activeIndex).setActive(false);
	}
	m_activeIndex = index;
	if (index >= 0) {
		item(index).setActive(true);
	}
}

void Inventory::setUpdate(std::unordered_map<uint8_t, VoxelHolder> &&changes, int active) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_nextChanges = std::move(changes);
	m_nextActive = active;
	m_hasUpdate = true;
}

void Inventory::update() {
	std::unique_lock<std::mutex> lock(m_mutex);
	if (!m_hasUpdate) return;
	for (auto &update : m_nextChanges) {
		LOG(TRACE) << "Set inventory item #" << (int) update.first << " to \"" << update.second.toString() << "\"";
		item(update.first).setVoxel(std::move(update.second));
	}
	setActive(m_nextActive);
	m_hasUpdate = false;
	LOG(DEBUG) << "Inventory updated";
}
