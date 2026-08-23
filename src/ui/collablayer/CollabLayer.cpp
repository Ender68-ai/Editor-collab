#include <Geode/Geode.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/binding/CustomListView.hpp>
#include <Geode/modify/LevelCell.hpp>

#include "settings/settings.hpp"
#include "CollabLayer.hpp"
#include "SessionManager.hpp"
#include "../ui.hpp"
#include "modes/HostMode.hpp"
#include "modes/JoinMode.hpp"


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


bool CollabLayer::cellPerformedAction(
    TableViewCell* cell,
    int listType,
    CellAction action,
    cocos2d::CCNode* parent
) {
    return false;
}

int CollabLayer::getSelectedCellIdx() {
    return -1;
}

bool CollabLayer::shouldSnapToSelected() {
    return false;
}

int CollabLayer::getCellDelegateType() {
    return 0;
}

bool fromCollab = false;


bool CollabLayer::init() {
    if (!CCLayer::init())
        return false;

    
    this->setID("collab-layer"_spr);

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

    auto modeButton = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
    modeButton->setScale(1.0f);
    modeButton->setContentWidth(2.f);
    modeButton->setColor({139, 69, 19}); // brown

    modeButton->setPosition({
        winSize.width * 0.5f,
        winSize.height * 0.92f
    });

    auto jointxt = CCLabelBMFont::create("JOIN", "goldFont.fnt");

    auto joinmodebtn = CCMenuItemSpriteExtra::create(
        jointxt,
        this,
        menu_selector(CollabLayer::onJoinMode)
    );

    joinmodebtn->setScale(0.8f);

    joinmodebtn->setPosition({
        winSize.width * 0.4f,
        winSize.height * 0.92f
    });

    auto hosttxt = CCLabelBMFont::create("HOST", "goldFont.fnt");

    auto hostmodebtn = CCMenuItemSpriteExtra::create(
        hosttxt,
        this,
        menu_selector(CollabLayer::onHostMode)
    );

    hostmodebtn->setScale(0.8f);

    auto modeButtonLine = CCLayerColor::create({255, 255, 255, 255}, 100.f, 1.f);
    modeButtonLine->setPosition({150.f, 150.f});

    hostmodebtn->setPosition({
        winSize.width * 0.6f,
        winSize.height * 0.92f
    });
    
    backButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.92f
    });

    settingsButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.80f
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
    menu2->setID("SessionMenu"_spr);
    menu2->addChild(m_onlineSprite);
    menu2->addChild(m_offlineSprite);
    menu2->addChild(m_playerCountLabel);
    menu2->setPosition(CCPoint(winSize.width * 0.1f, (float)(winSize.height * 0.35f)));
    addChild(menu2);

    auto menu3 = CCMenu::create();
    menu3->setID("CollabStateMenu"_spr);
    menu3->setPosition(0, 0);
    menu3->addChild(modeButton);
    menu3->addChild(joinmodebtn);
    menu3->addChild(hostmodebtn);
    menu3->addChild(modeButtonLine);
    addChild(menu3);

    auto delegate = HostLocalLevelList::create();

    auto listView = CustomListView::create(
        LocalLevelManager::sharedState()->m_localLevels,
        delegate,
        200.f,
        200.f,
        0,
        BoomListType::Level,
        0.f
    );

    listView->setID("local-levels-list"_spr);

    auto listLayer = GJListLayer::create(
        listView,
        "Local",
        {255, 255, 255, 255},
        200.f,
        200.f,
        0
    );
    auto top = listLayer->getChildByID("top-border");
    auto bottom = listLayer->getChildByID("bottom-border");
    auto view = listLayer->getChildByID("view-button");

    if (top) {
        top->setScaleX(0.6f);
    }

    if (bottom) { 
        bottom->setScaleX(0.6f);
    }



    listLayer->setPosition({
        winSize.width * 0.15f,
        winSize.height * 0.15f
    });
    
    addChild(listLayer);


    if(m_collabState) {
        CollabLayer::onHostMode(nullptr);
    } else {
        CollabLayer::onJoinMode(nullptr);
    }

    this->schedule(schedule_selector(CollabLayer::updateStatus), 0.25f);



    

    
    return true;
};


void CollabLayer::onBack(CCObject* sender) {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, 
        cocos2d::PopTransition()
    );
}


void CollabLayer::onSettings(CCObject*) {
    auto SettingsLayer = SettingsLayer::create();
    auto scene = CCScene::create();
    scene->addChild(SettingsLayer);
    auto transition = Transition::create(0.5f, scene, {0, 0, 0});
    CCDirector::sharedDirector()->pushScene(transition);
};

void CollabLayer::updateStatus(float) {
        auto &session = SessionManager::get();

        bool online = SessionManager::get().isInSession();
        size_t playerCount = session.getPlayers().size();


        m_onlineSprite->setVisible(online);
        m_offlineSprite->setVisible(!online);
        m_playerCountLabel->setString(
            fmt::format("{}", playerCount).c_str()
        );

};
void CollabLayer::onHostMode(CCObject*) {
    // adds roomcreate and roomlist to the layer.

    // Host mode adds the gjlistlayer and the roomcreatelayer to collablayer. it also flips the mode bool so onjoinmode is hidden.

};
void CollabLayer::onJoinMode(CCObject*) {
    // adds locallevellist and roomslist to the layer.

    // Join mode adds the roomslist and the RoomDescLayer to collablayer. it also flips the mode bool so onhostmode is hidden.

};



