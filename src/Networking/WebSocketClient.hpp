#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>

using namespace geode::prelude;

// Simplified WebSocket client for now
class WebSocketClient {
public:
    static WebSocketClient* get();
    
    bool connect(const std::string& url);
    void disconnect();
    bool isConnected() const;
    void send(const std::string& message);
    
    void setMessageHandler(std::function<void(const std::string&)> handler);
    void setConnectionHandler(std::function<void()> handler);
    void setCloseHandler(std::function<void()> handler);
    
private:
    WebSocketClient();
    ~WebSocketClient();
    
    static WebSocketClient* s_instance;
    
    bool m_isConnected;
    std::function<void(const std::string&)> m_messageHandler;
    std::function<void()> m_connectionHandler;
    std::function<void()> m_closeHandler;
};
