#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

class ChatLayer : public CCLayer {
protected:
    CCScale9Sprite* m_bg;
    ScrollLayer* m_scroll = nullptr;
    CCTextInputNode* m_input = nullptr;

    bool m_isOpen = false;

    virtual bool init() override;
    void onSendMessage(CCObject*);
    void animateSidebar(bool open);
    void onRichTextClicked(CCObject* sender);

public:
    static ChatLayer* create();
    void toggle();
    void addMessage(std::string const& user, std::string const& message, cocos2d::ccColor3B color);
    
    // Fixed: Added the double parameter to match CCLayer's definition
    virtual void keyDown(cocos2d::enumKeyCodes key, double repeat) override;
    bool isOpen() const { return m_isOpen; }
};