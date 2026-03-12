#include "CursorIndicator.hpp"
#include "CollabManager.hpp"
#include <Geode/modify/LevelEditorLayer.hpp>

static CursorIndicator* s_cursorIndicator = nullptr;

class $modify(LevelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level) override {
        if (!LevelEditorLayer::init(level)) return false;
        
        // Create cursor indicator
        s_cursorIndicator = CursorIndicator::create();
        s_cursorIndicator->setZOrder(9999); // Render on top
        this->addChild(s_cursorIndicator);
        
        return true;
    }
    
    void update(float dt) override {
        LevelEditorLayer::update(dt);
        
        // Update cursor indicator
        if (s_cursorIndicator) {
            s_cursorIndicator->update(dt);
        }
    }
};
