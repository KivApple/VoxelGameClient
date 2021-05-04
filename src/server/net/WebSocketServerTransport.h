#pragma once

#include <unordered_set>
#include <thread>
#include "WebSocketServerConfig.h"
#include "BinaryServerTransport.h"
#include "ClientConnection.h"

class WebSocketServerTransport: public BinaryServerTransport, std::thread {
	class Connection: public BinaryServerTransport::Connection {
		websocketpp::connection_hdl m_connection;
		std::atomic<bool> m_closed = false;
		bool m_sendingPendingChunks = false;
		
		[[nodiscard]] WebSocketServerTransport &webSocketTransport() const {
			return (WebSocketServerTransport&) transport();
		}
		
		friend class WebSocketServerTransport;
		
		void handleWriteComplete(const websocketpp::lib::error_code &errorCode);
		void handleWebSocketMessage(WebSocketServer::message_ptr message);
		void handleClose();
	
	protected:
		void sendMessage(const void *data, size_t dataSize) override;
		void newPendingChunk() override;
	
	public:
		Connection(WebSocketServerTransport &transport, websocketpp::connection_hdl connection);
		~Connection() override;
		
	};
	
	uint16_t m_port;
	WebSocketServer m_server;
	std::thread m_thread;
	GameServerEngine *m_engine = nullptr;
	
	void run();
	void handleOpen(websocketpp::connection_hdl conn_ptr);
	
public:
	explicit WebSocketServerTransport(uint16_t port);
	~WebSocketServerTransport() override;
	void start(GameServerEngine &engine) override;
	void shutdown() override;
	GameServerEngine *engine() override {
		return m_engine;
	}
	
};
