#include <sqlite3.h>
#include <easylogging++.h>
#include "VoxelWorldStorage.h"
#include "world/VoxelTypeRegistry.h"

VoxelWorldStorage::VoxelWorldStorage(
		std::string fileName,
		VoxelTypeRegistry &registry,
		VoxelChunkLoader &generator
): m_fileName(std::move(fileName)), m_registry(registry), m_generator(generator), m_serializationContext(registry) {
	m_thread = std::thread(&VoxelWorldStorage::run, this);
}

VoxelWorldStorage::~VoxelWorldStorage() {
	m_running = false;
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queueCondVar.notify_one();
	lock.unlock();
	m_thread.join();
}

void VoxelWorldStorage::openDatabase() {
	int retVal = sqlite3_open(m_fileName.c_str(), &m_database);
	if (retVal != SQLITE_OK) {
		LOG(ERROR) << "Unable to open database \"" << m_fileName << "\": " << sqlite3_errmsg(m_database);
		closeDatabase();
		return;
	}
	char *errorMsg;
	static const char *initSql =
			"CREATE TABLE IF NOT EXISTS voxel_types (\n"
   			"	id INTEGER NOT NULL PRIMARY KEY,\n"
	  		"	name TEXT NOT NULL UNIQUE\n"
	 		");\n"
   			"CREATE TABLE IF NOT EXISTS chunks (\n"
	  		"	x INTEGER NOT NULL,\n"
			"	y INTEGER NOT NULL,\n"
   			"	z INTEGER NOT NULL,\n"
	  		"	data BLOB NOT NULL,\n"
	 		"	PRIMARY KEY(x, y, z)\n"
	 		");\n";
	if (sqlite3_exec(m_database, initSql, nullptr, nullptr, &errorMsg) != SQLITE_OK) {
		LOG(ERROR) << "Failed to init database: " << errorMsg;
		sqlite3_free(errorMsg);
		closeDatabase();
		return;
	}
	
	static const char *queryVoxelTypesSql = "SELECT id, name FROM voxel_types ORDER BY id";
	sqlite3_stmt *stmt = nullptr;
	retVal = sqlite3_prepare_v2(m_database, queryVoxelTypesSql, -1, &stmt, nullptr);
	if (retVal != SQLITE_OK) {
		LOG(ERROR) << "Failed to prepare query voxel types SQL statement: " << sqlite3_errmsg(m_database);
		closeDatabase();
		return;
	}
	std::unordered_set<std::string> existingTypes;
	while ((retVal = sqlite3_step(stmt)) == SQLITE_ROW) {
		auto id = sqlite3_column_int(stmt, 0);
		auto name = sqlite3_column_text(stmt, 1);
		m_serializationContext.setTypeId(id, (const char*) name);
		existingTypes.emplace((const char*) name);
		LOG(INFO) << "Loaded voxel type \"" << name << "\" (id=" << id << ")";
	}
	if (retVal != SQLITE_DONE) {
		LOG(ERROR) << "Failed to execute query voxel types SQL statement: " << sqlite3_errmsg(m_database);
		sqlite3_finalize(stmt);
		closeDatabase();
		return;
	}
	sqlite3_finalize(stmt);
	
	static const char *insertVoxelTypeSql = "INSERT INTO voxel_types (id, name) VALUES (?, ?)";
	retVal = sqlite3_prepare_v2(m_database, insertVoxelTypeSql, -1, &stmt, nullptr);
	if (retVal != SQLITE_OK) {
		LOG(ERROR) << "Failed to prepare insert voxel type SQL statement: " << sqlite3_errmsg(m_database);
		closeDatabase();
		return;
	}
	retVal = SQLITE_DONE;
	m_registry.forEach([this, &existingTypes, stmt, &retVal](const std::string &name, VoxelType &type) {
		if (existingTypes.count(name)) return;
		if (retVal != SQLITE_DONE) return;
		auto id = m_serializationContext.typeId(type);
		assert(id >= 0);
		LOG(INFO) << "Stored voxel type \"" << name << "\" (id=" << id << ")";
		sqlite3_bind_int(stmt, 1, id);
		sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
		retVal = sqlite3_step(stmt);
		if (retVal == SQLITE_DONE) {
			sqlite3_reset(stmt);
		} else {
			LOG(ERROR) << "Failed to execute insert voxel type SQL statement: " << sqlite3_errmsg(m_database);
		}
	});
	if (retVal != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		closeDatabase();
		return;
	}
	sqlite3_finalize(stmt);
	
	static const char *storeChunkSql = "INSERT OR REPLACE INTO chunks (x, y, z, data) VALUES (?, ?, ?, ?)";
	retVal = sqlite3_prepare_v2(m_database, storeChunkSql, -1, &m_storeChunkStmt, nullptr);
	if (retVal != SQLITE_OK) {
		LOG(ERROR) << "Failed to prepare store chunk SQL statement: " << sqlite3_errmsg(m_database);
		closeDatabase();
	}
}

void VoxelWorldStorage::closeDatabase() {
	if (m_database == nullptr) return;
	sqlite3_close(m_database);
	m_database = nullptr;
}

