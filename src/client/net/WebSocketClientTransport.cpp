#include "WebSocketClientTransport.h"
#include <easylogging++.h>

WebSocketClientTransport::WebSocketClientTransport(
		GameEngine &engine,
		std::string url
): BinaryClientTransport(engine), m_url(std::move(url))
#ifdef __EMSCRIPTEN__
, m_socket(-1)
#endif
{
#ifndef __EMSCRIPTEN__
	m_client.clear_access_channels(websocketpp::log::elevel::all);
	m_client.init_asio();
#endif
}

void WebSocketClientTransport::start() {
#ifndef __EMSCRIPTEN__
	std::error_code errorCode;
	auto connection = m_client.get_connection(m_url, errorCode);
	m_connection = connection->get_handle();
	connection->set_fail_handler([this] (auto connection) { handleError(); });
	connection->set_open_handler([this] (auto connection) { handleOpen(); });
	connection->set_close_handler([this] (auto connection) { handleClose(); });
	connection->set_message_handler([this] (auto connection, client_t::message_ptr message) {
		handleMessage(message->get_payload());
	});
	m_client.connect(connection);
	m_thread = std::thread(&WebSocketClientTransport::run, this);
#else
	EmscriptenWebSocketCreateAttributes ws_attrs = {
			m_url.c_str(),
			nullptr,
			EM_TRUE
    };
	m_socket = emscripten_websocket_new(&ws_attrs);
	emscripten_websocket_set_onerror_callback(
			m_socket,
			this,
			[] (int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData) {
				static_cast<WebSocketClientTransport*>(userData)->handleError();
				return EM_TRUE;
			}
	);
	emscripten_websocket_set_onopen_callback(
			m_socket,
			this,
			[] (int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData) {
				static_cast<WebSocketClientTransport*>(userData)->handleOpen();
				return EM_TRUE;
			}
	);
	emscripten_websocket_set_onclose_callback(
			m_socket,
			this,
			[] (int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData) {
				static_cast<WebSocketClientTransport*>(userData)->handleClose();
				return EM_TRUE;
			}
	);
	emscripten_websocket_set_onmessage_callback(
			m_socket,
			this,
			[] (int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData) {
				static_cast<WebSocketClientTransport*>(userData)->handleMessage(
						std::string((const char*) websocketEvent->data, websocketEvent->numBytes)
				);
				return EM_TRUE;
			}
	);
#endif
}

#ifndef __EMSCRIPTEN__
void WebSocketClientTransport::run() {
	m_client.run();
}
#endif

void WebSocketClientTransport::shutdown() {
#ifndef __EMSCRIPTEN__
	if (m_connected) {
		m_client.close(m_connection, 1001, "CLOSE_GOING_AWAY");
	}
	m_thread.join();
#else
	emscripten_websocket_close(m_socket, 1001, "CLOSE_GOING_AWAY");
	m_socket = -1;
#endif
}

void WebSocketClientTransport::handleError() {
	LOG(ERROR) << "WebSocket error";
	m_connected = false;
}

void WebSocketClientTransport::handleOpen() {
	LOG(INFO) << "WebSocket connected";
	m_connected = true;
	sendHello();
}

void WebSocketClientTransport::handleClose() {
	LOG(INFO) << "WebSocket closed";
	m_connected = false;
}

void WebSocketClientTransport::sendMessage(const void *data, size_t dataSize) {
	if (!m_connected) return;
#ifndef __EMSCRIPTEN__
	std::error_code errorCode;
	m_client.send(m_connection, data, dataSize, websocketpp::frame::opcode::binary, errorCode);
	if (errorCode) {
		LOG(ERROR) << "WebSocket send payload error: " << errorCode.message();
	}
#else
	emscripten_websocket_send_binary(m_socket, (void*) data, dataSize);
#endif
}
