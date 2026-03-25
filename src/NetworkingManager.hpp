#pragma once
#include <Geode/Geode.hpp>

class NetworkingManager {
protected:
    static NetworkingManager* s_instance;
    int m_unreadCount = 0;
public:
    static NetworkingManager* get();
    
    void sendMessage(std::string const& message);
    // Removed the 'geode::cocos::' prefix
    void onMessageReceived(std::string const& user, std::string const& message, cocos2d::ccColor3B color);
    
    int getUnreadCount() const { return m_unreadCount; }
    void resetUnreadCount() { m_unreadCount = 0; }
};