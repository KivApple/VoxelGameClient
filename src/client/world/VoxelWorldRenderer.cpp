#include <algorithm>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "VoxelWorldRenderer.h"
#include "world/VoxelWorld.h"

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

constexpr bool VoxelWorldRenderer::almostEqual(float a, float b) {
	return fabsf(a - b) < 0.001f;
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
	
	auto prevX = shaderProviderPriority(
			chunk.extendedAt({location.x - 1, location.y, location.z}).shaderProvider()
	);
	auto prevY = shaderProviderPriority(
			chunk.extendedAt({location.x, location.y - 1, location.z}).shaderProvider()
	);
	auto prevZ = shaderProviderPriority(
			chunk.extendedAt({location.x, location.y, location.z - 1}).shaderProvider()
	);
	
	auto nextX = shaderProviderPriority(
			chunk.extendedAt({location.x + 1, location.y, location.z}).shaderProvider()
	);
	auto nextY = shaderProviderPriority(
			chunk.extendedAt({location.x, location.y + 1, location.z}).shaderProvider()
	);
	auto nextZ = shaderProviderPriority(
			chunk.extendedAt({location.x, location.y, location.z + 1}).shaderProvider()
	);
	
	if (
			curPriority == nextX && curPriority == nextY && curPriority == nextZ &&
			curPriority == prevX && curPriority == prevY && curPriority == prevZ
	) return;
	
	m_vertexDataBuffer.clear();
	cur.buildVertexData(m_vertexDataBuffer);
	
	auto it = parts.find(shaderProvider);
	if (it == parts.end()) {
		parts.emplace(shaderProvider, std::vector<float>());
		it = parts.find(shaderProvider);
	}
	auto &outData = it->second;
	
	auto x0 = (float) location.x;
	auto y0 = (float) location.y;
	auto z0 = (float) location.z;
	
	for (int i = 0; i < m_vertexDataBuffer.size(); i += 3) {
		auto &v0 = m_vertexDataBuffer[i];
		auto &v1 = m_vertexDataBuffer[i + 1];
		auto &v2 = m_vertexDataBuffer[i + 2];
		
		if (almostEqual(v0.x, v1.x) && almostEqual(v1.x, v2.x)) {
			if (almostEqual(v0.x , -0.5f)) {
				if (prevX >= curPriority) continue;
			} else if (almostEqual(v0.x, 0.5f)) {
				if (nextX >= curPriority) continue;
			}
		}
		if (almostEqual(v0.y, v1.y) && almostEqual(v1.y, v2.y)) {
			if (almostEqual(v0.y , -0.5f)) {
				if (prevY >= curPriority) continue;
			} else if (almostEqual(v0.y, 0.5f)) {
				if (nextY >= curPriority) continue;
			}
		}
		if (almostEqual(v0.z, v1.z) && almostEqual(v1.z, v2.z)) {
			if (almostEqual(v0.z , -0.5f)) {
				if (prevZ >= curPriority) continue;
			} else if (almostEqual(v0.z, 0.5f)) {
				if (nextZ >= curPriority) continue;
			}
		}
		
		outData.insert(outData.end(), {
			x0 + v0.x, y0 + v0.y, z0 + v0.z, v0.u, v0.v,
			x0 + v1.x, y0 + v1.y, z0 + v1.z, v1.u, v1.v,
			x0 + v2.x, y0 + v2.y, z0 + v2.z, v2.u, v2.v
		});
	}
}

bool VoxelWorldRenderer::build(const VoxelChunkLocation &location, VoxelChunkMesh &mesh) {
	auto chunk = m_world.extendedChunk(location);
	if (!chunk) return false;
	mesh.valid = false;
	for (auto &&part : mesh.parts) {
		part.second.clear();
	}
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

GLBuffer VoxelWorldRenderer::allocateBuffer() {
	if (!m_buffers.empty()) {
		auto it = m_buffers.begin() + m_buffers.size() - 1;
		auto buffer = std::move(*it);
		m_buffers.erase(it);
		return buffer;
	}
	return GLBuffer(GL_ARRAY_BUFFER);
}

void VoxelWorldRenderer::freeBuffer(GLBuffer &&buffer) {
	m_buffers.emplace_back(std::move(buffer));
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
			for (auto &part : mesh.buffers) {
				part.second.vertexCount = 0;
			}
			for (auto &part : mesh.parts) {
				if (part.second.empty()) continue;
				auto it2 = mesh.buffers.find(part.first);
				if (it2 == mesh.buffers.end()) {
					mesh.buffers.emplace(part.first, VoxelMeshPart {
							allocateBuffer(),
							part.second.size() / 5
					});
					it2 = mesh.buffers.find(part.first);
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
			&part.second
		});
	}
}

constexpr bool VoxelWorldRenderer::isChunkVisible(
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
			prevShaderProvider = step.shaderProvider;
			step.shaderProvider->setup(program);
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
	VoxelChunkLocation playerChunkLocation(
			(int) roundf(playerPosition.x / VOXEL_CHUNK_SIZE),
			(int) roundf(playerPosition.y / VOXEL_CHUNK_SIZE),
			(int) roundf(playerPosition.z / VOXEL_CHUNK_SIZE)
	);
	cleanupQueue(playerChunkLocation, radius);
	buildInvalidated(playerPosition);
	updateBuffersAndScheduleRender(playerChunkLocation, radius);
	freeBuffers(playerChunkLocation, radius);
	std::sort(
			m_renderSchedule.begin(),
			m_renderSchedule.end(),
			[](const VoxelChunkRenderStep &a, const VoxelChunkRenderStep &b) {
				return a.shaderProvider->priority() > b.shaderProvider->priority();
			}
	);
	renderScheduled(view, projection);
}
