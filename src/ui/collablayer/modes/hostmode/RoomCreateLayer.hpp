#pragma once

#include <Geode/Geode.hpp>

#include "ui/utils/NineSlice.hpp"

using namespace geode::prelude;

class RoomCreateLayer : public CCNode {
protected:
    TextInput* m_nameInput;
    TextInput* m_passInput;
    TextInput* m_limitInput;

    CCMenuItemToggler* m_privateToggle;
    CCMenuItemToggler* m_viewOnlyToggle;

    bool init();

public:
    static RoomCreateLayer* create();

    void onCreate(CCObject*);

    NineSliceBox* m_createRoomLayer;
};