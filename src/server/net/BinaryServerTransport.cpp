#include "BinaryServerTransport.h"
#include "net/ServerMessage.h"
#include "net/ClientMessage.h"
#include "server/GameServerEngine.h"

void BinaryServerTransport::Connection::deserializeAndHandleMessage(const std::string &payload) {
	ClientMessage<ClientMessageData::Empty> genericMessage;
	deserialize(payload, genericMessage);
	switch (genericMessage.type) {
		case ClientMessageType::UPDATE_POSITION: {
			ClientMessage<ClientMessageData::UpdatePosition> msg;
			deserialize(payload, msg);
			updatePosition({msg.data.x, msg.data.y, msg.data.z}, msg.data.yaw, msg.data.pitch, msg.data.viewRadius);
			break;
		}
		case ClientMessageType::UPDATE_ACTIVE_INVENTORY_ITEM: {
			ClientMessage<ClientMessageData::UpdateActiveInventoryItem> msg;
			deserialize(payload, msg);
			setActiveInventoryIndex(msg.data.active);
			break;
		}
		case ClientMessageType::DIG_VOXEL:
			digVoxel();
			break;
		case ClientMessageType::PLACE_VOXEL:
			placeVoxel();
			break;
		default:
			logger().warn(
					"[Client %v] Unknown message received from client: %v (%v bytes)",
					this, (int) genericMessage.type, payload.size()
			);
	}
}

void BinaryServerTransport::Connection::sendHello() {
	std::unordered_map<uint8_t, VoxelHolder> inventory;
	int active;
	queryInventory(inventory, active);
	inventoryUpdated(std::move(inventory), active);
}

void BinaryServerTransport::Connection::setPosition(const glm::vec3 &position) {
	serializeAndSendMessage(ServerMessage(ServerMessageData::SetPosition {
		position.x, position.y, position.z
	}));
}

void BinaryServerTransport::Connection::setVoxelTypes() {
	std::unique_lock<std::shared_mutex> lock(m_voxelSerializationContextMutex);
	if (!m_voxelSerializationContext) {
		m_voxelSerializationContext = std::make_unique<VoxelTypeSerializationContext>(
				transport().engine()->voxelTypeRegistry()
		);
	}
	std::string buffer;
	serializeAndSendMessage(ServerMessage(ServerMessageData::SetVoxelTypes {
		*m_voxelSerializationContext
	}));
	logger().debug("[Client %v] Sent voxel types", this);
}

std::shared_lock<std::shared_mutex> BinaryServerTransport::Connection::ensureVoxelSerializationContext() {
	std::shared_lock<std::shared_mutex> lock(m_voxelSerializationContextMutex);
	while (!m_voxelSerializationContext) {
		lock.unlock();
		setVoxelTypes();
		lock.lock();
	}
	return std::move(lock);
}

void BinaryServerTransport::Connection::setChunk(const VoxelChunkRef &chunk) {
	auto lock = ensureVoxelSerializationContext();
	logger().debug(
			"[Client %v] Sending chunk x=%v,y=%v,z=%v",
			this, chunk.location().x, chunk.location().y, chunk.location().z
	);
	std::string buffer;
	VoxelSerializer serializer(*m_voxelSerializationContext, buffer);
	serializer.object(ServerMessage(ServerMessageData::SetChunk { chunk.location() }));
	serializer.object(chunk);
	sendMessage(buffer.data(), serializer.adapter().currentWritePos());
}

void BinaryServerTransport::Connection::discardChunks(const std::vector<VoxelChunkLocation> &locations) {
	serializeAndSendMessage(ServerMessage(ServerMessageData::DiscardChunks {locations}));
}

void BinaryServerTransport::Connection::inventoryUpdated(
		std::unordered_map<uint8_t, VoxelHolder> &&changes,
		int active
) {
	auto lock = ensureVoxelSerializationContext();
	logger().debug("[Client %v] Sending %v updated inventory item(s)", this, changes.size());
	std::string buffer;
	VoxelSerializer serializer(*m_voxelSerializationContext, buffer);
	serializer.object(ServerMessage(ServerMessageData::SetInventory {
		std::move(changes),
		(uint8_t) active
	}));
	sendMessage(buffer.data(), serializer.adapter().currentWritePos());
}
