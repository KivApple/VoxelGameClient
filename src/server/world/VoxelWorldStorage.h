#pragma once

#include <atomic>
#include <variant>
#include <deque>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>
#include <condition_variable>
#include "world/VoxelWorld.h"

struct sqlite3;
struct sqlite3_stmt;

class VoxelWorldStorage: public VoxelChunkLoader {
	struct ChunkLoadTask {
		VoxelWorld *world;
		VoxelChunkLocation location;
		
		ChunkLoadTask(VoxelWorld *world, const VoxelChunkLocation &location): world(world), location(location) {
		}
	};
	
	std::string m_fileName;
	sqlite3 *m_database = nullptr;
	std::unordered_map<std::thread::id, sqlite3_stmt*> m_loadChunkStmts;
	std::shared_mutex m_loadChunkStmtsMutex;
	sqlite3_stmt *m_storeChunkStmt = nullptr;
	VoxelTypeRegistry &m_registry;
	VoxelChunkLoader &m_generator;
	VoxelTypeSerializationContext m_serializationContext;
	std::deque<std::variant<ChunkLoadTask, std::unique_ptr<SharedVoxelChunk>>> m_queue;
	std::mutex m_queueMutex;
	std::condition_variable m_queueCondVar;
	std::atomic<bool> m_running = true;
	std::thread m_thread;
	
	void openDatabase();
	void closeDatabase();
	void run();
	
public:
	VoxelWorldStorage(std::string fileName, VoxelTypeRegistry &registry, VoxelChunkLoader &generator);
	~VoxelWorldStorage() override;
	void load(VoxelChunkMutableRef &chunk) override;
	void loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	void cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	void unloadChunkAsync(std::unique_ptr<SharedVoxelChunk> chunk) override;
	
};
