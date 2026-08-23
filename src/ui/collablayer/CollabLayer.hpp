#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class CollabLayer : public CCLayer, public TableViewCellDelegate {
protected:
    bool init() override;
    void onBack(CCObject*);
    void onSettings(CCObject*);
    void updateStatus(float dt);

    bool fromCollab = false;

    bool m_collabState = false; // false = join, true = host

    bool isCollabList = false;

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
    CCSprite* m_onlineSprite = nullptr;
    CCSprite* m_offlineSprite = nullptr;
    CCLabelBMFont* m_playerCountLabel = nullptr;

    bool cellPerformedAction(
    TableViewCell* cell,
    int listType,
    CellAction action,
    cocos2d::CCNode* parent
    ) override;

    int getSelectedCellIdx() override;

    bool shouldSnapToSelected() override;

    int getCellDelegateType() override;

    void onHostMode(CCObject*);
    void onJoinMode(CCObject*);


};


