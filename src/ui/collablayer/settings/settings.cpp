#include "settings.hpp"

#include "cocos2d.h"

SettingsLayer* SettingsLayer::create() {
    auto ret = new SettingsLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}


void SettingsLayer::onBack(CCObject* sender) {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, 
        cocos2d::PopTransition()
    );
}

bool SettingsLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto background = CCSprite::create("GJ_gradientBG.png");    
    background->setScaleX(winSize.width / background->getContentSize().width);
    background->setScaleY(winSize.height / background->getContentSize().height);
    background->setPosition({winSize.width / 2, winSize.height / 2});
    background->setColor({ 120, 161, 255 });
    this->addChild(background, -10);


    auto backSprite = CCSprite::create("backbtn.png"_spr);
    backSprite->setAnchorPoint({0.5f, 0.5f});
    backSprite->setScale(0.2f);
    backSprite->setRotation(270.0f);
    backSprite->setColor({ 255, 255, 255 });
    backSprite->setOpacity(255);
    backSprite->setCascadeColorEnabled(true);
    backSprite->setCascadeOpacityEnabled(true);
    backSprite->setPosition({winSize.width * 0.02f, winSize.height * 0.95f});

    auto backButton = CCMenuItemSpriteExtra::create(
        backSprite,
        this,
        menu_selector(SettingsLayer::onBack)
    );
    backButton->setScale(0.8f);
    
    backButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.92f
    });

auto menu1 = CCMenu::create();
    menu1->setID("BackMenu"_spr);
    menu1->setPosition(CCPoint(winSize.width * 0.04f, (float)(0)));
    menu1->addChild(backButton);
    addChild(menu1);
    return true;
}

/* 1. Host Name Input (Maps to m_hostName)
UI Control: CCTextInputNode (a text input box with a cursor).

Description: Allows the user to type their display name.

Behavior: Defaults to "Player". It should have a character limit (e.g., 15–20 characters max) so it doesn't break other users' lobby UI layouts when they see it in the room list.

2. Player Limit Slider (Maps to m_maxPlayers)
UI Control: Slider / CCLabelBMFont combo.

Description: A slider that sets the room's maximum capacity.

Behavior:

Range: Set the slider's minimum value to 2 and the maximum to 999.

Display: As the user drags the slider, a text label right next to it should dynamically update to show the selected number (e.g., "Max Players: 120").

3. Port Configuration Input (Maps to m_port)
UI Control: CCTextInputNode (restricted to numbers only).

Description: A small text box to change the local UDP communication port.

Behavior: Defaults to 12345. Limiting this field strictly to numeric characters prevents crashes if someone accidentally types letters.

4. Loopback Mode Toggle (Maps to m_loopbackMode)
UI Control: CCMenuItemToggler (the standard green checkmark / red "X" button).

Description: A toggle labeled "Enable Loopback (Single PC Test)".

Behavior: When checked, it tells the network code to send packets to 127.0.0.1 instead of casting them across the physical LAN. This is what lets you run two copies of GD on your PC to test the lobby by yourself.

5. Verbose Logging Toggle (Maps to m_verboseLogging)
UI Control: CCMenuItemToggler (green checkmark / red "X" button).

Description: A toggle labeled "Debug Console Logs".

Behavior: Enables heavy logging. Excellent for checking if network packets are dropping without having to rebuild the mod with debug points. */
