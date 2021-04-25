#pragma once

#include <mutex>
#include <string>
#ifndef __EMSCRIPTEN__
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#else
#include <emscripten/websocket.h>
#endif
#include "BinaryClientTransport.h"

class WebSocketClientTransport: public BinaryClientTransport {
#ifndef __EMSCRIPTEN__
	typedef websocketpp::client<websocketpp::config::asio_client> client_t;
	
	client_t m_client;
	websocketpp::connection_hdl m_connection;
	std::thread m_thread;
	
	void run();
	
#else
	EMSCRIPTEN_WEBSOCKET_T m_socket;
	
#endif
	std::string m_url;
	std::atomic<bool> m_connected = false;
	
	void handleOpen();
	void handleError();
	void handleClose();
	void sendMessage(const void *data, size_t dataSize) override;
	
public:
	explicit WebSocketClientTransport(std::string url);
	void start() override;
	void shutdown() override;
	bool isConnected() override {
		return m_connected;
	}
	
};
