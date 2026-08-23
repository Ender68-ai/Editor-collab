#include <Geode/binding/LevelCell.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/loader/Log.hpp>

#include "../ui.hpp"


class $modify(MyLevelCell, LevelCell) {
    void onClick(CCObject* sender) {
    auto scene = CCDirector::sharedDirector()->getRunningScene();

    if (scene->getChildByID("ender68.collabeditor/collab-layer")) {
        fromCollab = true;
    } else {
        fromCollab = false;
    }

    LevelCell::onClick(sender);
}
};


