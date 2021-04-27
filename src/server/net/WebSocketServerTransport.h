#pragma once

#include <unordered_set>
#include <thread>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "BinaryServerTransport.h"
#include "ClientConnection.h"

class WebSocketServerTransport: public BinaryServerTransport, std::thread {
	typedef websocketpp::server<websocketpp::config::asio> server_t;
	
	class Connection: public BinaryServerTransport::Connection {
		websocketpp::connection_hdl m_connection;
		std::atomic<bool> m_closed = false;
		
		[[nodiscard]] WebSocketServerTransport &webSocketTransport() const {
			return (WebSocketServerTransport&) transport();
		}
		
		friend class WebSocketServerTransport;
		
		void handleWebSocketMessage(server_t::message_ptr message);
		void handleClose();
	
	protected:
		void sendMessage(const void *data, size_t dataSize) override;
	
	public:
		Connection(WebSocketServerTransport &transport, websocketpp::connection_hdl connection);
		~Connection() override;
		
	};
	
	uint16_t m_port;
	server_t m_server;
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
