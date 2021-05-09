#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <glm/mat4x4.hpp>
#include "world/Voxel.h"
#include "world/VoxelLocation.h"
#include "PerformanceCounter.h"

class VoxelShaderProvider;
class VoxelWorld;
class VoxelChunkExtendedRef;

struct VoxelMeshPart {
	GL::Buffer buffer;
	unsigned int vertexCount;
};

struct VoxelChunkMesh {
	std::unordered_map<const VoxelShaderProvider*, std::vector<float>> parts;
	std::atomic<bool> valid = false;
	std::mutex mutex;
	std::unordered_map<const VoxelShaderProvider*, VoxelMeshPart> buffers;
};

struct VoxelChunkRenderStep {
	VoxelChunkLocation location;
	const VoxelShaderProvider *shaderProvider;
	const VoxelMeshPart *part;
};

class VoxelWorldRenderer {
	VoxelWorld &m_world;
	std::unordered_set<VoxelChunkLocation> m_queue;
	std::mutex m_queueMutex;
	std::unordered_map<VoxelChunkLocation, std::unique_ptr<VoxelChunkMesh>> m_meshes;
	std::shared_mutex m_meshesMutex;
	std::vector<VoxelVertexData> m_vertexDataBuffer;
	std::vector<GL::Buffer> m_buffers;
	std::vector<VoxelChunkRenderStep> m_renderSchedule;
	PerformanceCounter m_buildPerformanceCounter;
	PerformanceCounter m_renderPerformanceCounter;
	
	std::optional<VoxelChunkLocation> getInvalidated(const glm::vec3 &playerPosition);
	constexpr static int shaderProviderPriority(const VoxelShaderProvider *shaderProvider);
	constexpr static float convertLightLevel(VoxelLightLevel level);
	constexpr static bool almostEqual(float a, float b);
	void build(
			const VoxelChunkExtendedRef &chunk,
			const InChunkVoxelLocation &location,
			std::unordered_map<const VoxelShaderProvider*, std::vector<float>> &data
	);
	bool build(const VoxelChunkLocation &location, VoxelChunkMesh &mesh);
	bool build(const VoxelChunkLocation &location);
	void buildInvalidated(const glm::vec3 &playerPosition);
	GL::Buffer allocateBuffer();
	void freeBuffer(GL::Buffer &&buffer);
	constexpr static bool isChunkVisible(
			const VoxelChunkLocation &location,
			const VoxelChunkLocation &playerLocation,
			int radius
	);
	void cleanupQueue(const VoxelChunkLocation &playerLocation, int radius);
	void updateBuffersAndScheduleRender(const VoxelChunkLocation &location, std::shared_lock<std::shared_mutex> &meshesLock);
	void freeBuffers(const VoxelChunkLocation &playerLocation, int radius);
	void updateBuffersAndScheduleRender(const VoxelChunkLocation &playerLocation, int radius);
	void renderScheduled(const glm::mat4 &view, const glm::mat4 &projection);
	
public:
	explicit VoxelWorldRenderer(VoxelWorld &world);
	void invalidate(const VoxelChunkLocation &location);
	void render(
			const glm::vec3 &playerPosition,
			int radius,
			const glm::mat4 &view,
			const glm::mat4 &projection
	);
	size_t queueSize() const {
		return m_queue.size();
	}
	size_t availableBufferCount() const {
		return m_buffers.size();
	}
	size_t usedBufferCount() const {
		return m_renderSchedule.size();
	}
	PerformanceCounter &buildPerformanceCounter() {
		return m_buildPerformanceCounter;
	}
	PerformanceCounter &renderPerformanceCounter() {
		return m_renderPerformanceCounter;
	}
	void reset();

};
