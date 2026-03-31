#include "CollabPopups.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GManager.hpp>

using namespace geode::prelude;

// CollabSetupPopup Implementation
bool CollabSetupPopup::init(const std::string& levelName, int levelID) {
    if (!Popup::init(300.f, 200.f))
        return false;
    
    m_data = std::make_tuple(levelName, levelID);
    
    this->setTitle("Collab?");
    
    auto label = CCLabelBMFont::create("Enable collaboration for this level?", "bigFont.fnt");
    label->setPosition({150.f, 120.f});
    m_mainLayer->addChild(label);
    
    auto nameLabel = CCLabelBMFont::create(levelName.c_str(), "goldFont.fnt");
    nameLabel->setScale(0.8f);
    nameLabel->setPosition({150.f, 90.f});
    m_mainLayer->addChild(nameLabel);
    
    // Enable switch
    auto enableBtn = CCMenuItemToggler::create(
        CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
        CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
        this,
        menu_selector(CollabSetupPopup::onEnable)
    );
    enableBtn->setPosition({150.f, 50.f});
    
    auto menu = CCMenu::create();
    menu->addChild(enableBtn);
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);
    
    return true;
}

void CollabSetupPopup::onEnable(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    bool enabled = toggle->isSelected();
    
    if (enabled) {
        auto levelName = std::get<0>(m_data);
        auto levelID = std::get<1>(m_data);
        auto accountID = GJAccountManager::get()->m_accountID;
        
        CollabManager::get()->enableCollabForLevel(levelID, levelName, accountID);
        CollabManager::get()->setFirstTimeSetupComplete();
        
        // Show success message
        FLAlertLayer::create("Success", "Collaboration enabled! You can now add players.", "OK")->show();
        this->onClose(nullptr);
    }
}

CollabSetupPopup* CollabSetupPopup::create(const std::string& levelName, int levelID) {
    auto ret = new CollabSetupPopup();
    if (ret && ret->init(levelName, levelID)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// CollabManagementPopup Implementation
bool CollabManagementPopup::init(int levelID) {
    if (!Popup::init(400.f, 300.f))
        return false;
    
    m_data = levelID;
    
    this->setTitle("Manage Collaboration");
    
    auto collabManager = CollabManager::get();
    auto& collabLevels = collabManager->getCollabLevels();
    auto level = collabLevels.find(levelID);
    
    if (level != collabLevels.end()) {
        auto nameLabel = CCLabelBMFont::create(level->second.levelName.c_str(), "goldFont.fnt");
        nameLabel->setScale(0.7f);
        nameLabel->setPosition({200.f, 250.f});
        m_mainLayer->addChild(nameLabel);
    }
    
    setupPlayerList();
    
    // Add Player Button
    auto addBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Add Player", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(CollabManagementPopup::onAddPlayer)
    );
    addBtn->setPosition({200.f, 40.f});
    
    // Back Button
    auto backBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Back", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(CollabManagementPopup::onBack)
    );
    backBtn->setPosition({100.f, 40.f});
    
    auto menu = CCMenu::create();
    menu->addChild(addBtn);
    menu->addChild(backBtn);
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);
    
    return true;
}

void CollabManagementPopup::setupPlayerList() {
    auto levelID = m_data;
    auto players = CollabManager::get()->getPlayersInLevel(levelID);
    
    // Create player list (simplified version)
    float y = 220.f;
    for (const auto& player : players) {
        auto nameLabel = CCLabelBMFont::create(player.name.c_str(), "bigFont.fnt");
        nameLabel->setScale(0.5f);
        nameLabel->setPosition({50.f, y});
        m_mainLayer->addChild(nameLabel);
        
        // Role label
        std::string roleStr;
        switch (player.role) {
            case CollabRole::Editor: roleStr = "Editor"; break;
            case CollabRole::Suggestor: roleStr = "Suggestor"; break;
            case CollabRole::Viewer: roleStr = "Viewer"; break;
        }
        
        auto roleLabel = CCLabelBMFont::create(roleStr.c_str(), "chatFont.fnt");
        roleLabel->setScale(0.4f);
        roleLabel->setPosition({200.f, y});
        m_mainLayer->addChild(roleLabel);
        
        // Enable/Disable toggle
        auto toggle = CCMenuItemToggler::create(
            CCSprite::createWithSpriteFrameName(player.enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"),
            CCSprite::createWithSpriteFrameName(player.enabled ? "GJ_checkOff_001.png" : "GJ_checkOn_001.png"),
            this,
            menu_selector(CollabManagementPopup::onTogglePlayer)
        );
        toggle->setUserObject(CCInteger::create(player.accountID));
        toggle->setPosition({350.f, y});
        
        auto menu = CCMenu::create();
        menu->addChild(toggle);
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);
        
        y -= 30.f;
    }
}

