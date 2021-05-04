#pragma once

#include <shared_mutex>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

template<typename config> class WebSocketAsioConnection: public websocketpp::transport::asio::connection<config> {
	websocketpp::transport::asio::handler_allocator m_writeHandlerWrapperAllocator;
	websocketpp::lib::function<void(const websocketpp::lib::error_code&)> m_writeCompleteHandler;
	std::shared_mutex m_writeCompleteHandlerMutex;
	
	void callWriteCompleteHandler(const websocketpp::lib::error_code errorCode) {
		std::shared_lock<std::shared_mutex> lock(m_writeCompleteHandlerMutex);
		if (m_writeCompleteHandler) {
			m_writeCompleteHandler(errorCode);
		}
	}
	
public:
	typedef WebSocketAsioConnection<config> type;
	typedef websocketpp::lib::shared_ptr<type> ptr;
	
	template<typename ...Args> explicit WebSocketAsioConnection(
			Args&&... args
	): websocketpp::transport::asio::connection<config>(std::forward<Args>(args)...) {
	}
	
	void setWriteCompleteHandler(websocketpp::lib::function<void(const websocketpp::lib::error_code&)> handler) {
		std::unique_lock<std::shared_mutex> lock(m_writeCompleteHandlerMutex);
		m_writeCompleteHandler = handler;
	}
	
	void async_write(
			const char* buf,
			size_t len,
			websocketpp::transport::write_handler handler
	) {
		using namespace websocketpp::transport::asio;
		connection<config>::async_write(
				buf,
				len,
				make_custom_alloc_handler(m_writeHandlerWrapperAllocator, [this, handler](auto &errorCode) {
					callWriteCompleteHandler(errorCode);
					handler(errorCode);
				})
		);
	}
	
	void async_write(
			std::vector<websocketpp::transport::buffer> const &bufs,
			websocketpp::transport::write_handler handler
	) {
		using namespace websocketpp::transport::asio;
		connection<config>::async_write(
				bufs,
				make_custom_alloc_handler(m_writeHandlerWrapperAllocator, [this, handler](auto &errorCode) {
					callWriteCompleteHandler(errorCode);
					handler(errorCode);
				})
		);
	}
	
};

template<typename config> class WebSocketAsioEndpoint: public websocketpp::transport::asio::endpoint<config> {
public:
	typedef WebSocketAsioConnection<config> transport_con_type;
	typedef typename transport_con_type::ptr transport_con_ptr;
};

struct WebSocketConfig: public websocketpp::config::asio {
	typedef WebSocketAsioEndpoint<transport_config> transport_type;
};

typedef websocketpp::server<WebSocketConfig> WebSocketServer;
