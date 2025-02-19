#include <algorithm>
#include <easylogging++.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "VoxelWorldRenderer.h"
#include "world/VoxelWorld.h"
#include "world/VoxelWorldUtils.h"
#ifndef __EMSCRIPTEN__
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#endif

VoxelWorldRenderer::VoxelWorldRenderer(VoxelWorld &world): m_world(world) {
}

void VoxelWorldRenderer::invalidate(const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queue.emplace(location);
}

std::optional<VoxelChunkLocation> VoxelWorldRenderer::getInvalidated(const glm::vec3 &playerPosition) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	auto nearestIt = m_queue.end();
	float nearestDistance = INFINITY;
	for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
		float dx = (float) it->x - playerPosition.x;
		float dy = (float) it->y - playerPosition.y;
		float dz = (float) it->z - playerPosition.z;
		float distance = dx * dx + dy * dy + dz * dz;
		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearestIt = it;
		}
	}
	if (nearestIt == m_queue.end()) {
		return std::nullopt;
	}
	auto location = *nearestIt;
	m_queue.erase(nearestIt);
	return std::make_optional(location);
}

void VoxelWorldRenderer::buildInvalidated(const glm::vec3 &playerPosition) {
	std::optional<VoxelChunkLocation> location;
	do {
		location = getInvalidated(playerPosition);
	} while (location.has_value() && !build(*location));
}

constexpr int VoxelWorldRenderer::shaderProviderPriority(const VoxelShaderProvider *shaderProvider) {
	return shaderProvider != nullptr ? shaderProvider->priority() : INT_MIN;
}

constexpr float VoxelWorldRenderer::convertLightLevel(VoxelLightLevel level) {
	return std::max(std::min((float) level / MAX_VOXEL_LIGHT_LEVEL, 1.0f), 0.05f);
}

void VoxelWorldRenderer::build(
		const VoxelChunkExtendedRef &chunk,
		const InChunkVoxelLocation &location,
		std::unordered_map<const VoxelShaderProvider*, std::vector<float>> &parts
) {
	auto &cur = chunk.at(location);
	auto shaderProvider = cur.shaderProvider();
	auto curPriority = shaderProviderPriority(shaderProvider);
	if (curPriority < 0) return;
	
	auto &prevX = chunk.extendedAt({location.x - 1, location.y, location.z});
	auto &prevY = chunk.extendedAt({location.x, location.y - 1, location.z});
	auto &prevZ = chunk.extendedAt({location.x, location.y, location.z - 1});

	auto &nextX = chunk.extendedAt({location.x + 1, location.y, location.z});
	auto &nextY = chunk.extendedAt({location.x, location.y + 1, location.z});
	auto &nextZ = chunk.extendedAt({location.x, location.y, location.z + 1});

	auto prevXPriority = shaderProviderPriority(prevX.shaderProvider());
	auto prevYPriority = shaderProviderPriority(prevY.shaderProvider());
	auto prevZPriority = shaderProviderPriority(prevZ.shaderProvider());
	
	auto nextXPriority = shaderProviderPriority(nextX.shaderProvider());
	auto nextYPriority = shaderProviderPriority(nextY.shaderProvider());
	auto nextZPriority = shaderProviderPriority(nextZ.shaderProvider());
	
	if (
			curPriority == nextXPriority && curPriority == nextYPriority && curPriority == nextZPriority &&
			curPriority == prevXPriority && curPriority == prevYPriority && curPriority == prevZPriority
	) return;
	
	m_vertexDataBuffer.clear();
	cur.buildVertexData(chunk, location, m_vertexDataBuffer);
	
	auto it = parts.find(shaderProvider);
	if (it == parts.end()) {
		parts.emplace(shaderProvider, std::vector<float>());
		it = parts.find(shaderProvider);
	}
	auto &outData = it->second;
	
	auto x0 = (float) location.x;
	auto y0 = (float) location.y;
	auto z0 = (float) location.z;
	
	for (unsigned int i = 0; i < m_vertexDataBuffer.size(); i += 3) {
		auto &v0 = m_vertexDataBuffer[i];
		auto &v1 = m_vertexDataBuffer[i + 1];
		auto &v2 = m_vertexDataBuffer[i + 2];

		if (almostEqual(v0.x, v1.x) && almostEqual(v1.x, v2.x)) {
			if (almostEqual(v0.x , -0.5f)) {
				if (&prevX.type() == &EmptyVoxelType::INSTANCE) continue;
				if (prevXPriority >= curPriority) continue;
			} else if (almostEqual(v0.x, 0.5f)) {
				if (&nextX.type() == &EmptyVoxelType::INSTANCE) continue;
				if (nextXPriority >= curPriority) continue;
			}
		}
		if (almostEqual(v0.y, v1.y) && almostEqual(v1.y, v2.y)) {
			if (almostEqual(v0.y , -0.5f)) {
				if (&prevY.type() == &EmptyVoxelType::INSTANCE) continue;
				if (prevYPriority >= curPriority) continue;
			} else if (almostEqual(v0.y, 0.5f)) {
				if (&nextY.type() == &EmptyVoxelType::INSTANCE) continue;
				if (nextYPriority >= curPriority) continue;
			}
		}
		if (almostEqual(v0.z, v1.z) && almostEqual(v1.z, v2.z)) {
			if (almostEqual(v0.z , -0.5f)) {
				if (&prevZ.type() == &EmptyVoxelType::INSTANCE) continue;
				if (prevZPriority >= curPriority && &prevZ.type() != &EmptyVoxelType::INSTANCE) continue;
			} else if (almostEqual(v0.z, 0.5f)) {
				if (&nextZ.type() == &EmptyVoxelType::INSTANCE) continue;
				if (nextZPriority >= curPriority && &nextZ.type() != &EmptyVoxelType::INSTANCE) continue;
			}
		}
		
		for (unsigned int j = 0; j < 3; j++) {
			auto &v = m_vertexDataBuffer[i + j];
			outData.insert(outData.end(), {
				x0 + v.x, y0 + v.y, z0 + v.z, v.u, 1.0f - v.v
			});
		}
	}
}

