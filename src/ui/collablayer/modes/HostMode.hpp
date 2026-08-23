#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/TableViewCellDelegate.hpp>

using namespace geode::prelude;

class HostLocalLevelList : public CCNode, public TableViewCellDelegate {
protected:
    bool init();

public:
    static HostLocalLevelList* create();

    bool cellPerformedAction(
        TableViewCell* cell,
        int listType,
        CellAction action,
        cocos2d::CCNode* parent
    );

    int getSelectedCellIdx();
    bool shouldSnapToSelected();
    int getCellDelegateType();

};

class RoomCreate {
protected:
    bool init();

public:
    static RoomCreate* create();
};