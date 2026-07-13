#pragma once

#include <Geode/Geode.hpp>
#include "cocos2d.h"

using namespace geode::prelude;

class settingsPopup : public geode::Popup {
protected:
    bool init(std::string const& value);

public:
    static settingsPopup* create(std::string const& text);
};