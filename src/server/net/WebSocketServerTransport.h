#pragma once

#include <thread>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "ServerTransport.h"

class WebSocketServerTransport: public ServerTransport, std::thread {
	typedef websocketpp::server<websocketpp::config::asio> server_t;
	
	uint16_t m_port;
	server_t m_server;
	std::thread m_thread;
	
	void run();
	void handleMessage(websocketpp::connection_hdl connection, server_t::message_ptr message);
	
public:
	explicit WebSocketServerTransport(uint16_t port);
	void start() override;
	void shutdown() override;
	
};
