#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "core/CollabNetworkManager.hpp"
#include "CollabManager.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/ButtonSprite.hpp>

using namespace geode::prelude;

class $modify(CollabEditLevelLayer, LevelEditorLayer) {
    struct Fields {
        std::map<uint32_t, CCSprite*> m_remoteCursorSprites;
    };

    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) return false;

        log::debug("EditLevelLayer::init called - initializing CollabEditor button");

        // Find the level-edit-menu
        auto levelEditMenu = this->getChildByIDRecursive("level-edit-menu");
        if (!levelEditMenu) {
            log::error("Could not find level-edit-menu");
            return true;
        }

        auto menu = typeinfo_cast<CCMenu*>(levelEditMenu);
        if (!menu) {
            log::error("level-edit-menu is not a CCMenu");
            return true;
        }

        // Create the logo sprite and scale it to fit inside a 40x40 button
        auto logo = CCSprite::create("button.png"_spr);
        if (!logo) {
            logo = CCSprite::createWithSpriteFrameName("edit_cursorBtn_001.png");
        }
        if (!logo) {
            log::error("Failed to load collab logo sprite");
            return true;
        }

        // Force scale so logo fits inside standard 40x40 button
        logo->setScale(25.f / logo->getContentSize().width);

        // Arguments: (topSprite, width, absoluteWidth, height, texture, scale)
        auto btnSprite = ButtonSprite::create(
            logo, 
            0, 
            false, 
            0.f, 
            "GJ_button_01.png"_spr, 
            0.6f
        );

        if (!btnSprite) {
            log::error("Failed to create ButtonSprite for collab button");
            CC_SAFE_DELETE(logo);
            return true;
        }

        auto collabBtn = CCMenuItemSpriteExtra::create(
            btnSprite, 
            this, 
            menu_selector(CollabEditLevelLayer::onCollabSettings)
        );
        
        if (!collabBtn) {
            log::error("Failed to create CCMenuItemSpriteExtra for collab button");
            return true;
        }

        // Set scale to 0.5f (fixed from 1.0f as requested)
        collabBtn->setScale(0.5f);

        collabBtn->setID("collab-button");
        log::debug("Collab button created successfully");

        // Add to menu
        menu->addChild(collabBtn);
        
        // CRITICAL: Call updateLayout to space it correctly as the 4th button
        menu->updateLayout();
        
        log::info("CollabEditor button added to level-edit-menu");
        
        // Initialize remote cursor map and start heartbeat
        m_fields->m_remoteCursorSprites.clear();
        this->schedule(schedule_selector(CollabEditLevelLayer::updateHeartbeat), 0.05f); // 50ms heartbeat
        this->schedule(schedule_selector(CollabEditLevelLayer::updateRemoteCursors), 0.05f);
        
        return true;
    }

    void onCollabSettings(CCObject* sender) {
        log::debug("onCollabSettings called from EditLevelLayer");
        
        if (!m_level) {
            log::error("Level is null in onCollabSettings - cannot create popup");
            return;
        }

        log::debug("Creating CollabSettingsPopup for level {} ({})", m_level->m_levelID, m_level->m_levelName);
        
        // Placeholder functionality for now
        FLAlertLayer::create("Collab Settings", "Collab settings functionality coming soon!", "OK")->show();
    }

    void updateHeartbeat(float dt) {
        auto netMgr = CollabNetworkManager::get();
        
        // Only send heartbeat if connected
        if (!netMgr->isTCPConnected() || !netMgr->isUDPConnected()) {
            return;
        }
        
        // Get current mouse/editor position
        auto mousePos = CCDirector::get()->getOpenGLView()->getMousePosition();
        
        // Convert screen coordinates to editor space
        auto realPos = ((LevelEditorLayer*)this)->m_objectLayer->convertToNodeSpace(mousePos);
        
        // Use converted editor coordinates
        float x = realPos.x;
        float y = realPos.y;
        float rotation = 0.0f; // Can be extended to track editor camera rotation
        
        // Update the network manager with current cursor position
        // This will handle throttling internally
        netMgr->updateLocalCursor(x, y, rotation);
    }

    void updateRemoteCursors(float dt) {
        auto netMgr = CollabNetworkManager::get();
        
        // Process any incoming network updates
        netMgr->processNetworkUpdates();
        
        // Get all remote cursors
        auto remoteCursors = netMgr->getRemoteCursors();
        
        // Update or create cursor sprites
        for (const auto& [accountID, cursor] : remoteCursors) {
            auto& spritePtr = m_fields->m_remoteCursorSprites[accountID];
            
            if (!spritePtr) {
                // Create new remote cursor sprite
                spritePtr = CCSprite::createWithSpriteFrameName("edit_cursorBtn_001.png");
                if (spritePtr) {
                    spritePtr->setScale(0.3f);
                    spritePtr->setOpacity(180);
                    spritePtr->setColor({100, 150, 255});
                    this->addChild(spritePtr, 1);
                    log::debug("Created remote cursor sprite for account {}", accountID);
                }
            }
            
            if (spritePtr) {
                spritePtr->setPosition({cursor.x, cursor.y});
                spritePtr->setRotation(cursor.rotation);
            }
        }
        
        // Remove cursors that are no longer active
        std::vector<uint32_t> toRemove;
        for (const auto& [accountID, spritePtr] : m_fields->m_remoteCursorSprites) {
            if (remoteCursors.find(accountID) == remoteCursors.end()) {
                if (spritePtr) {
                    spritePtr->removeFromParent();
                }
                toRemove.push_back(accountID);
            }
        }
        
        for (uint32_t accountID : toRemove) {
            m_fields->m_remoteCursorSprites.erase(accountID);
            log::debug("Removed remote cursor sprite for account {}", accountID);
        }
    }
};
