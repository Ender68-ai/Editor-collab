#include <Geode/Geode.hpp>
#include "ui.hpp"

using namespace geode::prelude;

CollabLayer* CollabLayer::create() {
    auto ret = new CollabLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void CollabLayer::onBack(CCObject*) {
    CCDirector::sharedDirector()->popScene();
}

bool CollabLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // Create the sprite for the back button
    auto backSprite = CCSprite::create("backbtn.png"_spr);
    backSprite->setAnchorPoint({0.5f, 0.5f});
    backSprite->setScale(0.2f);
    backSprite->setRotation(270.0f);
    backSprite->setColor({255, 255, 255});
    backSprite->setOpacity(255);
    backSprite->setCascadeColorEnabled(true);
    backSprite->setCascadeOpacityEnabled(true);
    backSprite->setPosition({winSize.width * 0.05f, winSize.height * 0.9f});

    // Make it clickable
    auto backButton = CCMenuItemSpriteExtra::create(
        backSprite,
        this,
        menu_selector(CollabLayer::onBack)
    );

    backButton->setPosition({
        winSize.width * 0.05f,
        winSize.height * 0.9f
    });

    // Menu to hold the button
    auto menu = CCMenu::create();
    menu->setPosition(CCPointZero);
    menu->addChild(backButton);

    addChild(menu);

    return true;
}