void VoxelWorldRenderer::buildTexturePixel(VoxelChunkMesh &mesh, int x, int y, int z, const VoxelHolder &voxel) {
	int x0 = ((y + 1) % 5) * (VOXEL_CHUNK_SIZE + 2);
	int y0 = ((y + 1) / 5) * (VOXEL_CHUNK_SIZE + 2);
	int i = ((y0 + (z + 1)) * 5 * (VOXEL_CHUNK_SIZE + 2) + (x0 + (x + 1))) * 4;
	assert((i + 4) <= mesh.textureData.size());
	mesh.textureData[i + 0] = (int) (convertLightLevel(voxel.lightLevel()) * 255); // R
	mesh.textureData[i + 1] = 0; // G
	mesh.textureData[i + 2] = 0; // B
	mesh.textureData[i + 3] = 255; // A
}

void VoxelWorldRenderer::buildTexture(const VoxelChunkExtendedRef &chunk, VoxelChunkMesh &mesh) {
	for (int z = -1; z <= VOXEL_CHUNK_SIZE; z++) {
		for (int y = -1; y <= VOXEL_CHUNK_SIZE; y++) {
			for (int x = -1; x <= VOXEL_CHUNK_SIZE; x++) {
				buildTexturePixel(mesh, x, y, z, chunk.extendedAt(x, y, z));
			}
		}
	}
}

bool VoxelWorldRenderer::build(const VoxelChunkLocation &location, VoxelChunkMesh &mesh) {
	auto chunk = m_world.extendedChunk(location);
	if (!chunk) return false;
	mesh.valid = false;
	for (auto &&part : mesh.parts) {
		part.second.clear();
	}
	buildTexture(chunk, mesh);
	for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
				build(chunk, {x, y, z}, mesh.parts);
			}
		}
	}
	auto it = mesh.parts.begin();
	while (it != mesh.parts.end()) {
		if (it->second.empty()) {
			it = mesh.parts.erase(it);
		} else {
			++it;
		}
	}
	mesh.valid = true;
	return true;
}

bool VoxelWorldRenderer::build(const VoxelChunkLocation &location) {
	PerformanceMeasurement measurement(m_buildPerformanceCounter);
	std::shared_lock<std::shared_mutex> meshesSharedLock(m_meshesMutex);
	auto it = m_meshes.find(location);
	while (it == m_meshes.end()) {
		meshesSharedLock.unlock();
		std::unique_lock<std::shared_mutex> meshesLock(m_meshesMutex);
		it = m_meshes.find(location);
		if (it == m_meshes.end()) {
			m_meshes.emplace(location, std::make_unique<VoxelChunkMesh>());
		}
		meshesLock.unlock();
		meshesSharedLock.lock();
		it = m_meshes.find(location);
	}
	auto &mesh = *it->second;
	std::unique_lock<std::mutex> lock(mesh.mutex);
	meshesSharedLock.unlock();
	if (build(location, mesh)) {
		return true;
	}
	measurement.discard();
	return false;
}

GL::Buffer VoxelWorldRenderer::allocateBuffer() {
	if (!m_buffers.empty()) {
		auto it = m_buffers.begin() + m_buffers.size() - 1;
		auto buffer = std::move(*it);
		m_buffers.erase(it);
		return buffer;
	}
	return GL::Buffer(GL_ARRAY_BUFFER);
}

