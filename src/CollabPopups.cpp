#include "CollabPopups.hpp"
#include "CollabNetworkManager.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GManager.hpp>

using namespace geode::prelude;

// ============================================================================
// CollabSetupPopup Implementation
// ============================================================================

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

// ============================================================================
// CollabManagementPopup Implementation
// ============================================================================

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
    this->setupPlayerList(); // Refresh list
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

// ============================================================================
// TransferConfirmPopup Implementation
// ============================================================================

bool TransferConfirmPopup::init(int levelID, int targetAccountID, const std::string& levelName) {
    if (!Popup::init(300.f, 200.f))
        return false;
    
    m_data = std::make_tuple(levelID, targetAccountID, levelName);
    
    this->setTitle("Transfer Level");
    
    auto label = CCLabelBMFont::create("Do you really wanna transfer level?", "bigFont.fnt");
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
    FLAlertLayer::create("Transfer Requested", "Transfer request sent to player.", "OK")->show();
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

// ============================================================================
// TransferRequestPopup Implementation
// ============================================================================

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
    FLAlertLayer::create("Transfer Accepted", "You are now host of this level!", "OK")->show();
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

// ============================================================================
// CollabSettingsPopup Implementation
// ============================================================================

bool CollabSettingsPopup::init(int levelID, const std::string& levelName) {
    if (!Popup::init(340.f, 260.f))
        return false;
    
    m_levelID = levelID;
    m_levelName = levelName;
    
    auto collabMgr = CollabManager::get();
    m_collabEnabled = collabMgr->isCollabEnabled(levelID);
    
    this->setTitle("Collab Settings");
    
    // Level name display - proper spacing from title
    auto nameLabel = CCLabelBMFont::create(levelName.c_str(), "goldFont.fnt");
    nameLabel->setScale(0.6f);
    nameLabel->setPosition({170.f, 185.f});
    m_mainLayer->addChild(nameLabel);
    
    // Separator line
    auto separator = CCDrawNode::create();
    separator->drawSegment({20.f, 170.f}, {320.f, 170.f}, 1.0f, {200, 200, 200, 255});
    m_mainLayer->addChild(separator);
    
    // "Enable Real-time Collab" label - better positioned
    auto collabLabel = CCLabelBMFont::create("Enable Real-time Collab", "bigFont.fnt");
    collabLabel->setScale(0.5f);
    collabLabel->setPosition({120.f, 135.f});
    m_mainLayer->addChild(collabLabel);
    
    // Toggle switch - positioned to the right of label
    auto toggle = CCMenuItemToggler::create(
        CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
        CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
        this,
        menu_selector(CollabSettingsPopup::onToggleRealTimeCollab)
    );
    
    if (m_collabEnabled) {
        toggle->toggle(true);
    }
    
    toggle->setPosition({280.f, 135.f});
    
    // Info text - properly spaced below
    auto infoLabel = CCLabelBMFont::create("When enabled, this level becomes", "chatFont.fnt");
    infoLabel->setScale(0.4f);
    infoLabel->setPosition({170.f, 100.f});
    m_mainLayer->addChild(infoLabel);
    
    auto infoLabel2 = CCLabelBMFont::create("your active host level for real-time", "chatFont.fnt");
    infoLabel2->setScale(0.4f);
    infoLabel2->setPosition({170.f, 85.f});
    m_mainLayer->addChild(infoLabel2);
    
    auto infoLabel3 = CCLabelBMFont::create("collaboration with other players.", "chatFont.fnt");
    infoLabel3->setScale(0.4f);
    infoLabel3->setPosition({170.f, 70.f});
    m_mainLayer->addChild(infoLabel3);
    
    // Close button - at bottom
    auto closeBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(CollabSettingsPopup::onClose)
    );
    closeBtn->setPosition({170.f, 30.f});
    
    auto menu = CCMenu::create();
    menu->addChild(toggle);
    menu->addChild(closeBtn);
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);
    
    return true;
}

