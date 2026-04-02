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

        // Add the collab settings button in the bottom right area
        // Position it above the existing buttons
        
        auto collabBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_shareBtn_001.png"),
            this,
            menu_selector(CollabLevelInfoLayerExt::onCollabSettings)
        );
        
        if (!collabBtn) {
            log::warn("Failed to load GJ_shareBtn_001.png for collab button");
            collabBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("Collab", "bigFont.fnt", "GJ_button_01.png", 0.7f),
                this,
                menu_selector(CollabLevelInfoLayerExt::onCollabSettings)
            );
        }
        
        collabBtn->setScale(0.8f);
        collabBtn->setID("collab-settings-btn"_spr);
        
        // Find or create the info menu where buttons are
        CCMenu* targetMenu = nullptr;
        
        // Try to find existing menus
        for (int i = 0; i < this->getChildren()->count(); i++) {
            auto child = static_cast<CCNode*>(this->getChildren()->objectAtIndex(i));
            if (auto menu = typeinfo_cast<CCMenu*>(child)) {
                // Check if this menu contains GJ buttons
                if (menu->getChildByID("info-menu"_spr) || menu->getChildByID("button-menu"_spr)) {
                    targetMenu = menu;
                    break;
                }
            }
        }
        
        if (!targetMenu) {
            // Create a new menu for the collab button
            targetMenu = CCMenu::create();
            targetMenu->setPosition({0.f, 0.f});
            this->addChild(targetMenu, 10);
        }
        
        // Position button in the bottom right area, above other buttons
        // Adjust based on where other buttons are
        collabBtn->setPosition({260.f, 55.f});
        targetMenu->addChild(collabBtn);
        
        log::info("Added CollabSettings button to LevelInfoLayer");
        
        return true;
    }

    void onCollabSettings(CCObject* sender) {
        auto level = m_level;
        if (!level) {
            log::error("Level is null in onCollabSettings");
            return;
        }

        auto popup = CollabSettingsPopup::create(level->m_levelID, level->m_levelName);
        if (popup) {
            popup->show();
        } else {
            log::error("Failed to create CollabSettingsPopup");
        }
    }
};
