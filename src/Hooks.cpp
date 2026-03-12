#include "Hooks.hpp"
#include "CollabManager.hpp"
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EditorUI.hpp>

bool Hooks::s_ignoreNext = false;

bool Hooks::shouldIgnoreAction() {
    return s_ignoreNext;
}

void Hooks::setIgnoreNext(bool ignore) {
    s_ignoreNext = ignore;
}

void Hooks::handleAddObject(const json& data) {
    setIgnoreNext(true);
    
    auto editor = LevelEditorLayer::get();
    if (!editor) return;
    
    int objectID = data["objectID"];
    float x = data["x"];
    float y = data["y"];
    float rotation = data.value("rotation", 0.0f);
    float scaleX = data.value("scaleX", 1.0f);
    float scaleY = data.value("scaleY", 1.0f);
    std::string ownerID = data.value("ownerID", "");
    
    // Find or create the object
    auto obj = editor->getObjectFromID(objectID);
    if (!obj) {
        // Create new object (simplified - you'd need proper object creation logic)
        obj = GameObject::create();
        obj->m_objectID = objectID;
        obj->setPosition({x, y});
        obj->setRotation(rotation);
        obj->setScaleX(scaleX);
        obj->setScaleY(scaleY);
        editor->addObject(obj);
    }
    
    if (!ownerID.empty()) {
        obj->setCollabOwner(ownerID);
    }
    
    setIgnoreNext(false);
}

void Hooks::handleRemoveObject(const json& data) {
    setIgnoreNext(true);
    
    auto editor = LevelEditorLayer::get();
    if (!editor) return;
    
    int objectID = data["objectID"];
    auto obj = editor->getObjectFromID(objectID);
    if (obj) {
        editor->removeObject(obj);
    }
    
    setIgnoreNext(false);
}

void Hooks::handleMoveObject(const json& data) {
    setIgnoreNext(true);
    
    auto editor = LevelEditorLayer::get();
    if (!editor) return;
    
    int objectID = data["objectID"];
    float x = data["x"];
    float y = data["y"];
    float rotation = data.value("rotation", 0.0f);
    float scaleX = data.value("scaleX", 1.0f);
    float scaleY = data.value("scaleY", 1.0f);
    std::string ownerID = data.value("ownerID", "");
    
    auto obj = editor->getObjectFromID(objectID);
    if (obj) {
        obj->setPosition({x, y});
        obj->setRotation(rotation);
        obj->setScaleX(scaleX);
        obj->setScaleY(scaleY);
        
        if (!ownerID.empty()) {
            obj->setCollabOwner(ownerID);
        }
    }
    
    setIgnoreNext(false);
}

void Hooks::handleClaimObject(const json& data) {
    auto editor = LevelEditorLayer::get();
    if (!editor) return;
    
    int objectID = data["objectID"];
    std::string ownerID = data["ownerID"];
    
    auto obj = editor->getObjectFromID(objectID);
    if (obj) {
        obj->setCollabOwner(ownerID);
    }
}

void Hooks::handleTransferOwnership(const json& data) {
    auto editor = LevelEditorLayer::get();
    if (!editor) return;
    
    int objectID = data["objectID"];
    std::string toPlayerID = data["toPlayerID"];
    
    auto obj = editor->getObjectFromID(objectID);
    if (obj) {
        obj->setCollabOwner(toPlayerID);
    }
}

// Hook into LevelEditorLayer to process network packets
class $modify(LevelEditorLayer, LevelEditorLayer) {
    void update(float dt) override {
        LevelEditorLayer::update(dt);
        
        // Process network packets on main thread
        CollabManager::get()->processQueue();
        
        // Send cursor updates
        auto director = CCDirector::sharedDirector();
        if (director) {
            auto winSize = director->getWinSize();
            auto mousePos = director->getMousePosition();
            CollabManager::get()->sendCursorUpdate(mousePos);
        }
    }
};

// Hook into object addition
class $modify(EditorUI, EditorUI) {
    void addObject(GameObject* obj) {
        if (Hooks::shouldIgnoreAction()) {
            EditorUI::addObject(obj);
            return;
        }
        
        EditorUI::addObject(obj);
        
        // Claim ownership and broadcast
        CollabManager::get()->claimObject(obj);
    }
    
    void removeObject(GameObject* obj) {
        if (Hooks::shouldIgnoreAction()) {
            EditorUI::removeObject(obj);
            return;
        }
        
        // Broadcast removal
        CollabManager::get()->sendObjectAction("REMOVE_OBJECT", obj);
        
        EditorUI::removeObject(obj);
    }
    
    void moveObject(GameObject* obj, CCPoint pos) {
        if (Hooks::shouldIgnoreAction()) {
            EditorUI::moveObject(obj, pos);
            return;
        }
        
        EditorUI::moveObject(obj, pos);
        
        // Broadcast movement
        CollabManager::get()->sendObjectAction("MOVE_OBJECT", obj);
    }
};
