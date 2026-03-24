#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class ChatLayer : public CCLayer {
protected:
    CCScale9Sprite* m_bg;
    bool m_isDragging = false;
    bool m_isResizing = false;
    CCPoint m_dragOffset;

    virtual bool init();
    ccColor3B getThemeColor();

    // Touch handlers
    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;

public:
    static ChatLayer* create();
};