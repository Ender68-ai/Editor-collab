#include <Geode/Geode.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/binding/EditLevelLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>

using namespace geode::prelude;

class $modify(CollabEditLevelLayer, EditLevelLayer) {
    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) return false;

        log::info("EditLevelLayer::init called!");
        
        // Before the m_levelType check, add logging
        log::info("Level Type: {}", (int)level->m_levelType);

        // Only show button on local levels (your own created levels)
        if (level->m_levelType != GJLevelType::Editor) return true;

        // 1. Try to find by ID
        auto menu = this->getChildByID("level-actions-menu");

        // 2. Fallback: If ID isn't assigned yet, find CCMenu on the right
        if (!menu) {
            for (auto child : CCArrayExt<CCNode*>(this->getChildren())) {
                if (auto m = typeinfo_cast<CCMenu*>(child)) {
                    // The level-actions-menu is usually around X = 400+ on standard resolution
                    if (m->getPositionX() > 400) { 
                        menu = m;
                        break;
                    }
                }
            }
        }

        if (menu) {
            // Create button using "button.png" from your resources
            auto sprite = CCSprite::create("button.png"_spr);
            if (!sprite) {
                log::warn("Failed to load button.png, using fallback");
                sprite = CCSprite::createWithSpriteFrameName("GJ_button_01.png"_spr);
            }
            
            if (sprite) {
                sprite->setScale(0.7f);
                auto btn = CCMenuItemSpriteExtra::create(
                    sprite, this, menu_selector(CollabEditLevelLayer::onCollabButton)
                );
                btn->setID("collab-button");
                menu->addChild(btn);
                menu->updateLayout();
                log::debug("Added Collab button to menu");
            }
        }

        return true;
    }

    void onCollabButton(CCObject* sender) {
        log::info("Collab button clicked - placeholder functionality");
    }
};
