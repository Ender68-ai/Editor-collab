#pragma once

#include <Geode/Geode.hpp>
#include "settings/settings.hpp"

using namespace geode::prelude;

class CollabLayer : public CCLayer {
protected:
    bool init() override;
    void onBack(CCObject*);
    void onSettings(CCObject*);

public:
    static CollabLayer* create();
};