void VoxelWorldRenderer::freeBuffer(GL::Buffer &&buffer) {
	m_buffers.emplace_back(std::move(buffer));
}

GL::Texture VoxelWorldRenderer::allocateTexture() {
	if (!m_textures.empty()) {
		auto it = m_textures.begin() + m_textures.size() - 1;
		auto texture = std::move(*it);
		m_textures.erase(it);
		return texture;
	}
	return GL::Texture(
			5 * (VOXEL_CHUNK_SIZE + 2),
			5 * (VOXEL_CHUNK_SIZE + 2),
			false
	);
}

void VoxelWorldRenderer::freeTexture(GL::Texture &&texture) {
	m_textures.emplace_back(std::move(texture));
}

void VoxelWorldRenderer::updateBuffersAndScheduleRender(
		const VoxelChunkLocation &location,
		std::shared_lock<std::shared_mutex> &meshesLock
) {
	auto it = m_meshes.find(location);
	if (it == m_meshes.end()) {
		invalidate(location);
		return;
	}
	auto &mesh = *it->second;
	if (mesh.valid) {
		std::unique_lock<std::mutex> lock(mesh.mutex);
		meshesLock.unlock();
		if (mesh.valid) {
			if (!mesh.texture.has_value()) {
				mesh.texture.emplace(allocateTexture());
			}
			mesh.texture->setData(mesh.textureData.data());
			for (auto &part : mesh.buffers) {
				part.second.vertexCount = 0;
			}
			for (auto &part : mesh.parts) {
				if (part.second.empty()) continue;
				auto it2 = mesh.buffers.find(part.first);
				if (it2 == mesh.buffers.end()) {
					mesh.buffers.emplace(part.first, VoxelMeshPart {
							allocateBuffer(),
							(unsigned int) part.second.size() / 5
					});
					it2 = mesh.buffers.find(part.first);
				} else {
					it2->second.vertexCount = (unsigned int) part.second.size() / 5;
				}
				it2->second.buffer.setData(
						part.second.data(),
						part.second.size() * sizeof(float),
						GL_DYNAMIC_DRAW
				);
			}
			mesh.valid = false;
			auto it2 = mesh.buffers.begin();
			while (it2 != mesh.buffers.end()) {
				if (it2->second.vertexCount == 0) {
					freeBuffer(std::move(it2->second.buffer));
					it2 = mesh.buffers.erase(it2);
				} else {
					++it2;
				}
			}
		}
		lock.unlock();
		meshesLock.lock();
	}
	for (auto &part : mesh.buffers) {
		if (part.first == nullptr) continue;
		m_renderSchedule.emplace_back(VoxelChunkRenderStep {
			location,
			part.first,
			&part.second,
			&mesh.texture.value()
		});
	}
}

bool VoxelWorldRenderer::isChunkVisible(
		const VoxelChunkLocation &location,
		const VoxelChunkLocation &playerLocation,
		int radius
) {
	int dx = abs(location.x - playerLocation.x);
	int dy = abs(location.y - playerLocation.y);
	int dz = abs(location.z - playerLocation.z);
	return dx <= radius && dy <= radius && dz <= radius;
}

void VoxelWorldRenderer::cleanupQueue(const VoxelChunkLocation &playerLocation, int radius) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	auto it = m_queue.begin();
	while (it != m_queue.end()) {
		if (isChunkVisible(*it, playerLocation, radius)) {
			++it;
		} else {
			it = m_queue.erase(it);
		}
	}
}

void VoxelWorldRenderer::freeBuffers(const VoxelChunkLocation &playerLocation, int radius) {
	std::unique_lock<std::shared_mutex> meshesUniqueLock(m_meshesMutex);
	auto it = m_meshes.begin();
	while (it != m_meshes.end()) {
		if (isChunkVisible(it->first, playerLocation, radius)) {
			++it;
		} else {
			auto &mesh = *it->second;
			std::unique_lock<std::mutex> lock(mesh.mutex, std::try_to_lock);
			if (lock.owns_lock()) {
				for (auto &&part : mesh.buffers) {
					freeBuffer(std::move(part.second.buffer));
				}
				if (mesh.texture.has_value()) {
					freeTexture(std::move(mesh.texture.value()));
				}
				lock.unlock();
				it = m_meshes.erase(it);
			} else {
				++it;
			}
		}
	}
}

void VoxelWorldRenderer::updateBuffersAndScheduleRender(const VoxelChunkLocation &playerLocation, int radius) {
	m_renderSchedule.clear();
	std::shared_lock<std::shared_mutex> meshesLock(m_meshesMutex);
	for (int z = -radius; z <= radius; z++) {
		for (int y = -radius; y <= radius; y++) {
			for (int x = -radius; x <= radius; x++) {
				updateBuffersAndScheduleRender({
					playerLocation.x + x,
					playerLocation.y + y,
					playerLocation.z + z
				}, meshesLock);
			}
		}
	}
}

