#include "CollabPanel.hpp"
#include "PlayerList.hpp"
#include "ChatWindow.hpp"
#include "CursorIndicator.hpp"
#include "../CollabManager.hpp"

CollabPanel* CollabPanel::s_instance = nullptr;

CollabPanel* CollabPanel::create() {
    auto ret = new CollabPanel();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void CollabPanel::setup() {
    if (s_instance) return;
    
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;
    
    s_instance = CollabPanel::create();
    scene->addChild(s_instance, 1000);
}

CollabPanel* CollabPanel::get() {
    return s_instance;
}

bool CollabPanel::init() {
    if (!CCNode::init()) return false;
    
    createUI();
    scheduleUpdate();
    return true;
}

void CollabPanel::createUI() {
    // Main panel background
    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setContentSize({300, 400});
    bg->setPosition({160, 120});
    bg->setOpacity(200);
    addChild(bg);
    
    // Title
    auto title = CCLabelBMFont::create("Collaborative Editor", "bigFont.fnt");
    title->setPosition({160, 380});
    addChild(title);
    
    // Status label
    m_statusLabel = CCLabelBMFont::create("Disconnected", "chatFont.fnt");
    m_statusLabel->setPosition({160, 350});
    m_statusLabel->setColor({255, 100, 100});
    addChild(m_statusLabel);
    
    // Main menu
    m_mainMenu = CCMenu::create();
    m_mainMenu->setPosition({0, 0});
    addChild(m_mainMenu);
    
    // Host Session button
    auto hostBtnSprite = ButtonSprite::create("Host Session");
    auto hostBtn = CCMenuItemSpriteExtra::create(
        hostBtnSprite,
        this,
        (SEL_MenuHandler)&CollabPanel::onHostSession
    );
    hostBtn->setPosition({160, 300});
    m_mainMenu->addChild(hostBtn);
    
    // Join Session button
    auto joinBtnSprite = ButtonSprite::create("Join Session");
    auto joinBtn = CCMenuItemSpriteExtra::create(
        joinBtnSprite,
        this,
        (SEL_MenuHandler)&CollabPanel::onJoinSession
    );
    joinBtn->setPosition({160, 260});
    m_mainMenu->addChild(joinBtn);
    
    // Disconnect button
    auto disconnectBtnSprite = ButtonSprite::create("Disconnect");
    auto disconnectBtn = CCMenuItemSpriteExtra::create(
        disconnectBtnSprite,
        this,
        (SEL_MenuHandler)&CollabPanel::onDisconnect
    );
    disconnectBtn->setPosition({160, 220});
    m_mainMenu->addChild(disconnectBtn);
    
    // Settings button
    auto settingsBtnSprite = ButtonSprite::create("Settings");
    auto settingsBtn = CCMenuItemSpriteExtra::create(
        settingsBtnSprite,
        this,
        (SEL_MenuHandler)&CollabPanel::onSettings
    );
    settingsBtn->setPosition({160, 180});
    m_mainMenu->addChild(settingsBtn);
    
    // Player list container
    m_playerListNode = CCNode::create();
    m_playerListNode->setPosition({20, 140});
    addChild(m_playerListNode);
    
    // Chat container
    m_chatNode = CCNode::create();
    m_chatNode->setPosition({20, 50});
    addChild(m_chatNode);
    
    // Chat input
    m_chatInput = CCTextFieldNode::create("Type message...", "bigFont.fnt", 20);
    m_chatInput->setPosition({160, 30});
    addChild(m_chatInput);
    
    // Initially hidden
    setVisible(false);
}

void CollabPanel::show() {
    setVisible(true);
    updatePlayerList();
}

void CollabPanel::hide() {
    setVisible(false);
}

void CollabPanel::toggle() {
    setVisible(!isVisible());
    if (isVisible()) {
        updatePlayerList();
    }
}

void CollabPanel::updatePlayerList() {
    if (!m_playerListNode) return;
    
    m_playerListNode->removeAllChildrenWithCleanup(true);
    
    auto& manager = CollabManager::get();
    const auto& players = manager.getPlayers();
    
    int yOffset = 0;
    for (const auto& [playerID, player] : players) {
        auto nameLabel = CCLabelBMFont::create(player.name.c_str(), "chatFont.fnt");
        nameLabel->setColor(player.color);
        nameLabel->setPosition({0, -yOffset});
        nameLabel->setAnchorPoint({0, 0.5f});
        m_playerListNode->addChild(nameLabel);
        
        yOffset += 20;
    }
}

void CollabPanel::addChatMessage(const std::string& playerName, const std::string& message) {
    // This would integrate with ChatWindow component
    log::info("Chat: {}: {}", playerName, message);
}

void CollabPanel::onHostSession(CCObject* sender) {
    // Create input dialog for server details
    std::string serverUrl = "ws://localhost:8080";
    std::string playerName = "Player_" + std::to_string(rand() % 1000);
    
    if (CollabManager::get()->connect(serverUrl, playerName)) {
        m_statusLabel->setString("Connected");
        m_statusLabel->setColor({100, 255, 100});
        updatePlayerList();
    }
}

void CollabPanel::onJoinSession(CCObject* sender) {
    // Similar to host but with different UI flow
    onHostSession(sender);
}

void CollabPanel::onDisconnect(CCObject* sender) {
    CollabManager::get()->disconnect();
    m_statusLabel->setString("Disconnected");
    m_statusLabel->setColor({255, 100, 100});
    updatePlayerList();
}

void CollabPanel::onSettings(CCObject* sender) {
    // Toggle settings visibility
    auto& manager = CollabManager::get();
    manager.setLowLagMode(!manager.isLowLagMode());
}

void CollabPanel::onSendMessage(CCObject* sender) {
    if (!m_chatInput) return;
    
    std::string message = m_chatInput->getString();
    if (!message.empty()) {
        CollabManager::get()->sendChatMessage(message);
        m_chatInput->setString("");
    }
}

void CollabPanel::update(float dt) {
    CCNode::update(dt);
    
    // Update status
    auto& manager = CollabManager::get();
    if (manager.isConnected()) {
        m_statusLabel->setString("Connected");
        m_statusLabel->setColor({100, 255, 100});
    } else {
        m_statusLabel->setString("Disconnected");
        m_statusLabel->setColor({255, 100, 100});
    }
}
