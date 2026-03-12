#include "CollabManager.hpp"
#include "Hooks.hpp"
#include <Geode/modify/LevelEditorLayer.hpp>
#include <nlohmann/json.hpp>

using namespace geode::prelude;
using json = nlohmann::json;

class ResyncManager {
public:
    static void requestFullSync();
    static void sendFullSync();
    static void processSyncData(const json& data);
    
private:
    static json serializeLevel();
    static void deserializeLevel(const json& data);
};

void ResyncManager::requestFullSync() {
    if (!CollabManager::get()->isConnected()) return;
    
    json packet = {
        {"type", "REQUEST_SYNC"},
        {"playerID", CollabManager::get()->getLocalPlayerID()}
    };
    
    CollabManager::get()->send(packet);
}

void ResyncManager::sendFullSync() {
    if (!CollabManager::get()->isConnected()) return;
    
    json packet = {
        {"type", "FULL_SYNC"},
        {"playerID", CollabManager::get()->getLocalPlayerID()},
        {"levelData", serializeLevel()}
    };
    
    CollabManager::get()->send(packet);
}

void ResyncManager::processSyncData(const json& data) {
    Hooks::setIgnoreNext(true);
    deserializeLevel(data);
    Hooks::setIgnoreNext(false);
}

json ResyncManager::serializeLevel() {
    auto editor = LevelEditorLayer::get();
    if (!editor) return json{};
    
    json levelData;
    levelData["objects"] = json::array();
    
    auto& objects = editor->m_objects;
    for (auto* obj : objects) {
        if (!obj) continue;
        
        json objData = {
            {"id", obj->m_objectID},
            {"x", obj->getPositionX()},
            {"y", obj->getPositionY()},
            {"rotation", obj->getRotation()},
            {"scaleX", obj->getScaleX()},
            {"scaleY", obj->getScaleY()},
            {"ownerID", obj->m_collabOwnerID}
        };
        
        levelData["objects"].push_back(objData);
    }
    
    return levelData;
}

void ResyncManager::deserializeLevel(const json& data) {
    auto editor = LevelEditorLayer::get();
    if (!editor) return;
    
    // Clear existing objects
    Hooks::setIgnoreNext(true);
    for (auto* obj : editor->m_objects) {
        if (obj) {
            editor->removeObject(obj);
        }
    }
    Hooks::setIgnoreNext(false);
    
    // Add objects from sync data
    if (data.contains("objects")) {
        for (const auto& objData : data["objects"]) {
            Hooks::handleAddObject(objData);
        }
    }
}

// Hook to handle sync requests
class $modify(LevelEditorLayer, LevelEditorLayer) {
    void onSyncRequest() {
        ResyncManager::sendFullSync();
    }
};
