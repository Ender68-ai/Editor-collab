#include <Geode/Geode.hpp>
#include "settings/settings.hpp"
#include "CollabLayer.hpp"
#include "../ui.hpp"

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
    auto background = CCSprite::create("GJ_gradientBG.png");    
    background->setScaleX(winSize.width / background->getContentSize().width);
    background->setScaleY(winSize.height / background->getContentSize().height);
    background->setPosition({winSize.width / 2, winSize.height / 2});
    background->setColor({ 120, 161, 255 });
    this->addChild(background, -10);

    // Create the sprite for the back button
    
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
        menu_selector(CollabLayer::onBack)
    );

    auto settingsSprite = CCSprite::create("settingsbtn.png"_spr);
    settingsSprite->setAnchorPoint({0.5f, 0.5f});
    settingsSprite->setScale(0.25f);
    settingsSprite->setColor({ 250, 243, 243 });
    settingsSprite->setOpacity(255);
    settingsSprite->setCascadeColorEnabled(true);
    settingsSprite->setCascadeOpacityEnabled(true); 

    auto settingsButton = CCMenuItemSpriteExtra::create(
        settingsSprite,
        this,
        menu_selector(CollabLayer::onSettings)
    );

    backButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.92f
    });

    settingsButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.80f
    });

    auto menu = CCMenu::create();
    menu->setID("BackMenu"_spr);
    menu->setPosition(CCPoint(winSize.width * 0.04f, (float)(0)));
    menu->addChild(backButton);
    menu->addChild(settingsButton);
    addChild(menu);




    return true;
};

void CollabLayer::onSettings(CCObject*) {
    auto popup = settingsPopup::create("Hello");
    if (popup) {
        this->addChild(popup);
    }
}



