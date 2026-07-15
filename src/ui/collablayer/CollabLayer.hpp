#pragma once

#include <Geode/Geode.hpp>
#include "settings/settings.hpp"

using namespace geode::prelude;

class CollabLayer : public CCLayer {
protected:
    bool init() override;
    void onBack(CCObject*);
    void onSettings(CCObject*);
    void onMultiplayer(CCObject*);
    void onHost(CCObject*);
    void onJoin(CCObject*);

public:
    static CollabLayer* create();
    static CollabLayer *get() {
        auto* runningScene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
        if (!runningScene) return nullptr;
        for (auto* child : CCArrayExt<CCNode*>(runningScene->getChildren())) {
            if (auto* layer = typeinfo_cast<CollabLayer*>(child)) {
                return layer;
            }
        }
        return nullptr;
    };
};
