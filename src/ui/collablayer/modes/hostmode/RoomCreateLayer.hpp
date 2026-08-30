#pragma once

#include <functional>

#include <Geode/Geode.hpp>

#include "ui/utils/NineSlice.hpp"

using namespace geode::prelude;

class RoomCreateLayer : public CCNode {
private:
    void onSessionStarted(std::string const& roomCode, int localPlayerId);

protected:
    TextInput* m_nameInput;
    TextInput* m_passInput;
    TextInput* m_limitInput;

    CCMenuItemToggler* m_privateToggle;
    CCMenuItemToggler* m_viewOnlyToggle;

    bool init();

    CCLabelBMFont* m_boxTitle;
    CCNode* m_layoutNode;
    CCMenu* m_createMenu;

    CCLabelBMFont* m_hostingTitle;
    std::function<void()> m_onRoomCreated;

public:
    static RoomCreateLayer* create(std::function<void()> onRoomCreated = {});

    void onCreate(CCObject*);

    NineSliceBox* m_createRoomLayer;
};