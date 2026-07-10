#include <Geode/Geode.hpp>
#include "../ui.hpp"

using namespace geode::prelude;

// Complex popup example: subclass of geode::Popup
class settingsPopup : public geode::Popup {
protected:
    bool init(std::string const& value) {
        if (!Popup::init(240.f, 160.f))
            return false;

        this->setTitle("Settings");

        auto label = CCLabelBMFont::create(value.c_str(), "bigFont.fnt");
        if (label && m_mainLayer) {
            m_mainLayer->addChildAtPosition(label, Anchor::Center);
        }

        return true;
    }

public:
    static settingsPopup* create(std::string const& text) {
        auto ret = new settingsPopup();
        if (ret && ret->init(text)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

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

    cocos2d::ccColor4B backgroundColor = { 255, 255, 255, 255 };
    auto background = CCLayerColor::create(backgroundColor, winSize.width, winSize.height);
    background->setPosition({0, 0});
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
    backSprite->setPosition({winSize.width * 0.05f, winSize.height * 0.9f});

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
        winSize.width * 0.05f,
        winSize.height * 0.9f
    });

    settingsButton->setPosition({
        winSize.width * 0.05f,
        winSize.height * 0.75f
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



