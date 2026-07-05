#pragma once
#include "cocos2d.h"

using namespace cocos2d;

class CollabLayer : public CCLayer {
public:
    static CollabLayer* create();
    bool init() override;

    void onBack(CCObject*);
    void onMyButton(CCObject*);
};