#include <Geode/Geode.hpp>

#include "../../../P2PManager.hpp"
#include "JoinMode.hpp"
#include "../../../ui/menu/MultiplayerMenuPopup.hpp"








namespace mpedit {

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

            auto hostStr = fmt::format("Host: {} ({})", info.hostName, info.version);
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

}