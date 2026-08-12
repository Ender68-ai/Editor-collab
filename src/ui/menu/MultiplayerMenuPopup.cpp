#include "MultiplayerMenuPopup.hpp"
#include "CreateRoomPopup.hpp"
#include "../../P2PManager.hpp"
#include "../../BinaryProtocol.hpp"
#include "../core/BasePopup.hpp"
#include "../UpdateHelperNode.hpp"
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace mpedit {

    // =========================================================================
    // =========================================================================

    class PasswordPopup : public BasePopup {
    protected:
        geode::TextInput* m_input = nullptr;
        P2PManager::RoomInfo m_room;
        MultiplayerMenuPopup* m_parent = nullptr;

        bool init(P2PManager::RoomInfo const& room, MultiplayerMenuPopup* parent) {
            if (!BasePopup::init(260.f, 160.f)) return false;
            m_room = room;
            m_parent = parent;
            this->setTitle("Enter Password");

            m_input = geode::TextInput::create(200.f, "Password", "chatFont.fnt");
            m_input->setPosition(this->center());
            m_input->setCommonFilter(geode::CommonFilter::Any);
            m_mainLayer->addChild(m_input);

            auto joinBtnSprite = ButtonSprite::create("Join", "goldFont.fnt", "GJ_button_01.png", 0.6f);
            auto joinBtn = CCMenuItemSpriteExtra::create(joinBtnSprite, this, menu_selector(PasswordPopup::onJoin));
            joinBtn->setPosition(this->fromBottom(25.f));
            m_uiMenu->addChild(joinBtn);

            return true;
        }

        void onJoin(cocos2d::CCObject*) {
            std::string pwd = m_input->getString();
            SessionManager::get().joinSession(m_room.roomCode, GJAccountManager::sharedState()->m_username, pwd);
            if (m_parent) m_parent->onConnecting();
            this->onClose(nullptr);
        }

    public:
        static PasswordPopup* create(P2PManager::RoomInfo const& room, MultiplayerMenuPopup* parent) {
            auto ret = new PasswordPopup();
            if (ret->init(room, parent)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    // =========================================================================
    // =========================================================================

    class PlayerControlsPopup : public BasePopup {
    protected:
        PlayerInfo m_player;
        
        bool init(PlayerInfo const& p) {
            if (!BasePopup::init(240.f, 200.f)) return false;
            m_player = p;
            this->setTitle("Player Controls");
            
            auto layoutNode = CCMenu::create();
            layoutNode->setContentSize({200.f, 120.f});
            layoutNode->setPosition(this->center() + cocos2d::CCPoint{0.f, -10.f});
            layoutNode->setLayout(ColumnLayout::create()->setGap(10.f)->setAxisReverse(true));
            m_mainLayer->addChild(layoutNode);
            
            auto viewSprite = ButtonSprite::create(m_player.isViewOnly ? "Remove View-Only" : "Make View-Only", "goldFont.fnt", "GJ_button_01.png", 0.6f);
            auto viewBtn = CCMenuItemSpriteExtra::create(viewSprite, this, menu_selector(PlayerControlsPopup::onToggleViewOnly));
            layoutNode->addChild(viewBtn);
            
            auto kickSprite = ButtonSprite::create("Kick", "goldFont.fnt", "GJ_button_06.png", 0.6f);
            auto kickBtn = CCMenuItemSpriteExtra::create(kickSprite, this, menu_selector(PlayerControlsPopup::onKick));
            layoutNode->addChild(kickBtn);
            
            auto banSprite = ButtonSprite::create("Ban", "goldFont.fnt", "GJ_button_06.png", 0.6f);
            auto banBtn = CCMenuItemSpriteExtra::create(banSprite, this, menu_selector(PlayerControlsPopup::onBan));
            layoutNode->addChild(banBtn);
            
            layoutNode->updateLayout();
            
            return true;
        }

        void sendAction(proto::Opcode op, bool hasBool = false, bool boolVal = false) {
            std::vector<uint8_t> data;
            data.push_back(static_cast<uint8_t>(op));
            proto::Writer writer;
            writer.writeU32(m_player.id);
            if (hasBool) writer.writeBool(boolVal);
            auto p = writer.data();
            data.insert(data.end(), p.begin(), p.end());
            P2PManager::get().send(data, ChannelType::Reliable);
            this->onClose(nullptr);
        }

        void onToggleViewOnly(CCObject*) {
            bool newValue = !m_player.isViewOnly;
            sendAction(proto::Opcode::SetViewOnly, true, newValue);
            SessionManager::get().setPlayerViewOnly(m_player.id, newValue);
            if (MultiplayerMenuPopup::s_instance) MultiplayerMenuPopup::s_instance->setupActiveSession();
        }
        void onKick(CCObject*) { 
            sendAction(proto::Opcode::KickPlayer); 
            int id = m_player.id;
            std::thread([id]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                geode::queueInMainThread([id]() {
                    P2PManager::get().disconnectPeer(id);
                });
            }).detach();
        }
        void onBan(CCObject*) { 
            sendAction(proto::Opcode::BanPlayer); 
            P2PManager::get().banPlayer(m_player.name);
            int id = m_player.id;
            std::thread([id]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                geode::queueInMainThread([id]() {
                    P2PManager::get().disconnectPeer(id);
                });
            }).detach();
        }
        
    public:
        static PlayerControlsPopup* create(PlayerInfo const& p) {
            auto ret = new PlayerControlsPopup();
            if (ret->init(p)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    class RoomCell : public cocos2d::CCNode {
    protected:
        P2PManager::RoomInfo m_info;
        MultiplayerMenuPopup* m_parentPopup = nullptr;

        bool init(P2PManager::RoomInfo const& info, MultiplayerMenuPopup* parent, float width) {
            if (!CCNode::init()) return false;
            m_info = info;
            m_parentPopup = parent;
            this->setContentSize({width, 45.f});

            auto bg = CCScale9Sprite::create("square02_small.png");
            bg->setContentSize(this->getContentSize());
            bg->setAnchorPoint({0, 0});
            bg->setOpacity(90);
            bg->setColor({0, 0, 0});
            this->addChild(bg);

            std::string nameStr;
            if (info.playerLimit == 0) {
                nameStr = fmt::format("{} ({})", info.roomName, info.playerCount);
            } else {
                nameStr = fmt::format("{} ({}/{})", info.roomName, info.playerCount, info.playerLimit);
            }
            auto nameLabel = CCLabelBMFont::create(nameStr.c_str(), "bigFont.fnt");
            nameLabel->setAnchorPoint({0, 0.5f});
            nameLabel->setPosition({12.f, 30.f});
            nameLabel->setScale(0.45f);
            this->addChild(nameLabel);

            auto hostStr = fmt::format("Host: {}", info.hostName);
            auto hostLabel = CCLabelBMFont::create(hostStr.c_str(), "goldFont.fnt");
            hostLabel->setAnchorPoint({0, 0.5f});
            hostLabel->setPosition({12.f, 13.f});
            hostLabel->setScale(0.45f);
            this->addChild(hostLabel);
            
            if (!info.description.empty()) {
                auto descLabel = CCLabelBMFont::create(info.description.c_str(), "chatFont.fnt");
                descLabel->setAnchorPoint({0, 0.5f});
                descLabel->setScale(0.45f);
                descLabel->setPosition({width * 0.45f, 22.5f});
                descLabel->setColor({200, 200, 200});
                
                if (descLabel->getContentSize().width * descLabel->getScale() > width * 0.35f) {
                    descLabel->limitLabelWidth(width * 0.35f, 0.45f, 0.1f);
                }
                
                this->addChild(descLabel);
            }

            if (info.hasPassword) {
                auto lock = CCSprite::createWithSpriteFrameName("GJ_lock_001.png");
                lock->setScale(0.5f);
                lock->setPosition({width - 70.f, 22.5f});
                this->addChild(lock);
            }

            auto menu = CCMenu::create();
            menu->setPosition({width - 45.f, 22.5f});
            this->addChild(menu);

            auto joinBtnSprite = ButtonSprite::create("Join", "goldFont.fnt", "GJ_button_01.png", 0.8f);
            joinBtnSprite->setScale(0.55f);
            auto joinBtn = CCMenuItemSpriteExtra::create(joinBtnSprite, this, menu_selector(RoomCell::onJoin));
            menu->addChild(joinBtn);

            return true;
        }

        void onJoin(CCObject*) {
            if (m_parentPopup) {
                m_parentPopup->onJoinRoom(m_info);
            }
        }

    public:
        static RoomCell* create(P2PManager::RoomInfo const& info, MultiplayerMenuPopup* parent, float width) {
            auto ret = new RoomCell();
            if (ret->init(info, parent, width)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    class ActivePlayerCell : public cocos2d::CCNode {
    protected:
        bool init(PlayerInfo const& info, float width, cocos2d::ccColor3B nameColor) {
            if (!CCNode::init()) return false;
            this->setContentSize({width, 30.f});

            auto bg = CCScale9Sprite::create("square02_small.png");
            bg->setContentSize({width, 30.f});
            bg->setAnchorPoint({0, 0});
            bg->setOpacity(90);
            bg->setColor({0, 0, 0});
            this->addChild(bg);

            auto name = CCLabelBMFont::create(info.name.c_str(), "bigFont.fnt");
            name->setAnchorPoint({0, 0.5f});
            name->setPosition({10.f, 15.f});
            name->setScale(0.4f);
            name->setColor(nameColor);
            this->addChild(name);
            
            if (info.isViewOnly) {
                auto viewOnlyLabel = CCLabelBMFont::create("[View Only]", "chatFont.fnt");
                viewOnlyLabel->setScale(0.35f);
                viewOnlyLabel->setColor({255, 200, 100});
                viewOnlyLabel->setPosition({width - 80.f, 15.f});
                this->addChild(viewOnlyLabel);
            }
            
            if (SessionManager::get().getRole() == SessionManager::Role::Host && info.id != SessionManager::get().getLocalPlayerId()) {
                auto menu = CCMenu::create();
                menu->setContentSize(this->getContentSize());
                menu->setPosition(this->getContentSize() / 2.f);
                this->addChild(menu);
                
                auto gear = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
                gear->setScale(0.5f);
                auto btn = CCMenuItemSpriteExtra::create(gear, this, menu_selector(ActivePlayerCell::onOptions));
                btn->setPosition(CCPoint{width - 20.f, 15.f} - menu->getContentSize() / 2.f);
                btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(info.id)));
                menu->addChild(btn);
            }
            
            return true;
        }
        
        void onOptions(CCObject* sender) {
            auto btn = static_cast<CCNode*>(sender);
            int id = static_cast<int>(reinterpret_cast<uintptr_t>(btn->getUserData()));
            if (auto p = SessionManager::get().getPlayer(id)) {
                auto popup = PlayerControlsPopup::create(*p);
                if (popup) popup->show();
            }
        }
        
    public:
        static ActivePlayerCell* create(PlayerInfo const& info, float width, cocos2d::ccColor3B nameColor = {255, 255, 255}) {
            auto ret = new ActivePlayerCell();
            if (ret->init(info, width, nameColor)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    // =========================================================================
    // =========================================================================

    MultiplayerMenuPopup* MultiplayerMenuPopup::create() {
        auto ret = new MultiplayerMenuPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool MultiplayerMenuPopup::init() {
        if (!BasePopup::init(420.f, 280.f)) return false;
        s_instance = this;
        
        this->setTitle("Multiplayer");

        m_centerNode = CCNode::create();
        m_centerNode->setContentSize({350.f, 180.f});
        m_centerNode->setPosition(this->fromTop(45.f));
        m_centerNode->setAnchorPoint({0.5f, 1.0f});
        m_mainLayer->addChild(m_centerNode);
        
        auto centerBg = CCScale9Sprite::create("square02_small.png");
        centerBg->setContentSize(m_centerNode->getContentSize());
        centerBg->setPosition(m_centerNode->getContentSize() / 2.f);
        centerBg->setOpacity(75);
        m_centerNode->addChild(centerBg, -1);

        m_sessionUiNode = CCNode::create();
        m_sessionUiNode->setContentSize(m_mainLayer->getContentSize());
        m_sessionUiNode->setPosition({0, 0});
        m_mainLayer->addChild(m_sessionUiNode);

        this->setupMenus();

        if (SessionManager::get().isInSession()) {
            this->setupActiveSession();
        } else {
            this->setupRoomBrowser();
        }

        auto* helper = UpdateHelperNode::create([](float dt) {
            P2PManager::get().dispatchMessages();
        }, 0.05f);
        if (helper) {
            this->addChild(helper);
        }

        SessionManager::get().onSessionStarted(this, [this]() {
            auto& session = SessionManager::get();
            if (session.getRole() == SessionManager::Role::Client) {
                this->createLoadingView("Waiting for host to enter the level...");
            } else {
                this->setupActiveSession();
                geode::Notification::create("Session started!", geode::NotificationIcon::Success)->show();
            }
        });
        SessionManager::get().onPlayerJoined(this, [this](PlayerInfo const&) {
            if (!m_isConnecting) {
                this->setupActiveSession();
            }
        });
        SessionManager::get().onPlayerLeft(this, [this](PlayerInfo const&) {
            if (!m_isConnecting) {
                this->setupActiveSession();
            }
        });
        SessionManager::get().onStatus(this, [this](std::string const& status) {
            this->updateStatus(status);
        });
        SessionManager::get().onError(this, [this](std::string const& error) {
            geode::queueInMainThread([this, error]() {
                this->setupRoomBrowser();
                FLAlertLayer::create("Error", error, "OK")->show();
            });
        });

        return true;
    }

    MultiplayerMenuPopup::~MultiplayerMenuPopup() {
        auto& session = SessionManager::get();
        if (session.isInSession() && session.getRole() == SessionManager::Role::Client && !LevelEditorLayer::get()) {
            geode::log::info("MultiplayerMenuPopup: Leaving session because popup was closed during sync");
            session.leaveSession();
        }
        session.removeListener(this);
        if (s_instance == this) s_instance = nullptr;
    }

    void MultiplayerMenuPopup::setupMenus() {
        auto topMenu = CCMenu::create();
        topMenu->setContentSize({50.f, 40.f});
        topMenu->setPosition(this->fromTopRight(15.f, 15.f));
        topMenu->setAnchorPoint({1.f, 1.f});
        topMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::End));
        m_uiMenu->addChild(topMenu);

        auto patreonSprite = ButtonSprite::create("Patreon", "goldFont.fnt", "GJ_button_02.png", 0.8f);
        patreonSprite->setScale(0.6f);
        auto patreonBtn = CCMenuItemSpriteExtra::create(patreonSprite, this, menu_selector(MultiplayerMenuPopup::onPatreon));
        topMenu->addChild(patreonBtn);
        topMenu->updateLayout();

        m_browserUiNode = CCNode::create();
        m_mainLayer->addChild(m_browserUiNode);

        auto centerMenu = CCMenu::create();
        centerMenu->setContentSize({200.f, 40.f});
        centerMenu->setPosition(this->fromBottom(25.f));
        centerMenu->setAnchorPoint({0.5f, 0.5f});
        centerMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center)->setGap(8.f));
        m_browserUiNode->addChild(centerMenu);

        m_codeInput = geode::TextInput::create(130.f, "Code", "chatFont.fnt");
        m_codeInput->setCommonFilter(geode::CommonFilter::Alphanumeric);
        centerMenu->addChild(m_codeInput);

        auto joinSprite = ButtonSprite::create("Join", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        joinSprite->setScale(0.65f);
        auto joinBtn = CCMenuItemSpriteExtra::create(joinSprite, this, menu_selector(MultiplayerMenuPopup::onJoinByCode));
        centerMenu->addChild(joinBtn);
        centerMenu->updateLayout();

        auto rightMenu = CCMenu::create();
        rightMenu->setContentSize({50.f, 40.f});
        rightMenu->setPosition(this->fromBottomRight(18.f, 15.f));
        rightMenu->setAnchorPoint({1.f, 0.f});
        rightMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::End));
        m_browserUiNode->addChild(rightMenu);

        auto refreshSprite = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
        refreshSprite->setScale(0.85f);
        auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSprite, this, menu_selector(MultiplayerMenuPopup::onRefresh));
        rightMenu->addChild(refreshBtn);
        rightMenu->updateLayout();
    }

    void MultiplayerMenuPopup::clearCenter() {
        m_statusLabel = nullptr;
        m_scrollLayer = nullptr;
        m_centerNode->removeAllChildren();
        if (m_sessionUiNode) m_sessionUiNode->removeAllChildren();

        auto centerBg = CCScale9Sprite::create("square02_small.png");
        centerBg->setContentSize(m_centerNode->getContentSize());
        centerBg->setPosition(m_centerNode->getContentSize() / 2.f);
        centerBg->setOpacity(75);
        m_centerNode->addChild(centerBg, -1);
    }

    void MultiplayerMenuPopup::setupRoomBrowser() {
        this->clearCenter();
        m_isConnecting = false;
        if (m_browserUiNode) m_browserUiNode->setVisible(true);

        m_statusLabel = CCLabelBMFont::create("Fetching rooms...", "chatFont.fnt");
        m_statusLabel->setPosition(m_centerNode->getContentSize() / 2.f);
        m_statusLabel->setScale(0.6f);
        m_centerNode->addChild(m_statusLabel);

        m_scrollLayer = ScrollLayer::create(m_centerNode->getContentSize());
        m_scrollLayer->setPosition({0, 0});
        m_scrollLayer->m_contentLayer->setLayout(ColumnLayout::create()->setGap(0.f)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End));
        m_centerNode->addChild(m_scrollLayer);

        auto borders = ListBorders::create();
        borders->setContentSize(m_scrollLayer->getContentSize());
        borders->setPosition(m_scrollLayer->getContentSize() / 2.f);
        m_centerNode->addChild(borders);

        this->fetchRooms();
    }

    void MultiplayerMenuPopup::fetchRooms() {
        geode::Ref<MultiplayerMenuPopup> safeThis = this;
        P2PManager::get().fetchRooms([safeThis](std::vector<P2PManager::RoomInfo> const& rooms) {
            if (safeThis->getParent()) {
                safeThis->populateRooms(rooms);
                if (rooms.empty() && safeThis->m_statusLabel) {
                    safeThis->m_statusLabel->setVisible(true);
                    safeThis->m_statusLabel->setString("No rooms found");
                }
            }
        });
    }

    void MultiplayerMenuPopup::populateRooms(std::vector<P2PManager::RoomInfo> const& rooms) {
        if (!m_scrollLayer) return;
        m_scrollLayer->m_contentLayer->removeAllChildren();
        if (m_statusLabel) m_statusLabel->setVisible(rooms.empty());

        for (auto const& r : rooms) {
            auto cell = RoomCell::create(r, this, m_scrollLayer->getContentSize().width);
            m_scrollLayer->m_contentLayer->addChild(cell);
        }
        m_scrollLayer->m_contentLayer->updateLayout();
        m_scrollLayer->m_contentLayer->setPositionY(
            std::max(0.f, m_scrollLayer->getContentSize().height - m_scrollLayer->m_contentLayer->getContentSize().height)
        );
        geode::cocos::handleTouchPriority(this);
    }

    void MultiplayerMenuPopup::onRefresh(CCObject*) {
        if (m_scrollLayer) {
            m_scrollLayer->m_contentLayer->removeAllChildren();
            if (m_statusLabel) {
                m_statusLabel->setVisible(true);
                m_statusLabel->setString("Fetching rooms...");
            }
            fetchRooms();
        }
    }

    void MultiplayerMenuPopup::onJoinByCode(cocos2d::CCObject*) {
        if (!m_codeInput) return;
        std::string code = m_codeInput->getString();
        if (code.empty()) {
            geode::Notification::create("Please enter a room code", geode::NotificationIcon::Warning)->show();
            return;
        }

        m_lastJoinCode = code;
        SessionManager::get().joinSession(code, GJAccountManager::sharedState()->m_username, "");
        this->onConnecting();
    }

    void MultiplayerMenuPopup::onHostForm(cocos2d::CCObject*) {
        CreateRoomPopup::create(this)->show();
    }

    void MultiplayerMenuPopup::onConnecting() {
        this->clearCenter();
        if (m_browserUiNode) m_browserUiNode->setVisible(false);
        m_isConnecting = true;

        auto size = m_centerNode->getContentSize();
        auto center = size / 2.f;

        auto spinner = CCSprite::create("loadingCircle.png");
        if (spinner) {
            spinner->setScale(0.7f);
            spinner->setPosition({center.width, center.height + 15.f});
            spinner->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
            spinner->runAction(CCRepeatForever::create(CCRotateBy::create(1.0f, 360.f)));
            m_centerNode->addChild(spinner);
        }

        m_statusLabel = CCLabelBMFont::create("Connecting...", "chatFont.fnt");
        m_statusLabel->setPosition({center.width, center.height - 30.f});
        m_statusLabel->setScale(0.6f);
        m_statusLabel->setColor({255, 255, 100});
        m_centerNode->addChild(m_statusLabel);

        auto cancelSprite = ButtonSprite::create("Cancel", "goldFont.fnt", "GJ_button_06.png", 0.8f);
        cancelSprite->setScale(0.65f);
        auto cancelBtn = CCMenuItemSpriteExtra::create(cancelSprite, this, menu_selector(MultiplayerMenuPopup::onLeave));
        auto cancelMenu = CCMenu::create();
        cancelMenu->setPosition(this->fromBottom(25.f));
        cancelMenu->addChild(cancelBtn);
        m_sessionUiNode->addChild(cancelMenu);

        geode::cocos::handleTouchPriority(this);
    }

    void MultiplayerMenuPopup::updateStatus(std::string const& status) {
        if (m_statusLabel) {
            m_statusLabel->setString(status.c_str());
        }
    }

    void MultiplayerMenuPopup::setupActiveSession() {
        this->clearCenter();
        if (m_browserUiNode) m_browserUiNode->setVisible(false);
        m_isConnecting = false;
        auto& session = SessionManager::get();

        auto size = m_centerNode->getContentSize();
        float w = size.width;
        float h = size.height;

        auto topRow = CCNode::create();
        topRow->setContentSize({w, 25.f});
        topRow->setPosition({w / 2.f, h - 15.f});
        topRow->setAnchorPoint({0.5f, 0.5f});
        topRow->setLayout(RowLayout::create()->setGap(10.f));
        m_centerNode->addChild(topRow);
        
        auto codeLbl = CCLabelBMFont::create(fmt::format("Code: {}", session.getRoomCode()).c_str(), "goldFont.fnt");
        codeLbl->setScale(0.55f);
        topRow->addChild(codeLbl);
        
        auto copySprite = ButtonSprite::create("Copy", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        copySprite->setScale(0.45f);
        auto copyBtn = CCMenuItemSpriteExtra::create(copySprite, this, menu_selector(MultiplayerMenuPopup::onCopyCode));
        auto copyMenu = CCMenu::create();
        copyMenu->setContentSize(copyBtn->getScaledContentSize());
        copyBtn->setPosition(copyMenu->getContentSize() / 2.f);
        copyMenu->addChild(copyBtn);
        topRow->addChild(copyMenu);
        
        topRow->updateLayout();

        auto roleStr = session.getRole() == SessionManager::Role::Host ? "You are the Host" : "You are a Guest";
        auto roleColor = session.getRole() == SessionManager::Role::Host ? ccColor3B{180, 255, 180} : ccColor3B{100, 200, 255};
        auto roleLabel = CCLabelBMFont::create(roleStr, "bigFont.fnt");
        roleLabel->setScale(0.3f);
        roleLabel->setColor(roleColor);
        roleLabel->setPosition({w / 2.f, h - 35.f});
        m_centerNode->addChild(roleLabel);

        auto players = session.getPlayers();
        auto countStr = fmt::format("Players: {}", players.size());
        auto countLabel = CCLabelBMFont::create(countStr.c_str(), "bigFont.fnt");
        countLabel->setScale(0.3f);
        countLabel->setPosition({w / 2.f, h - 50.f});
        m_centerNode->addChild(countLabel);

        float scrollHeight = h - 65.f;
        m_scrollLayer = ScrollLayer::create({w, scrollHeight});
        m_scrollLayer->setPosition({0, 5.f});
        m_scrollLayer->m_contentLayer->setLayout(ColumnLayout::create()->setGap(2.f)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End));
        m_centerNode->addChild(m_scrollLayer);

        auto borders = ListBorders::create();
        borders->setContentSize(m_scrollLayer->getContentSize());
        borders->setPosition({w / 2.f, 5.f + scrollHeight / 2.f});
        m_centerNode->addChild(borders);

        static const std::array<ccColor3B, 6> playerColors = {
            ccColor3B{100, 200, 255},
            ccColor3B{255, 120, 100},
            ccColor3B{100, 255, 150},
            ccColor3B{255, 200, 100},
            ccColor3B{200, 150, 255},
            ccColor3B{255, 150, 200},
        };

        for (auto& p : players) {
            auto cell = ActivePlayerCell::create(p, m_scrollLayer->getContentSize().width, playerColors[p.colorIndex % playerColors.size()]);
            m_scrollLayer->m_contentLayer->addChild(cell);
        }
        m_scrollLayer->m_contentLayer->updateLayout();
        m_scrollLayer->m_contentLayer->setPositionY(
            std::max(0.f, m_scrollLayer->getContentSize().height - m_scrollLayer->m_contentLayer->getContentSize().height)
        );

        m_statusLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_statusLabel->setPosition(this->fromBottom(48.f));
        m_statusLabel->setScale(0.45f);
        m_statusLabel->setColor({200, 200, 200});
        m_sessionUiNode->addChild(m_statusLabel);
        
        if (LevelEditorLayer::get()) {
            m_statusLabel->setString("");
        } else if (session.getRole() == SessionManager::Role::Client) {
            m_statusLabel->setString("Waiting for host to enter the level...");
        } else {
            m_statusLabel->setString("Waiting for players...");
        }

        auto leaveSprite = ButtonSprite::create("Leave", "goldFont.fnt", "GJ_button_06.png", 0.8f);
        leaveSprite->setScale(0.7f);
        auto leaveBtn = CCMenuItemSpriteExtra::create(leaveSprite, this, menu_selector(MultiplayerMenuPopup::onLeave));
        auto leaveMenu = CCMenu::create();
        leaveMenu->setPosition(this->fromBottomLeft(35.f, 25.f));
        leaveMenu->addChild(leaveBtn);
        m_sessionUiNode->addChild(leaveMenu);

        geode::cocos::handleTouchPriority(this);
    }

    void MultiplayerMenuPopup::onLeave(CCObject*) {
        if (SessionManager::get().isInSession()) {
            bool isHost = SessionManager::get().getRole() == SessionManager::Role::Host;
            SessionManager::get().leaveSession();
            geode::Notification::create("Left session", geode::NotificationIcon::Info)->show();

            if (isHost) {
                CreateRoomPopup::create(nullptr)->show();
                this->onClose(nullptr);
            } else {
                this->onClose(nullptr);
                if (LevelEditorLayer::get()) {
                    auto* director = cocos2d::CCDirector::sharedDirector();
                    if (auto* runningScene = director->getRunningScene()) {
                        std::function<EditorPauseLayer*(cocos2d::CCNode*)> findPauseLayer = [&](cocos2d::CCNode* parent) -> EditorPauseLayer* {
                            if (!parent) return nullptr;
                            if (auto* pause = typeinfo_cast<EditorPauseLayer*>(parent)) {
                                return pause;
                            }
                            if (parent->getChildren()) {
                                for (auto* child : CCArrayExt<CCNode*>(parent->getChildren())) {
                                    if (auto* p = findPauseLayer(child)) return p;
                                }
                            }
                            return nullptr;
                        };

                        auto* pauseLayer = findPauseLayer(runningScene);
                        if (pauseLayer) {
                            auto* dummySender = cocos2d::CCNode::create();
                            pauseLayer->onExitEditor(dummySender);
                        } else {
                            director->popScene();
                        }
                    }
                }
            }
        } else {
            m_isConnecting = false;
            this->setupRoomBrowser();
        }
    }

    void MultiplayerMenuPopup::onPatreon(CCObject*) {
        geode::utils::web::openLinkInBrowser("https://www.patreon.com/cw/d050/membership");
    }

    void MultiplayerMenuPopup::onCopyCode(CCObject*) {
        geode::utils::clipboard::write(SessionManager::get().getRoomCode());
        geode::Notification::create("Copied!", CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png"))->show();
    }

    void MultiplayerMenuPopup::onJoinRoom(P2PManager::RoomInfo const& room) {
        m_lastJoinCode = room.roomCode;
        if (room.hasPassword) {
            promptPassword(room);
        } else {
            SessionManager::get().joinSession(room.roomCode, GJAccountManager::sharedState()->m_username, "");
            this->onConnecting();
        }
    }

    void MultiplayerMenuPopup::promptPassword(P2PManager::RoomInfo const& room) {
        if (auto p = PasswordPopup::create(room, this)) {
            p->show();
        }
    }

    void MultiplayerMenuPopup::createLoadingView(std::string const& statusText) {
        this->clearCenter();
        if (m_browserUiNode) m_browserUiNode->setVisible(false);
        m_isConnecting = true;

        auto size = m_centerNode->getContentSize();
        auto center = size / 2.f;

        auto bg = CCScale9Sprite::create("square02b_001.png");
        bg->setContentSize(size);
        bg->setPosition(center);
        bg->setColor({10, 15, 28});
        bg->setOpacity(180);
        m_centerNode->addChild(bg);

        auto title = CCLabelBMFont::create("Synchronizing Level", "goldFont.fnt");
        title->setScale(0.65f);
        title->setPosition({center.width, center.height + 50.f});
        m_centerNode->addChild(title);

        auto spinner = CCSprite::create("loadingCircle.png");
        if (spinner) {
            spinner->setScale(0.7f);
            spinner->setPosition({center.width, center.height + 5.f});
            spinner->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
            spinner->runAction(CCRepeatForever::create(CCRotateBy::create(1.0f, 360.f)));
            m_centerNode->addChild(spinner);
        }

        m_statusLabel = CCLabelBMFont::create(statusText.c_str(), "chatFont.fnt");
        m_statusLabel->setScale(0.55f);
        m_statusLabel->setPosition({center.width, center.height - 45.f});
        m_statusLabel->setColor({200, 200, 200});
        m_centerNode->addChild(m_statusLabel);

        auto cancelSprite = ButtonSprite::create("Cancel", "goldFont.fnt", "GJ_button_06.png", 0.8f);
        cancelSprite->setScale(0.65f);
        auto cancelBtn = CCMenuItemSpriteExtra::create(cancelSprite, this, menu_selector(MultiplayerMenuPopup::onLeave));
        auto cancelMenu = CCMenu::create();
        cancelMenu->setPosition(this->fromBottom(25.f));
        cancelMenu->addChild(cancelBtn);
        m_sessionUiNode->addChild(cancelMenu);

        geode::cocos::handleTouchPriority(this);
    }

}