void VoxelWorldStorage::run() {
	LOG(INFO) << "Voxel world storage thread started";
	openDatabase();
	std::unique_lock<std::mutex> lock(m_queueMutex, std::defer_lock);
	while (true) {
		lock.lock();
		if (m_queue.empty()) {
			if (!m_running) break;
			m_queueCondVar.wait(lock);
		}
		if (m_queue.empty()) continue;
		auto job = std::move(m_queue.front());
		m_queue.pop_front();
		lock.unlock();
		
		if (m_database == nullptr) continue;
		if (std::holds_alternative<ChunkLoadTask>(job)) {
			auto &task = std::get<ChunkLoadTask>(job);
			auto chunk = task.world->mutableChunk(task.location, VoxelWorld::MissingChunkPolicy::CREATE);
			load(chunk);
		} else if (std::holds_alternative<std::unique_ptr<SharedVoxelChunk>>(job)) {
			auto &chunk = *std::get<std::unique_ptr<SharedVoxelChunk>>(job);
			if (!chunk.lightComputed()) continue;
			auto &l = chunk.location();
			LOG(DEBUG) << "Storing chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z;
			std::string buffer;
			VoxelSerializer serializer(m_serializationContext, buffer);
			serializer.object(chunk);
			sqlite3_bind_int(m_storeChunkStmt, 1, l.x);
			sqlite3_bind_int(m_storeChunkStmt, 2, l.y);
			sqlite3_bind_int(m_storeChunkStmt, 3, l.z);
			sqlite3_bind_blob(m_storeChunkStmt, 4, buffer.data(), buffer.size(), SQLITE_STATIC);
			if (sqlite3_step(m_storeChunkStmt) != SQLITE_DONE) {
				LOG(ERROR) << "Failed to store chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z << ": " <<
					sqlite3_errmsg(m_database);
			}
			sqlite3_reset(m_storeChunkStmt);
		}
	}
	std::unique_lock<std::shared_mutex> stmtsLock(m_loadChunkStmtsMutex);
	for (auto &pair : m_loadChunkStmts) {
		sqlite3_finalize(pair.second);
	}
	stmtsLock.unlock();
	closeDatabase();
	LOG(INFO) << "Voxel world storage thread stopped";
}

void VoxelWorldStorage::load(VoxelChunkMutableRef &chunk) {
	sqlite3_stmt *stmt = nullptr;
	std::shared_lock<std::shared_mutex> sharedLock(m_loadChunkStmtsMutex);
	if (m_running && m_database != nullptr) {
		auto threadId = std::this_thread::get_id();
		auto it = m_loadChunkStmts.find(threadId);
		if (it == m_loadChunkStmts.end()) {
			sharedLock.unlock();
			std::unique_lock<std::shared_mutex> lock(m_loadChunkStmtsMutex);
			it = m_loadChunkStmts.find(threadId);
			if (it == m_loadChunkStmts.end()) {
				static const char *loadChunkSql = "SELECT data FROM chunks WHERE x = ? AND y = ? AND Z = ? LIMIT 1";
				auto retVal = sqlite3_prepare_v2(m_database, loadChunkSql, -1, &stmt, nullptr);
				if (retVal != SQLITE_OK) {
					LOG(ERROR) << "Failed to prepare load chunk SQL statement: " << sqlite3_errmsg(m_database);
				}
				m_loadChunkStmts.emplace(threadId, stmt);
			} else {
				stmt = it->second;
			}
			lock.unlock();
			sharedLock.lock();
			if (!m_running) return;
		} else {
			stmt = it->second;
		}
	}
	if (stmt != nullptr) {
		auto &l = chunk.location();
		sqlite3_bind_int(stmt, 1, l.x);
		sqlite3_bind_int(stmt, 2, l.y);
		sqlite3_bind_int(stmt, 3, l.z);
		auto retVal = sqlite3_step(stmt);
		if (retVal == SQLITE_ROW) {
			LOG(DEBUG) << "Loading chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z;
			const void *data = sqlite3_column_blob(stmt, 0);
			size_t dataSize = sqlite3_column_bytes(stmt, 0);
			std::string buffer((const char*) data, dataSize);
			VoxelDeserializer deserializer(m_serializationContext, buffer.cbegin(), buffer.cend());
			deserializer.object(chunk);
			sqlite3_reset(stmt);
			chunk.markDirty(true);
			return;
		}
		if (retVal != SQLITE_DONE) {
			LOG(ERROR) << "Failed to load chunk at x=" << l.x << ",y=" << l.y << ",z=" << l.z << ": " <<
				sqlite3_errmsg(m_database);
		}
		sqlite3_reset(stmt);
	}
	m_generator.load(chunk);
}

void VoxelWorldStorage::loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queue.emplace_back(ChunkLoadTask(&world, location));
	lock.unlock();
	m_queueCondVar.notify_one();
}

void VoxelWorldStorage::cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	auto it = m_queue.begin();
	while (it != m_queue.end()) {
		if (std::holds_alternative<ChunkLoadTask>(*it)) {
			auto &task = std::get<ChunkLoadTask>(*it);
			if (task.world == &world && task.location == location) {
				it = m_queue.erase(it);
				continue;
			}
		}
		++it;
	}
}

void VoxelWorldStorage::unloadChunkAsync(std::unique_ptr<SharedVoxelChunk> chunk) {
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_queue.emplace_back(std::move(chunk));
	lock.unlock();
	m_queueCondVar.notify_one();
}
