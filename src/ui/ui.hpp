#pragma once

using namespace cocos2d;

class CollabLayer : public CCLayer {
public:
    static CollabLayer* create();
    bool init() override;

    void onBack(CCObject*);
    void onSettings(CCObject*);
};