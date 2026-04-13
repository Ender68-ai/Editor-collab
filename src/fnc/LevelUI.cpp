#include <Geode/Geode.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/binding/EditLevelLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include "fnc/CollabRoomPopup.hpp"

using namespace geode::prelude;

/**
 * CollabEditLevelLayer
 * * Injects the "Collab" button into the EditLevelLayer (the screen where you 
 * see the "Play", "Edit", and "Share" buttons).
 */
class $modify(CollabEditLevelLayer, EditLevelLayer) {
    
    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) {
            return false;
        }

        // Only show the collaboration button on local editor levels.
        // This prevents the button from showing on downloaded online levels.
        if (level->m_levelType != GJLevelType::Editor) {
            return true;
        }

        // 1. Attempt to find the actions menu by its Node ID (standard in Geode/Node IDs mod)
        auto menu = this->getChildByID("level-actions-menu");

        // 2. Fallback: If IDs aren't present, iterate through children to find the right-side CCMenu
        if (!menu) {
            for (auto child : CCArrayExt<CCNode*>(this->getChildren())) {
                if (auto m = typeinfo_cast<CCMenu*>(child)) {
                    // In standard GD layout, the actions menu is positioned on the right half
                    if (m->getPositionX() > 400) { 
                        menu = m;
                        break;
                    }
                }
            }
        }

        if (menu) {
            // Create the button sprite using the mod's specific resource
            auto sprite = CCSprite::create("button.png"_spr);
            
            // Fallback to a default GD button sprite if the custom one fails to load
            if (!sprite) {
                log::warn("Resource 'button.png' not found, using GJ_button_01.png fallback.");
                sprite = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
            }
            
            if (sprite) {
                sprite->setScale(0.7f);
                
                auto btn = CCMenuItemSpriteExtra::create(
                    sprite, 
                    this, 
                    menu_selector(CollabEditLevelLayer::onCollabButton)
                );
                
                btn->setID("collab-button");
                menu->addChild(btn);
                
                // Force the menu to recalculate the layout of its children
                menu->updateLayout();
            }
        }

        return true;
    }

    /**
     * Callback for the Collab button.
     * Initializes and shows the Geode-style CollabRoomPopup.
     */
    void onCollabButton(CCObject* sender) {
        // Create the popup with a placeholder or initial room code
        auto popup = CollabRoomPopup::create("NONE");
        
        if (popup) {
            // Critical: Set m_scene to 'this' so the popup attaches to the current 
            // layer/scene context, ensuring visibility.
            popup->m_scene = this;
            popup->show();
        } else {
            log::error("Failed to create CollabRoomPopup.");
        }
    }
};