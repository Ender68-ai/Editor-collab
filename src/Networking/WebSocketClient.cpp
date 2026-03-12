#include "WebSocketClient.hpp"

WebSocketClient* WebSocketClient::s_instance = nullptr;

WebSocketClient* WebSocketClient::get() {
    if (!s_instance) {
        s_instance = new WebSocketClient();
    }
    return s_instance;
}

WebSocketClient::WebSocketClient() : m_isConnected(false) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect(const std::string& url) {
    // Simplified implementation - just mark as connected for now
    m_isConnected = true;
    if (m_connectionHandler) {
        m_connectionHandler();
    }
    return true;
}

void WebSocketClient::disconnect() {
    m_isConnected = false;
    if (m_closeHandler) {
        m_closeHandler();
    }
}

bool WebSocketClient::isConnected() const {
    return m_isConnected;
}

void WebSocketClient::send(const std::string& message) {
    // Simplified implementation - just log for now
    log::info("WebSocket send: {}", message);
}

void WebSocketClient::setMessageHandler(std::function<void(const std::string&)> handler) {
    m_messageHandler = handler;
}

void WebSocketClient::setConnectionHandler(std::function<void()> handler) {
    m_connectionHandler = handler;
}

void WebSocketClient::setCloseHandler(std::function<void()> handler) {
    m_closeHandler = handler;
}

void WebSocketClient::onOpen(websocketpp::connection_hdl hdl) {
    m_connection = hdl;
    m_isConnected = true;
    
    if (m_connectionHandler) {
        m_connectionHandler();
    }
}

void WebSocketClient::onClose(websocketpp::connection_hdl hdl) {
    m_isConnected = false;
    
    if (m_closeHandler) {
        m_closeHandler();
    }
}

void WebSocketClient::onMessage(websocketpp::connection_hdl hdl, 
    websocketpp::client<websocketpp::config::asio_client>::message_ptr msg) {
    
    if (m_messageHandler) {
        m_messageHandler(msg->get_payload());
    }
}

void WebSocketClient::networkThreadLoop() {
    try {
        m_client.run();
    } catch (const std::exception&) {
        // Handle network thread error
    }
}
