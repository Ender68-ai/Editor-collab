#include <Geode/Geode.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/binding/CustomListView.hpp>
#include <Geode/modify/LevelCell.hpp>

#include "settings/settings.hpp"
#include "CollabLayer.hpp"
#include "SessionManager.hpp"
#include "modes/HostMode.hpp"
#include "modes/JoinMode.hpp"
#include "../utils/Panel.hpp"
#include "../Ui.hpp"
#include "../menu/MultiplayerMenuPopup.hpp"


using namespace geode::prelude;
using namespace mpedit;


// Create


CollabLayer* CollabLayer::create() {
    auto ret = new CollabLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

// Stuff for listview delegate

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


// initialize fromcollab so hooks work

bool fromCollab = false;

// init
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

    // session indicator for online indicator sprite
    auto &session = SessionManager::get();
    auto playerCount = session.getPlayers().size();

    auto settingsButton = CCMenuItemSpriteExtra::create(
        settingsSprite,
        this,
        menu_selector(CollabLayer::onSettings)
    );


    // mode tag and buttons

    auto modeTag = Panel::create("", {winSize.width * 0.4f, 50.f});
    modeTag->setPosition({winSize.width * 0.5f, winSize.height * 0.92f});
    modeTag->setScale(0.8f);


    auto jointxt = CCLabelBMFont::create("JOIN", "goldFont.fnt");

    auto joinmodebtn = CCMenuItemSpriteExtra::create(
        jointxt,
        this,
        menu_selector(CollabLayer::onJoinMode)
    );

    joinmodebtn->setScale(0.6f);

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

    hostmodebtn->setScale(0.6f);

    hostmodebtn->setPosition({
        winSize.width * 0.6f,
        winSize.height * 0.92f
    });

    auto modeMenu = CCMenu::create();
    modeMenu->setID("CollabStateMenu"_spr);
    modeMenu->setPosition(0, 0);
    modeMenu->addChild(modeTag);
    modeMenu->addChild(joinmodebtn);
    modeMenu->addChild(hostmodebtn);
    addChild(modeMenu);

    // BackMenu
    
    backButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.92f
    });

    settingsButton->setPosition({
        winSize.width * 0.02f,
        winSize.height * 0.80f
    });

    auto backMenu = CCMenu::create();
    backMenu->setID("BackMenu"_spr);
    backMenu->setPosition(CCPoint(winSize.width * 0.04f, (float)(0)));
    backMenu->addChild(backButton);
    backMenu->addChild(settingsButton);
    addChild(backMenu);

    // SessionMenu

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
        
    auto SessionMenu = CCMenu::create();
    SessionMenu->setID("SessionMenu"_spr);
    SessionMenu->addChild(m_onlineSprite);
    SessionMenu->addChild(m_offlineSprite);
    SessionMenu->addChild(m_playerCountLabel);
    SessionMenu->setPosition(CCPoint(winSize.width * 0.1f, (float)(winSize.height * 0.35f)));
    addChild(SessionMenu);

    // JoinModes implementation

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

    auto levelListLayer = GJListLayer::create(
        listView,
        "Local",
        {255, 255, 255, 255},
        200.f,
        200.f,
        0
    );
    m_listLayer = levelListLayer;
    auto top = levelListLayer->getChildByID("top-border");
    auto bottom = levelListLayer->getChildByID("bottom-border");
    auto view = levelListLayer->getChildByID("view-button");

    if (top) {
        top->setScaleX(0.6f);
    }

    if (bottom) { 
        bottom->setScaleX(0.6f);
    }

    levelListLayer->setPosition({
        winSize.width * 0.15f,
        winSize.height * 0.15f
    });
    levelListLayer->setVisible(false);
    
    addChild(levelListLayer);

    // PublicRoomList
    
    auto panel = NineSliceBox::create(winSize.width * 0.8f, winSize.height * 0.7f);
    panel->setPosition({
        winSize.width * 0.1f,
        winSize.height * 0.1f
    });
    m_publicRoomList = panel;

    auto boxTitle = CCLabelBMFont::create("Public Rooms", "goldFont.fnt");
    boxTitle->setScale(0.6f);
    boxTitle->setPosition({
        winSize.width * 0.4f,
        winSize.height * 0.65f
    });







    panel->addChild(boxTitle);
    this->addChild(panel);
    


    /// Placeholder button (relocate for now)

    auto* roomListBtnSpr = ButtonSprite::create(
            "Multiplayer Edit", 90, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.45f);

    auto* roomListButton = CCMenuItemSpriteExtra::create(
        roomListBtnSpr,
        this,
        menu_selector(CollabLayer::onMultiplayer)
    );
    m_roomListButton = roomListButton;

    auto roomListMenu = CCMenu::create();
    roomListMenu->setPosition({
        winSize.width * 0.7f,
        winSize.height * 0.15f
    });
    m_roomListMenu = roomListMenu;
    roomListMenu->addChild(roomListButton);
    this->addChild(roomListMenu);


    // end of JoinMode

    // HostModes implementation

    // RoomCreate




    

    // end of HostMode


    if(m_collabState) {
        CollabLayer::onHostMode(nullptr);
    } else {
        CollabLayer::onJoinMode(nullptr);
    }


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
    m_listLayer->setVisible(true);

    m_roomListMenu->setVisible(false);

    m_publicRoomList->setVisible(false);

    m_roomListButton->setVisible(false);
    // Host mode adds the gjlistlayer and the roomcreatelayer to collablayer. it also flips the mode bool so onjoinmode is hidden.

};
void CollabLayer::onJoinMode(CCObject*) {

    m_listLayer->setVisible(false);

    m_roomListMenu->setVisible(true);

    m_publicRoomList->setVisible(true);

    m_roomListButton->setVisible(true);

}


void CollabLayer::onMultiplayer(CCObject*) {
        MultiplayerMenuPopup::create()->show();
    }



