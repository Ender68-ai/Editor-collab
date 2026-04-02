#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include "CollabManager.hpp"
#include "CollabPopups.hpp"

using namespace geode::prelude;

class $modify(CollabLevelInfoLayerExt, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        log::debug("LevelInfoLayer::init called - initializing CollabEditor button");

        // Safety: Create button sprite safely
        CCSprite* btnSprite = CCSprite::createWithSpriteFrameName("GJ_shareBtn_001.png");
        if (!btnSprite) {
            log::warn("Failed to load GJ_shareBtn_001.png, using fallback button");
            btnSprite = CCSprite::create("unknown.png");
            if (!btnSprite) {
                log::error("Could not create button sprite at all - skipping collab button");
                return true;
            }
        }

        // Create button with safe sprite
        auto collabBtn = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(CollabLevelInfoLayerExt::onCollabSettings)
        );
        
        if (!collabBtn) {
            log::error("Failed to create CCMenuItemSpriteExtra for collab button");
            return true;
        }

        collabBtn->setScale(0.8f);
        collabBtn->setID("collab-settings-btn"_spr);
        log::debug("Collab button created successfully");

        // Find the bottom-right menu safely
        CCMenu* targetMenu = nullptr;
        
        // Check if we can find the main layer's menu
        auto children = this->getChildren();
        if (children) {
            for (auto obj : CCArrayExt<CCObject*>(children)) {
                if (auto menu = typeinfo_cast<CCMenu*>(obj)) {
                    // Found a menu - use it
                    targetMenu = menu;
                    log::debug("Found existing menu for collab button placement");
                    break;
                }
            }
        }
        
        // If no menu found, create one
        if (!targetMenu) {
            log::debug("No existing menu found, creating new menu for collab button");
            targetMenu = CCMenu::create();
            if (!targetMenu) {
                log::error("Failed to create CCMenu for collab button");
                return true;
            }
            targetMenu->setPosition({0.f, 0.f});
            this->addChild(targetMenu, 10);
            log::debug("Created new menu at z-order 10");
        }
        
        // Position button with HARDCODED coordinates (safe fallback)
        // Bottom right area of the screen
        collabBtn->setPosition({260.f, 55.f});
        targetMenu->addChild(collabBtn);
        
        log::info("CollabEditor button added to LevelInfoLayer at position (260, 55)");
        
        return true;
    }

    void onCollabSettings(CCObject* sender) {
        log::debug("onCollabSettings called");
        
        auto level = m_level;
        if (!level) {
            log::error("Level is null in onCollabSettings - cannot create popup");
            return;
        }

        log::debug("Creating CollabSettingsPopup for level {} ({})", level->m_levelID, level->m_levelName);
        
        auto popup = CollabSettingsPopup::create(level->m_levelID, level->m_levelName);
        if (popup) {
            log::debug("CollabSettingsPopup created successfully, showing...");
            popup->show();
        } else {
            log::error("Failed to create CollabSettingsPopup");
        }
    }
};
