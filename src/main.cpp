#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include "CollabManager.hpp"
#include "UI/CollabPanel.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("Collaborative Editor Mod loaded");
    
    // Initialize the collaboration manager
    CollabManager::get();
    
    // Register UI components
    CollabPanel::setup();
}

$on_mod(Unloaded) {
    log::info("Collaborative Editor Mod unloaded");
    CollabManager::get()->disconnect();
}

// Add ownership tracking to GameObject
class $modify(GameObject, GameObject) {
public:
    std::string m_collabOwnerID = "";
    bool m_isCollabSelected = false;
    ccColor3B m_originalColor = {255, 255, 255};
    
    void setCollabOwner(const std::string& ownerID) {
        m_collabOwnerID = ownerID;
        updateCollabColor();
    }
    
    void updateCollabColor() {
        if (m_collabOwnerID.empty()) {
            setColor(m_originalColor);
            return;
        }
        
        auto& manager = CollabManager::get();
        auto color = manager.getPlayerColor(m_collabOwnerID);
        if (color) {
            setColor(*color);
        }
    }
    
    void resetCollabColor() {
        setColor(m_originalColor);
    }
};
