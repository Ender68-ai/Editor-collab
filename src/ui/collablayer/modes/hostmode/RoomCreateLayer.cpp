#include <Geode/Geode.hpp>

#include <fmt/format.h>

#include "RoomCreateLayer.hpp"
#include "SessionManager.hpp"
#include "P2PManager.hpp"


using namespace geode::prelude;



RoomCreateLayer* RoomCreateLayer::create(std::function<void()> onRoomCreated) {
    auto ret = new RoomCreateLayer();
    ret->m_onRoomCreated = std::move(onRoomCreated);

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}


bool RoomCreateLayer::init() {
    if (!CCNode::init())
        return false;

    auto& session = mpedit::SessionManager::get();
    if (session.isInSession()) {
        if (m_onRoomCreated) {
            m_onRoomCreated();
        }
        return true;
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto panelWidth = winSize.width * 0.8f;
    auto panelHeight = winSize.height * 0.7f;

    auto panel = NineSliceBox::create(panelWidth, panelHeight);
    panel->setPosition({winSize.width * 0.1f, winSize.height * 0.1f});
    m_createRoomLayer = panel;

    auto boxTitle = CCLabelBMFont::create("Create A Room", "goldFont.fnt");
    boxTitle->setScale(0.6f);
    boxTitle->setPosition({panelWidth * 0.5f, panelHeight - 10.f});
    panel->addChild(boxTitle);
    m_boxTitle = boxTitle;


    auto layoutNode = CCNode::create();
    layoutNode->setContentSize({260.f, 190.f});
    layoutNode->setPosition({panelWidth * 0.5f, panelHeight * 0.52f});
    layoutNode->setAnchorPoint({0.5f, 0.5f});
    layoutNode->setLayout(ColumnLayout::create()->setGap(12.f)->setAxisReverse(true));
    m_layoutNode = layoutNode;

    auto createLabeledInput = [](CCNode* parent, const char* labelStr, float width, const char* placeholder, int maxLen, geode::CommonFilter filter, geode::TextInput*& outInput) {
        auto wrapper = CCNode::create();
        wrapper->setContentSize({width, 45.f});
        wrapper->setLayout(ColumnLayout::create()->setAxisReverse(true)->setGap(4.f));

        auto label = CCLabelBMFont::create(labelStr, "goldFont.fnt");
        label->setScale(0.5f);
        wrapper->addChild(label);

        outInput = geode::TextInput::create(width, placeholder, "chatFont.fnt");
        outInput->setMaxCharCount(maxLen);
        outInput->setCommonFilter(filter);
        wrapper->addChild(outInput);
        wrapper->updateLayout();
        parent->addChild(wrapper);
    };

    createLabeledInput(layoutNode, "Room Name", 240.f, "Room Name", 32, geode::CommonFilter::Any, m_nameInput);
    m_nameInput->setString(fmt::format("{}'s room", GJAccountManager::sharedState()->m_username));

    auto inputRow = CCNode::create();
    inputRow->setContentSize({240.f, 45.f});
    inputRow->setLayout(RowLayout::create()->setGap(15.f));
    createLabeledInput(inputRow, "Passcode", 112.5f, "(none)", 11, geode::CommonFilter::Uint, m_passInput);
    createLabeledInput(inputRow, "Player limit", 112.5f, "Unlimited", 6, geode::CommonFilter::Uint, m_limitInput);
    inputRow->updateLayout();
    layoutNode->addChild(inputRow);

    auto createToggleRow = [](CCNode* parent, const char* labelStr, CCMenuItemToggler*& outToggle) {
        auto row = CCNode::create();
        row->setContentSize({240.f, 30.f});
        row->setLayout(RowLayout::create()->setGap(8.f)->setAxisAlignment(AxisAlignment::Center));

        auto label = CCLabelBMFont::create(labelStr, "goldFont.fnt");
        label->setScale(0.4f);
        row->addChild(label);

        auto onSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        auto offSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        onSpr->setScale(0.6f);
        offSpr->setScale(0.6f);
        outToggle = CCMenuItemToggler::create(offSpr, onSpr, parent, nullptr);

        auto menu = CCMenu::create();
        menu->setContentSize({30.f, 30.f});
        outToggle->setPosition({15.f, 15.f});
        menu->addChild(outToggle);
        row->addChild(menu);
        row->updateLayout();
        parent->addChild(row);
    };

    createToggleRow(layoutNode, "Private Room", m_privateToggle);
    createToggleRow(layoutNode, "Default View-Only", m_viewOnlyToggle);
    layoutNode->updateLayout();
    m_layoutNode = layoutNode;
    panel->addChild(layoutNode);

    auto createButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Create", "goldFont.fnt", "GJ_button_01.png", 0.8f),
        this,
        menu_selector(RoomCreateLayer::onCreate)
    );
    auto createMenu = CCMenu::create();
    createMenu->setPosition({panelWidth * 0.5f, 10.f});
    createMenu->addChild(createButton);
    panel->addChild(createMenu);
    m_createMenu = createMenu;

    this->addChild(panel);



    // wHEN ROOM IS CREATED

    auto* hostingTitle = CCLabelBMFont::create("", "goldFont.fnt");
    hostingTitle->setScale(0.6f);
    hostingTitle->setPosition({panelWidth * 0.25f, panelHeight - 10.f});
    hostingTitle->setVisible(false);
    panel->addChild(hostingTitle);
    m_hostingTitle = hostingTitle;





    auto self = this;
    self->retain();
    mpedit::P2PManager::get().onSessionStarted(
        [self](std::string const& roomCode, int localPlayerId) {
            if (!self->m_createRoomLayer) {
                self->release();
                return;
            }

            self->onSessionStarted(roomCode, localPlayerId);
            self->release();
        }
    );
    return true;
}

void RoomCreateLayer::onCreate(CCObject*) {
    mpedit::RoomSettings settings;
    settings.roomName = m_nameInput->getString();
    if (settings.roomName.empty())
        settings.roomName = fmt::format("{}'s room", GJAccountManager::sharedState()->m_username);

    std::string limit = m_limitInput->getString();
    if (!limit.empty()) {
        // @geode-ignore(geode-alternative)
        settings.playerLimit = std::stoi(limit);
    } else {
        settings.playerLimit = 0;
    }

    settings.password = m_passInput->getString();
    settings.isPrivate = m_privateToggle->isToggled();
    settings.defaultViewOnly = m_viewOnlyToggle->isToggled();
    mpedit::SessionManager::get().hostSession(
        Mod::get()->getSettingValue<std::string>("player-name"),
        settings
    );
}

void RoomCreateLayer::onSessionStarted(std::string const& roomCode, int playerId) {
    if (!m_createRoomLayer || !m_boxTitle || !m_layoutNode || !m_createMenu || !m_hostingTitle) {
        return;
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    log::info("Room created: {}", roomCode);

    m_hostingTitle->setString(
    m_nameInput->getString().c_str()
    );

    m_createRoomLayer->animateResize( winSize.width * 0.4f, winSize.height * 0.7f, 0.5f);

    m_boxTitle->setVisible(false);
    m_layoutNode->setVisible(false);
    m_createMenu->setVisible(false);
    m_hostingTitle->setVisible(true);

    if (m_onRoomCreated) {
        m_onRoomCreated();
    }
}



