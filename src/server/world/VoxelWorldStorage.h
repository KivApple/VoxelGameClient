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
#include "Worker.h"

struct sqlite3;
struct sqlite3_stmt;
class VoxelWorldStorage;

enum class VoxelWorldStorageAction {
	LOAD,
	STORE
};

struct VoxelWorldStorageJob {
	VoxelWorldStorage *storage;
	VoxelWorldStorageAction action;
	VoxelWorld *world;
	VoxelChunkLocation location;
	
	VoxelWorldStorageJob(
			VoxelWorldStorage *storage,
			VoxelWorldStorageAction action,
			VoxelWorld *world,
			const VoxelChunkLocation &location
	);
	bool operator==(const VoxelWorldStorageJob &job) const;
	void operator()() const;
};

class VoxelWorldStorage: public VoxelChunkLoader, public Worker<VoxelWorldStorageJob> {
	std::string m_fileName;
	sqlite3 *m_database = nullptr;
	std::unordered_map<std::thread::id, sqlite3_stmt*> m_loadChunkStmts;
	std::shared_mutex m_loadChunkStmtsMutex;
	sqlite3_stmt *m_storeChunkStmt = nullptr;
	VoxelTypeRegistry &m_registry;
	VoxelChunkLoader &m_generator;
	VoxelTypeSerializationContext m_serializationContext;
	
	void openDatabase();
	void closeDatabase();
	void store(const VoxelChunkRef &chunk);
	
	friend class VoxelWorldStorageJob;
	
public:
	VoxelWorldStorage(std::string fileName, VoxelTypeRegistry &registry, VoxelChunkLoader &generator);
	~VoxelWorldStorage() override;
	void load(VoxelChunkMutableRef &chunk) override;
	void loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	void cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	void storeChunkAsync(VoxelWorld &world, const VoxelChunkLocation &location) override;
	
};