void CollabSettingsPopup::onToggleRealTimeCollab(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    // Fix toggle logic: isToggled() returns true when ON (checkmark showing)
    // We want to know the NEW state after the toggle was clicked
    bool isNowEnabled = toggle->isToggled();
    
    // Safety: Check if level is valid before proceeding
    if (m_levelID <= 0) {
        log::error("Invalid level ID in onToggleRealTimeCollab: {}", m_levelID);
        return;
    }
    
    auto collabMgr = CollabManager::get();
    auto netMgr = CollabNetworkManager::get();
    auto accountMgr = GJAccountManager::get();
    
    if (isNowEnabled) {
        // Enable real-time collaboration
        collabMgr->enableCollabForLevel(m_levelID, m_levelName, accountMgr->m_accountID);
        collabMgr->setCurrentLevelID(m_levelID);
        
        log::info("Real-time collaboration ENABLED for level {} ({})", m_levelID, m_levelName);
        
        // Connect to relay server
        // TODO: Get these from configuration or use Playit.gg tunnel
        std::string tcpServer = "localhost:8080";  // Replace with actual Playit.gg domain
        std::string udpServer = "localhost:41234"; // Replace with actual Playit.gg domain
        
        // Start network connection
        bool connected = netMgr->startConnection(
            static_cast<uint32_t>(accountMgr->m_accountID),
            GJAccountManager::sharedState()->m_password, // Use GJAccountManager password field
            static_cast<uint32_t>(m_levelID),
            tcpServer,
            udpServer
        );
        
        if (connected) {
            log::info("Network connection initiated for level {}", m_levelID);
            FLAlertLayer::create("ENABLED", 
                "Real-time collaboration is now active for this level!\n"
                "Connecting to relay server...", "OK")->show();
        } else {
            log::error("Failed to initiate network connection");
            FLAlertLayer::create("ERROR", 
                "Failed to connect to relay server.\n"
                "Please check your network settings.", "OK")->show();
            
            // Disable in manager if connection failed
            collabMgr->disableCollabForLevel(m_levelID);
            toggle->toggle(false);
        }
    } else {
        // Disable real-time collaboration
        collabMgr->disableCollabForLevel(m_levelID);
        if (collabMgr->getCurrentLevelID() == m_levelID) {
            collabMgr->setCurrentLevelID(-1);
        }
        
        // Disconnect from relay server
        netMgr->stopConnection();
        
        log::info("Real-time collaboration DISABLED for level {}", m_levelID);
        FLAlertLayer::create("DISABLED", "Real-time collaboration has been turned off.", "OK")->show();
    }
    
    m_collabEnabled = isNowEnabled;
}

void CollabSettingsPopup::onClose(CCObject* sender) {
    // Prevent zombie popup and ensure clean close
    this->setKeypadEnabled(false);
    Popup::onClose(sender);
}

CollabSettingsPopup* CollabSettingsPopup::create(int levelID, const std::string& levelName) {
    auto ret = new CollabSettingsPopup();
    if (ret && ret->init(levelID, levelName)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// ============================================================================
// FirstTimeSetupPopup Implementation
// ============================================================================

bool FirstTimeSetupPopup::init() {
    if (!Popup::init(360.f, 280.f))
        return false;
    
    this->setTitle("Welcome to CollabEditor!");
    
    // Subtitle message
    auto msgLabel = CCLabelBMFont::create("Real-Time Level Collaboration Tool", "smallFont.fnt");
    msgLabel->setScale(0.65f);
    msgLabel->setPosition({180.f, 230.f});
    m_mainLayer->addChild(msgLabel);
    
    // Description
    auto descLabel = CCLabelBMFont::create(
        "Collaborate with friends in real-time:\n\n"
        "- Share levels and edit together\n"
        "- See remote cursors and locks\n"
        "- Sync object movements\n"
        "- Manage permissions per level",
        "chatFont.fnt"
    );
    descLabel->setScale(0.5f);
    descLabel->setPosition({180.f, 145.f});
    m_mainLayer->addChild(descLabel);
    
    // Draw separator line
    auto separator = CCDrawNode::create();
    separator->drawSegment({20.f, 100.f}, {340.f, 100.f}, 1.f, ccc4f(1.f, 1.f, 1.f, 0.3f));
    m_mainLayer->addChild(separator);
    
    // Info text
    auto infoLabel = CCLabelBMFont::create("Visit settings to configure your experience.", "smallFont.fnt");
    infoLabel->setScale(0.5f);
    infoLabel->setPosition({180.f, 70.f});
    m_mainLayer->addChild(infoLabel);
    
    // Menu with buttons
    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    
    // Next button
    auto nextBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Next", 100, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.6f),
        this,
        menu_selector(FirstTimeSetupPopup::onNext)
    );
    nextBtn->setPosition({120.f, 25.f});
    menu->addChild(nextBtn);
    
    // Skip button
    auto skipBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Skip", 100, true, "goldFont.fnt", "GJ_button_06.png", 30.f, 0.6f),
        this,
        menu_selector(FirstTimeSetupPopup::onSkip)
    );
    skipBtn->setPosition({240.f, 25.f});
    menu->addChild(skipBtn);
    
    m_mainLayer->addChild(menu);
    
    return true;
}

void FirstTimeSetupPopup::onNext(CCObject* sender) {
    auto collabMgr = CollabManager::get();
    collabMgr->setFirstTimeSetupComplete();
    collabMgr->saveData();
    
    // Show settings info alert (3 arguments: title, description, button)
    FLAlertLayer::create(
        "Settings Available",
        "Check Geode settings menu to configure CollabEditor options like auto-connect, cursor sync, and performance settings.",
        "OK"
    )->show();
    
    this->onClose(nullptr);
}

void FirstTimeSetupPopup::onSkip(CCObject* sender) {
    auto collabMgr = CollabManager::get();
    collabMgr->setFirstTimeSetupComplete();
    collabMgr->saveData();
    this->onClose(nullptr);
}

FirstTimeSetupPopup* FirstTimeSetupPopup::create() {
    auto ret = new FirstTimeSetupPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
