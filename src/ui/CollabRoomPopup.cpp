#include "CollabRoomPopup.hpp"

using namespace geode::prelude;

CollabRoomPopup* CollabRoomPopup::create(std::string const& roomCode) {
    auto ret = new CollabRoomPopup();
    if (ret && ret->init(roomCode)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void CollabRoomPopup::setRoomCode(std::string const& roomCode) {
    m_roomCode = roomCode;
}

bool CollabRoomPopup::init(std::string const& roomCode) {
    m_roomCode = roomCode;
    if (!Popup::init(280.f, 180.f))
        return false;

    this->setTitle("Collab Editor");
    auto winSize = m_mainLayer->getContentSize();

    // Input Field - Centered slightly higher
    auto roomInput = TextInput::create(150.f, "Room Code", "chatFont.fnt");
    roomInput->setPosition(winSize / 2 + cocos2d::CCPoint(0, 20.f));
    m_mainLayer->addChild(roomInput);

    // Button Menu for Host/Join
    auto menu = CCMenu::create();
    menu->setPosition(winSize / 2 + cocos2d::CCPoint(0, -40.f));
    m_mainLayer->addChild(menu);

    auto hostBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Host", "goldFont.fnt", "GJ_button_01.png", 0.8f),
        ButtonSprite::create("Host", "goldFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(CollabRoomPopup::onHost)
    );
    
    auto joinBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Join", "bigFont.fnt", "GJ_button_02.png", 0.8f),
        ButtonSprite::create("Join", "bigFont.fnt", "GJ_button_02.png", 0.8f),
        this,
        menu_selector(CollabRoomPopup::onJoin)
    );

    menu->addChild(hostBtn);
    menu->addChild(joinBtn);
    
    // Stack buttons horizontally with 10px spacing
    menu->setLayout(RowLayout::create()->setGap(10.f));
    menu->updateLayout();

    return true;
}

void CollabRoomPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);
}

void CollabRoomPopup::onHost(cocos2d::CCObject* sender) {
    FLAlertLayer::create(
        "Host Room",
        "<cy>Host Action</c> - Room code will be generated and shared with other players.",
        "OK"
    )->show();
}

void CollabRoomPopup::onJoin(cocos2d::CCObject* sender) {
    FLAlertLayer::create(
        "Join Room",
        "<cy>Join Action</c> - Enter the room code from the input field to join their collaboration session.",
        "OK"
    )->show();
}