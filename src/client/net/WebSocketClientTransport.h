#pragma once

#include <mutex>
#include <string>
#ifndef __EMSCRIPTEN__
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#else
#include <emscripten/websocket.h>
#endif
#include "ClientTransport.h"

class WebSocketClientTransport: public ClientTransport {
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
	glm::vec3 m_playerPosition = glm::vec3(0.0f);
	float m_playerYaw = 0.0f;
	float m_playerPitch = 0.0f;
	bool m_playerPositionValid = false;
	std::mutex m_playerPositionMutex;
	std::atomic<bool> m_connected = false;
	
	void handleOpen();
	void handleError();
	void handleClose();
	void handleMessage(const char *data, size_t dataSize);
	void sendMessage(const void *data, size_t dataSize);
	void sendPlayerPosition();
	
public:
	explicit WebSocketClientTransport(std::string url);
	void start() override;
	void shutdown() override;
	void sendPlayerPosition(const glm::vec3 &position, float yaw, float pitch) override;
	
};
