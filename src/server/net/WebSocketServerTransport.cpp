#include <condition_variable>
#include "WebSocketServerTransport.h"
#include "../GameServerEngine.h"

/* WebSocketServerTransport::Connection */

WebSocketServerTransport::Connection::Connection(
		WebSocketServerTransport &transport,
		websocketpp::connection_hdl connection
): BinaryServerTransport::Connection(transport), m_connection(connection) {
	logger().info("[Client %v] Connected", this);
	auto conn = transport.m_server.get_con_from_hdl(connection);
	conn->setWriteCompleteHandler(std::bind(&Connection::handleWriteComplete, this, std::placeholders::_1));
	conn->set_message_handler(std::bind(&Connection::handleWebSocketMessage, this, std::placeholders::_2));
	conn->set_close_handler(std::bind(&Connection::handleClose, this));
}

WebSocketServerTransport::Connection::~Connection() {
	if (m_closed) return;
	m_closed = true;
	logger().warn("[Client %v] Closed", this);
	auto conn = webSocketTransport().m_server.get_con_from_hdl(m_connection);
	conn->setWriteCompleteHandler(nullptr);
	conn->set_message_handler(nullptr);
	conn->set_close_handler(nullptr);
	conn->close(1000, "CLOSE_NORMAL");
}

void WebSocketServerTransport::Connection::handleWriteComplete(const websocketpp::lib::error_code &errorCode) {
	if (errorCode) {
		logger().error("[Client %v] Write failed %v", this, errorCode);
		return;
	}
	auto conn = webSocketTransport().m_server.get_con_from_hdl(m_connection);
	if (conn->get_buffered_amount() > 0) {
		return;
	}
	logger().trace("[Client %v] Connection idle detected", this);
	m_sendingPendingChunks = setPendingChunk();
	if (!m_sendingPendingChunks) {
		logger().trace("[Client %v] Finished sending chunks", this);
	}
}

void WebSocketServerTransport::Connection::handleWebSocketMessage(WebSocketServer::message_ptr message) {
	deserializeAndHandleMessage(message->get_payload());
}

void WebSocketServerTransport::Connection::handleClose() {
	if (m_closed) return;
	logger().info("[Client %v] Disconnected", this);
	m_closed = true;
	webSocketTransport().m_engine->unregisterConnection(this);
}

void WebSocketServerTransport::Connection::sendMessage(const void *data, size_t dataSize) {
	if (m_closed) return;
	auto conn = webSocketTransport().m_server.get_con_from_hdl(m_connection);
	if (conn->buffered_amount() > 512 * 1024) {
		logger().warn("[Client %v] Connection is too slow. Closing...", this);
		conn->close(websocketpp::close::status::omit_handshake, "Too slow connection");
		return;
	}
	conn->send((void*) data, dataSize, websocketpp::frame::opcode::binary);
}

void WebSocketServerTransport::Connection::newPendingChunk() {
	if (m_closed) return;
	if (!m_sendingPendingChunks) {
		logger().trace("[Client %v] Started sending chunks", this);
		m_sendingPendingChunks = setPendingChunk();
		if (!m_sendingPendingChunks) {
			logger().trace("[Client %v] Finished sending chunks", this);
		}
	}
}

/* WebSocketServerTransport */

WebSocketServerTransport::WebSocketServerTransport(uint16_t port): m_port(port) {
	m_server.clear_access_channels(websocketpp::log::alevel::all);
	m_server.init_asio();
}

WebSocketServerTransport::~WebSocketServerTransport() {
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void WebSocketServerTransport::start(GameServerEngine &engine) {
	m_engine = &engine;
	m_thread = std::thread(&WebSocketServerTransport::run, this);
}

void WebSocketServerTransport::handleOpen(websocketpp::connection_hdl conn_ptr) {
	m_engine->registerConnection(std::make_unique<Connection>(*this, conn_ptr));
}

void WebSocketServerTransport::run() {
	m_server.set_open_handler(std::bind(&WebSocketServerTransport::handleOpen,this, std::placeholders::_1));
	
	m_server.set_reuse_addr(true);
	m_server.listen(m_port);
	m_server.start_accept();
	
	m_server.run();
}

void WebSocketServerTransport::shutdown() {
	m_server.stop_listening();
}
