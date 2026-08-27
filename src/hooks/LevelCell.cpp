#include <Geode/binding/LevelCell.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/loader/Log.hpp>

#include "../ui/Ui.hpp"

using namespace geode::prelude;

class $modify(MPLevelCell, LevelCell) {

    void onClick(CCObject* sender) {

        auto scene = CCDirector::sharedDirector()->getRunningScene();

        bool fromCollab = scene->getChildByID(
            "ender68.multiplayeredit/collab-layer"
        ) != nullptr;

        if(fromCollab) {
            // hook to add to other list
        }
        else {
        LevelCell::onClick(sender);
        }
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);


        this->schedule(
                schedule_selector(MPLevelCell::updateCollabButtons),
                0.5f
            );      
    }

    void updateCollabButtons(float dt) {

    auto scene = CCDirector::sharedDirector()->getRunningScene();

    bool isCollabScene =
        scene->getChildByID("ender68.multiplayeredit/collab-layer") != nullptr;


    if (!isCollabScene) {
        return;
    }

    auto mainLayer = this->getChildByID("main-layer");

        if (!mainLayer) {
            log::debug("NO MAIN LAYER");
            return;
        }

    auto mainMenu = mainLayer->getChildByID("main-menu");

        if (!mainMenu) {
            log::debug("NO MAIN MENU");
            return;
        }

        if (auto infoIcon = mainLayer->getChildByID("info-icon")) {
            infoIcon->setVisible(false);
        }
        if (auto infoLabel = mainLayer->getChildByID("info-label")) {
            infoLabel->setVisible(false);
        }
        if (auto toggler = mainMenu->getChildByID("select-toggler")) {
            toggler->setVisible(false);
        }
        if (auto revision = mainLayer->getChildByID("level-revision")) {
            revision->setVisible(false);
        }

    auto view = mainMenu->getChildByID("view-button");

    auto viewButton = typeinfo_cast<CCMenuItemSpriteExtra*>(view);
        if (!viewButton) {
            log::debug("NO VIEW BUTTON");
            return;
        }

    auto btnSprite = typeinfo_cast<ButtonSprite*>(viewButton->getNormalImage());
        if(!btnSprite)
            return;

        btnSprite->m_label->setString("Host");
        btnSprite->m_BGSprite->setColor({30, 128, 255});  
        
    }
};