void CollabManagementPopup::onAddPlayer(CCObject* sender) {
    // TODO: Implement add player functionality
    FLAlertLayer::create("Coming Soon", "Add player functionality will be implemented soon!", "OK")->show();
}

void CollabManagementPopup::onBack(CCObject* sender) {
    this->onClose(nullptr);
}

void CollabManagementPopup::onTogglePlayer(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto accountID = static_cast<CCInteger*>(toggle->getUserObject())->getValue();
    auto levelID = m_data;
    
    CollabManager::get()->togglePlayerStatus(levelID, accountID);
    this->setupPlayerList(); // Refresh the list
}

CollabManagementPopup* CollabManagementPopup::create(int levelID) {
    auto ret = new CollabManagementPopup();
    if (ret && ret->init(levelID)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// TransferConfirmPopup Implementation
bool TransferConfirmPopup::init(int levelID, int targetAccountID, const std::string& levelName) {
    if (!Popup::init(300.f, 200.f))
        return false;
    
    m_data = std::make_tuple(levelID, targetAccountID, levelName);
    
    this->setTitle("Transfer Level");
    
    auto label = CCLabelBMFont::create("Do you really wanna transfer the level?", "bigFont.fnt");
    label->setPosition({150.f, 120.f});
    m_mainLayer->addChild(label);
    
    auto nameLabel = CCLabelBMFont::create(levelName.c_str(), "goldFont.fnt");
    nameLabel->setScale(0.7f);
    nameLabel->setPosition({150.f, 90.f});
    m_mainLayer->addChild(nameLabel);
    
    // Yes/No buttons
    auto yesBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Yes", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(TransferConfirmPopup::onConfirm)
    );
    yesBtn->setPosition({100.f, 50.f});
    
    auto noBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("No", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(TransferConfirmPopup::onCancel)
    );
    noBtn->setPosition({200.f, 50.f});
    
    auto menu = CCMenu::create();
    menu->addChild(yesBtn);
    menu->addChild(noBtn);
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);
    
    return true;
}

void TransferConfirmPopup::onConfirm(CCObject* sender) {
    auto levelID = std::get<0>(m_data);
    auto targetAccountID = std::get<1>(m_data);
    auto levelName = std::get<2>(m_data);
    auto accountID = GJAccountManager::get()->m_accountID;
    
    CollabManager::get()->requestTransfer(levelID, accountID, targetAccountID, levelName);
    FLAlertLayer::create("Transfer Requested", "Transfer request sent to the player.", "OK")->show();
    this->onClose(nullptr);
}

void TransferConfirmPopup::onCancel(CCObject* sender) {
    this->onClose(nullptr);
}

TransferConfirmPopup* TransferConfirmPopup::create(int levelID, int targetAccountID, const std::string& levelName) {
    auto ret = new TransferConfirmPopup();
    if (ret && ret->init(levelID, targetAccountID, levelName)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// TransferRequestPopup Implementation
bool TransferRequestPopup::init(const TransferRequest& request) {
    if (!Popup::init(300.f, 200.f))
        return false;
    
    m_data = request;
    
    this->setTitle("Transfer Request");
    
    auto label = CCLabelBMFont::create(("Transfer request for " + request.levelName).c_str(), "bigFont.fnt");
    label->setPosition({150.f, 120.f});
    m_mainLayer->addChild(label);
    
    auto fromLabel = CCLabelBMFont::create(("From: " + std::to_string(request.fromAccountID)).c_str(), "chatFont.fnt");
    fromLabel->setScale(0.5f);
    fromLabel->setPosition({150.f, 90.f});
    m_mainLayer->addChild(fromLabel);
    
    // Accept/Decline buttons
    auto acceptBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Accept", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(TransferRequestPopup::onAccept)
    );
    acceptBtn->setPosition({100.f, 50.f});
    
    auto declineBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Decline", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(TransferRequestPopup::onDecline)
    );
    declineBtn->setPosition({200.f, 50.f});
    
    auto menu = CCMenu::create();
    menu->addChild(acceptBtn);
    menu->addChild(declineBtn);
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);
    
    return true;
}

void TransferRequestPopup::onAccept(CCObject* sender) {
    auto request = m_data;
    CollabManager::get()->acceptTransfer(request);
    FLAlertLayer::create("Transfer Accepted", "You are now the host of this level!", "OK")->show();
    this->onClose(nullptr);
}

void TransferRequestPopup::onDecline(CCObject* sender) {
    auto request = m_data;
    CollabManager::get()->declineTransfer(request);
    FLAlertLayer::create("Transfer Declined", "Transfer request has been declined.", "OK")->show();
    this->onClose(nullptr);
}

TransferRequestPopup* TransferRequestPopup::create(const TransferRequest& request) {
    auto ret = new TransferRequestPopup();
    if (ret && ret->init(request)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
