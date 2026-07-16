#include <Geode/Geode.hpp>
#include "settings/settings.hpp"
#include "CollabLayer.hpp"
#include "../MultiplayerPopup.hpp"
#include "SessionManager.hpp"
#include "../ui.hpp"

using namespace geode::prelude;
using namespace mpedit;

CollabLayer* CollabLayer::create() {
    auto ret = new CollabLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void CollabLayer::onBack(CCObject* sender) {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, 
        cocos2d::PopTransition()
    );
}

void CollabLayer::updateStatus(float) {
        auto &session = SessionManager::get();

        bool online = SessionManager::get().isInSession();
        size_t playerCount = session.getPlayers().size();


        m_onlineSprite->setVisible(online);
        m_offlineSprite->setVisible(!online);
        m_playerCountLabel->setString(
            fmt::format("{}", playerCount).c_str()
        );
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

    auto &session = SessionManager::get();
    auto playerCount = session.getPlayers().size();

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

    auto* joinSprite = ButtonSprite::create(
        "Join", 40, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.45f
    );
    auto* hostSprite = ButtonSprite::create(
        "Host", 40, true, "bigFont.fnt", "GJ_button_02.png", 30.f, 0.45f
    );
    auto* joinBtn = CCMenuItemSpriteExtra::create(
        joinSprite,
        this,
        menu_selector(CollabLayer::onJoin)
    );
    auto* hostBtn = CCMenuItemSpriteExtra::create(
        hostSprite,
        this,
        menu_selector(CollabLayer::onHost)
    );
    joinBtn->setID("multiplayer-button"_spr);
    hostBtn->setID("host-button"_spr);

    joinBtn->setPosition({
        winSize.width * 0.65f,
        winSize.height * 0.55f
    });

    hostBtn->setPosition({
        winSize.width * 0.8f,
        winSize.height * 0.55f
    });
    bool isInSession = session.isInSession();

    m_onlineSprite = CCSprite::create("online.png"_spr);
    m_onlineSprite->setScale(0.5f);

    m_offlineSprite = CCSprite::create("offline.png"_spr);
    m_offlineSprite->setScale(0.5f);

    
    bool online = SessionManager::get().isInSession();

    m_onlineSprite->setVisible(online);
    m_offlineSprite->setVisible(!online);

    

    m_offlineSprite->setPosition({
        winSize.width * 0.8f,
        winSize.height * 0.45f
    });
    m_onlineSprite->setPosition({
        winSize.width * 0.8f,
        winSize.height * 0.45f
    });
    m_onlineSprite->setScale(0.2f);
    m_offlineSprite->setScale(0.22f);
    m_playerCountLabel = CCLabelBMFont::create(
    fmt::format("{}", playerCount).c_str(),
    "bigFont.fnt"
    );
    m_playerCountLabel->setScale(0.5f);
    m_playerCountLabel->setPosition({
    winSize.width * 0.85f,
    winSize.height * 0.45f
    });

    auto menu1 = CCMenu::create();
    menu1->setID("BackMenu"_spr);
    menu1->setPosition(CCPoint(winSize.width * 0.04f, (float)(0)));
    menu1->addChild(backButton);
    menu1->addChild(settingsButton);
    addChild(menu1);

        
    auto menu2 = CCMenu::create();
    menu2->setID("JoinMenu"_spr);
    menu2->setPosition(CCPoint(winSize.width * 0.1f, (float)(winSize.height * 0.35f)));
    menu2->addChild(joinBtn);
    menu2->addChild(hostBtn);
    addChild(menu2);

    auto menu3 = CCMenu::create();
    menu3->setID("SessionMenu"_spr);
    menu3->addChild(m_onlineSprite);
    menu3->addChild(m_offlineSprite);
    menu3->addChild(m_playerCountLabel);
    menu3->setPosition(CCPoint(winSize.width * 0.1f, (float)(winSize.height * 0.35f)));
    addChild(menu3);


    this->schedule(schedule_selector(CollabLayer::updateStatus), 1.0f);

    return true;
};

void CollabLayer::onSettings(CCObject*) {
    /* auto popup = settingsPopup::create("Hello");
    if (popup) {
        this->addChild(popup);
    } */
    auto SettingsLayer = SettingsLayer::create();
    auto scene = CCScene::create();
    scene->addChild(SettingsLayer);
    auto transition = Transition::create(0.5f, scene, {0, 0, 0});
    CCDirector::sharedDirector()->pushScene(transition);
};

void CollabLayer::onMultiplayer(CCObject*) {
    auto popup = MultiplayerPopup::create();
    if (popup) {
        this->addChild(popup);
    }
};

void CollabLayer::onHost(CCObject*) {
    auto popup = MultiplayerPopup::create(MultiplayerPopup::Mode::Host);
    if (popup) {
        this->addChild(popup);
    }
}

void CollabLayer::onJoin(CCObject*) {
    auto popup = MultiplayerPopup::create(MultiplayerPopup::Mode::Join);
    if (popup) {
        this->addChild(popup);
    }
}



