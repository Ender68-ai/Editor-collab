#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "CollabManager.hpp"
#include "CollabPopups.hpp"

using namespace geode::prelude;

class $modify(CollabLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        // Add collab icon next to level name
        auto levelName = this->getChildByIDRecursive("level-name-label");
        if (levelName) {
            auto collabBtn = CCMenuItemSpriteExtra::create(
                CCSprite::create("logo.png"_spr),
                this,
                menu_selector(CollabLevelInfoLayer::onCollabButton)
            );
            collabBtn->setID("collab-button"_spr);
            collabBtn->setScale(0.5f);
            
            // Position to the right of the level name
            auto pos = levelName->getPosition();
            auto size = levelName->getScaledContentSize();
            collabBtn->setPosition({pos.x + size.width / 2 + 20.f, pos.y});
            
            auto menu = CCMenu::create();
            menu->addChild(collabBtn);
            menu->setPosition({0.f, 0.f});
            this->addChild(menu);
        }

        return true;
    }

    void onCollabButton(CCObject* sender) {
        auto level = m_level;
        if (!level) return;

        auto collabManager = CollabManager::get();
        auto accountID = GJAccountManager::get()->m_accountID;
        int levelID = level->m_levelID;
        std::string levelName = level->m_levelName;

        if (collabManager->isFirstTimeSetup()) {
            // First time setup - show the Collab? popup
            auto popup = CollabSetupPopup::create(levelName, levelID);
            popup->show();
        } else if (collabManager->isCollabEnabled(levelID)) {
            // Show management menu
            if (collabManager->isHostOfLevel(levelID, accountID)) {
                auto popup = CollabManagementPopup::create(levelID);
                popup->show();
            } else {
                FLAlertLayer::create("Access Denied", "Only the host can manage collaboration.", "OK")->show();
            }
        } else {
            // Collab is not enabled for this level
            FLAlertLayer::create("Collab Disabled", "Collaboration is not enabled for this level.", "OK")->show();
        }
    }
};

// Right-click context menu for transfer
class $modify(CollabLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        
        // Store level info for context menu
        this->setUserObject(level);
    }

    void onRightClick(CCObject* sender) {
        auto level = static_cast<GJGameLevel*>(this->getUserObject());
        if (!level) return;

        auto collabManager = CollabManager::get();
        auto accountID = GJAccountManager::get()->m_accountID;
        int levelID = level->m_levelID;

        if (collabManager->isCollabEnabled(levelID) && collabManager->isHostOfLevel(levelID, accountID)) {
            // Show transfer context menu
            geode::createQuickPopup(
                "Level Options",
                "What would you like to do with this level?",
                "Cancel", "Transfer",
                [this, level](auto, bool btn2) {
                    if (btn2) {
                        // Show transfer confirmation
                        auto popup = TransferConfirmPopup::create(
                            level->m_levelID,
                            0, // TODO: Get target account ID from selection
                            level->m_levelName
                        );
                        popup->show();
                    }
                }
            );
        }
        // Removed the call to LevelCell::onRightClick(sender) as it's not accessible
    }
};

// My Levels tab integration
class $modify(CollabMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Add collab button to My Levels
        auto myLevelsBtn = this->getChildByIDRecursive("my-levels-button");
        if (myLevelsBtn) {
            auto collabIcon = CCSprite::create("logo.png"_spr);
            if (collabIcon) {
                collabIcon->setScale(0.3f);
                collabIcon->setPosition({myLevelsBtn->getScaledContentSize().width - 10.f, myLevelsBtn->getScaledContentSize().height - 10.f});
                collabIcon->setID("collab-icon"_spr);
                myLevelsBtn->addChild(collabIcon);
            }
        }

        return true;
    }
};