void VoxelWorldRenderer::renderScheduled(const glm::mat4 &view, const glm::mat4 &projection) {
	const CommonShaderProgram *prevProgram = nullptr;
	const VoxelShaderProvider *prevShaderProvider = nullptr;
	const GL::Texture *prevChunkTexture = nullptr;
	for (auto &step : m_renderSchedule) {
		auto &program = step.shaderProvider->get();
		if (prevProgram != &program) {
			prevProgram = &program;
			program.use();
			program.setView(view);
			program.setProjection(projection);
		}
		program.setModel(glm::translate(glm::mat4(1.0f), glm::vec3(
				(float) (step.location.x * VOXEL_CHUNK_SIZE),
				(float) (step.location.y * VOXEL_CHUNK_SIZE),
				(float) (step.location.z * VOXEL_CHUNK_SIZE)
		)));
		if (prevShaderProvider != step.shaderProvider) {
			if (prevShaderProvider == nullptr || prevShaderProvider->priority() == MAX_VOXEL_SHADER_PRIORITY) {
				if (step.shaderProvider->priority() < MAX_VOXEL_SHADER_PRIORITY) {
					//glDisable(GL_DEPTH_TEST);
					glEnable(GL_BLEND);
				}
			}
			
			prevShaderProvider = step.shaderProvider;
			step.shaderProvider->setup(program);
		}
		if (prevChunkTexture != step.chunkTexture && step.chunkTexture != nullptr) {
			prevChunkTexture = step.chunkTexture;
			program.setChunkTexture(*step.chunkTexture);
		}
		program.setPositions(step.part->buffer.pointer(
				GL_FLOAT,
				0,
				sizeof(float) * 5
		));
		program.setTexCoords(step.part->buffer.pointer(
				GL_FLOAT,
				sizeof(float) * 3,
				sizeof(float) * 5
		));
		glDrawArrays(GL_TRIANGLES, 0, step.part->vertexCount);
	}
}

void VoxelWorldRenderer::render(
		const glm::vec3 &playerPosition,
		int radius,
		const glm::mat4 &view,
		const glm::mat4 &projection
) {
	PerformanceMeasurement measurement(m_renderPerformanceCounter);
	auto playerChunkLocation = VoxelLocation(
			(int) roundf(playerPosition.x),
			(int) roundf(playerPosition.y),
			(int) roundf(playerPosition.z)
	).chunk();
	cleanupQueue(playerChunkLocation, radius);
	buildInvalidated(playerPosition);
	updateBuffersAndScheduleRender(playerChunkLocation, radius);
	freeBuffers(playerChunkLocation, radius);
	std::sort(
			m_renderSchedule.begin(),
			m_renderSchedule.end(),
			[](const VoxelChunkRenderStep &a, const VoxelChunkRenderStep &b) {
				if (a.shaderProvider->priority() > b.shaderProvider->priority()) {
					return true;
				} else if (a.shaderProvider->priority() == b.shaderProvider->priority()) {
					return a.shaderProvider > b.shaderProvider;
				} else {
					return false;
				}
			}
	);
	renderScheduled(view, projection);
}

void VoxelWorldRenderer::reset() {
	std::unique_lock<std::mutex> lock1(m_queueMutex);
	std::unique_lock<std::shared_mutex> lock2(m_meshesMutex);
	m_queue.clear();
	m_meshes.clear();
	m_buffers.clear();
}

void VoxelWorldRenderer::saveChunkTexture(const glm::vec3 &playerPosition) {
#ifndef __EMSCRIPTEN__
	auto l = VoxelLocation(
			(int) roundf(playerPosition.x),
			(int) roundf(playerPosition.y),
			(int) roundf(playerPosition.z)
	).chunk();
	std::shared_lock<std::shared_mutex> meshesLock(m_meshesMutex);
	auto it = m_meshes.find(l);
	if (it == m_meshes.end()) {
		return;
	}
	LOG(INFO) << "Saving the texture for chunk x=" << l.x << ",y=" << l.y << ",z=" << l.z;
	std::unique_lock<std::mutex> lock(it->second->mutex);
	std::array<uint8_t, (5 * (VOXEL_CHUNK_SIZE + 2) * 5 * (VOXEL_CHUNK_SIZE + 2)) * 4> buffer;
	it->second->texture->bind();
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
	stbi_write_png(
			"chunk_texture.png",
			5 * (VOXEL_CHUNK_SIZE + 2),
			5 * (VOXEL_CHUNK_SIZE + 2),
			4,
			buffer.data(),
			5 * 18 * 4
	);
#endif
}
