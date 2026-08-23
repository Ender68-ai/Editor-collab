#include <Geode/binding/LevelCell.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/loader/Log.hpp>
#include "../ui/ui.hpp"

class $modify(MyLevelCell, LevelCell) {
    void onClick(CCObject* sender) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();

        if (scene->getChildByID("ender68.collabeditor/collab-layer")) {
            fromCollab = true;
        } else {
            fromCollab = false;
        }

        LevelCell::onClick(sender);

        this->updateCollabButtons();
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        this->updateCollabButtons();
    }

    void updateCollabButtons() {
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
            view->setVisible(false);
        }
    }
};