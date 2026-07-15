#pragma once

#include <Geode/Geode.hpp>
#include "cocos2d.h"

using namespace geode::prelude;

class settingsPopup : public geode::Popup {
protected:
    bool init(std::string const& value);

public:
    static settingsPopup* create(std::string const& text);
    std::string m_hostName = "Player";
    int m_maxPlayers = 999;
    int m_port = 12345;
    bool m_loopbackMode = false;
    bool m_verboseMode = false;
};