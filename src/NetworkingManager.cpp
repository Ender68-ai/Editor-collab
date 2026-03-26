#include "NetworkingManager.hpp"
#include "ChatLayer.hpp"

using namespace geode::prelude;

NetworkingManager* NetworkingManager::s_instance = nullptr;
std::mutex NetworkingManager::s_instanceMutex;

NetworkingManager* NetworkingManager::get() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) s_instance = new NetworkingManager();
    return s_instance;
}

void NetworkingManager::destroy() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    delete s_instance;
    s_instance = nullptr;
}

void NetworkingManager::sendMessage(std::string const& message) {
    log::info("Network: Sending '{}'", message);
    
    // Simulate immediately receiving the message so you can see it in action
    this->onMessageReceived("You", message, {0, 255, 0});
}

void NetworkingManager::onMessageReceived(std::string const& user, std::string const& message, ccColor3B color) {
    // Ensure UI operations happen on main thread
    Loader::get()->queueInMainThread([this, user, message, color]() {
        auto scene = CCDirector::get()->getRunningScene();
        if (!scene) return;

        auto chat = typeinfo_cast<ChatLayer*>(scene->getChildByIDRecursive("collab-chat-layer"_spr));
        if (chat) {
            chat->addMessage(user, message, color, false);
            
            // Update badge logic if the sidebar is closed
            if (!chat->isOpen()) {
                m_unreadCount++;
                if (auto badge = typeinfo_cast<CCLabelBMFont*>(scene->getChildByIDRecursive("chat-badge"_spr))) {
                    std::string text = m_unreadCount > 99 ? "99+" : std::to_string(m_unreadCount);
                    badge->setString(text.c_str());
                    badge->setVisible(true);
                }
            }
        }
    });
}