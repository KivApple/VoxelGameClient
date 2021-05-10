#include <easylogging++.h>
#include <sqlite3.h>
#include "VoxelWorldStorage.h"
#include "world/VoxelTypeRegistry.h"

VoxelWorldStorageJob::VoxelWorldStorageJob(
		VoxelWorldStorage *storage,
		VoxelWorldStorageAction action,
		VoxelWorld *world,
		const VoxelChunkLocation &location
): storage(storage), action(action), world(world), location(location) {
}

bool VoxelWorldStorageJob::operator==(const VoxelWorldStorageJob &job) const {
	return action == job.action && world == job.world && location == job.location;
}

void VoxelWorldStorageJob::operator()() const {
	if (!storage->m_database) return;
	switch (action) {
		case VoxelWorldStorageAction::LOAD: {
			auto ref = world->mutableChunk(location, VoxelWorld::MissingChunkPolicy::CREATE);
			storage->load(ref);
			break;
		}
		case VoxelWorldStorageAction::STORE: {
			auto ref = world->chunk(location);
			if (ref) {
				storage->store(ref);
				ref.unlock();
				world->chunkStored(location);
			}
			break;
		}
	}
}

VoxelWorldStorage::VoxelWorldStorage(
		std::string fileName,
		VoxelTypeRegistry &registry,
		VoxelChunkLoader &generator
): Worker("VoxelWorldStorage"), m_fileName(std::move(fileName)),
	m_registry(registry), m_generator(generator), m_serializationContext(registry)
{
	openDatabase();
}

VoxelWorldStorage::~VoxelWorldStorage() {
	shutdown(true);
	std::unique_lock<std::shared_mutex> stmtsLock(m_loadChunkStmtsMutex);
	for (auto &&pair : m_loadChunkStmts) {
		sqlite3_finalize(pair.second);
		pair.second = nullptr;
	}
	LOG(DEBUG) << m_loadChunkStmts.size() << " database statement(s) finalized";
	stmtsLock.unlock();
	closeDatabase();
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
	if (m_storeChunkStmt != nullptr) {
		sqlite3_finalize(m_storeChunkStmt);
		m_storeChunkStmt = nullptr;
	}
	sqlite3_close(m_database);
	m_database = nullptr;
	LOG(DEBUG) << "Database closed";
}

void VoxelWorldStorage::load(VoxelChunkMutableRef &chunk) {
	sqlite3_stmt *stmt = nullptr;
	std::shared_lock<std::shared_mutex> sharedLock(m_loadChunkStmtsMutex);
	if (m_database != nullptr) {
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
			chunk.setLightState(VoxelChunkLightState::READY);
			chunk.setUpdatedAt(0);
			chunk.setStoredAt(0);
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

void VoxelWorldStorage::store(const VoxelChunkRef &chunk) {
	if (!chunk) return;
	switch (chunk.lightState()) {
		case VoxelChunkLightState::PENDING_INITIAL:
		case VoxelChunkLightState::PENDING_INCREMENTAL:
		case VoxelChunkLightState::COMPUTING:
			return;
		case VoxelChunkLightState::READY:
		case VoxelChunkLightState::COMPLETE:
			break;
	}
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

void VoxelWorldStorage::loadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	post(this, VoxelWorldStorageAction::LOAD, &world, location);
}

void VoxelWorldStorage::cancelLoadAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	cancel(VoxelWorldStorageJob(
			this,
			VoxelWorldStorageAction::LOAD,
			&world,
			location
	), false);
}

void VoxelWorldStorage::storeChunkAsync(VoxelWorld &world, const VoxelChunkLocation &location) {
	post(this, VoxelWorldStorageAction::STORE, &world, location);
}
