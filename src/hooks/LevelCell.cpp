#include <Geode/binding/LevelCell.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/loader/Log.hpp>

#include "../ui/ui.hpp"

using namespace geode::prelude;

class $modify(MyLevelCell, LevelCell) {

    void onClick(CCObject* sender) {

        auto scene = CCDirector::sharedDirector()->getRunningScene();

        bool collabLayer = scene->getChildByID(
            "d050.mpedit/collablayer"
        ) != nullptr;

        fromCollab = collabLayer;

        LevelCell::onClick(sender);

        if (collabLayer) {
            this->schedule(
                schedule_selector(MyLevelCell::updateCollabButtons),
                0.5f
            );
        }
    }

    void loadFromLevel(GJGameLevel* level) {

        LevelCell::loadFromLevel(level);

        auto scene = CCDirector::sharedDirector()->getRunningScene();

        bool collabLayer = scene->getChildByID(
            "d050.multiplayeredit/collab-layer"
        ) != nullptr;

        if (collabLayer) {
            this->schedule(
                schedule_selector(MyLevelCell::updateCollabButtons),
                0.5f
            );
        }
    }

    void updateCollabButtons(float dt) {

        auto mainLayer = this->getChildByID("main-layer");
        if (!mainLayer)
            return;

        auto mainMenu = mainLayer->getChildByID("main-menu");
        if (!mainMenu)
            return;

        if (auto select = mainMenu->getChildByID("select-toggler")) {
            select->setVisible(false);
        }

        if (auto view = mainMenu->getChildByID("view-button")) {
            log::debug("view button found, hiding");
            view->setVisible(false);
        }
    }
};