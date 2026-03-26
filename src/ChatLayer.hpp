#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

class ChatLayer : public CCLayer, public TextInputDelegate {
protected:
    CCScale9Sprite* m_bg;
    ScrollLayer* m_scroll = nullptr;
    CCTextInputNode* m_input = nullptr;
    bool m_isOpen = false;

    virtual bool init() override;
    void onSendMessage(CCObject*);
    void onClose(CCObject*);

public:
    static ChatLayer* create();
    void toggle();
    void addMessage(std::string const& user, std::string const& message, cocos2d::ccColor3B color, bool isMe);
    
    virtual void registerWithTouchDispatcher() override;
    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void keyDown(cocos2d::enumKeyCodes key, double repeat) override;
    virtual void onExit() override; // Fix memory leak
    
    virtual void textChanged(CCTextInputNode* p0) override;

    bool isOpen() const { return m_isOpen; }
};