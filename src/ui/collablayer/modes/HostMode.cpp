#include "HostMode.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/binding/CustomListView.hpp>


// arch

/*
    CollabLayer:
    GJListLayer:
         CustomListView : BoomListView
            
        HostLocalLevelList : CCNode + TableViewCellDelegate

*/


using namespace geode::prelude;

HostLocalLevelList* HostLocalLevelList::create() {
    auto ret = new HostLocalLevelList();

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool HostLocalLevelList::cellPerformedAction(
    TableViewCell* cell,
    int listType,
    CellAction action,
    cocos2d::CCNode* parent
) {
    if (action == CellAction::Click) {
        // TODO
        return true;
    }

    return false;
}

int HostLocalLevelList::getSelectedCellIdx() {
    return -1;
}

bool HostLocalLevelList::shouldSnapToSelected() {
    return false;
}

int HostLocalLevelList::getCellDelegateType() {
    return 0;
}

bool HostLocalLevelList::init() {
    if (!CCNode::init()) {
        return false;
    }

    auto levels = LocalLevelManager::sharedState()->m_localLevels;

    auto list = CustomListView::create(
        levels,
        this,
        200.f,
        200.f,
        0,
        BoomListType::Level,
        0.f
    );

    if (!list) {
        return false;
    }
    
    return true;
}