#pragma once
#include <Geode/Geode.hpp>
#include <mutex>

class NetworkingManager {
protected:
    static NetworkingManager* s_instance;
    static std::mutex s_instanceMutex;
    int m_unreadCount = 0;
    
    NetworkingManager() = default;
    ~NetworkingManager() = default;
    
public:
    static NetworkingManager* get();
    static void destroy();
    
    void sendMessage(std::string const& message);
    void onMessageReceived(std::string const& user, std::string const& message, cocos2d::ccColor3B color);
    
    int getUnreadCount() const { return m_unreadCount; }
    void resetUnreadCount() { m_unreadCount = 0; }
};