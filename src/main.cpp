#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "ChatLayer.hpp"
#include "NetworkingManager.hpp"

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lek) {
        if (!EditorUI::init(lek)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        
        // Attach the hidden sidebar
        auto chatLayer = ChatLayer::create();
        chatLayer->setID("collab-chat-layer"_spr);
        this->addChild(chatLayer, 1000);

        // Setup the UI Button
        auto btnSprite = CCSprite::create("logo.png"_spr);
        if (!btnSprite) btnSprite = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        btnSprite->setScale(0.6f);

        auto btn = CCMenuItemSpriteExtra::create(
            btnSprite, this, menu_selector(MyEditorUI::onToggleChat)
        );

        // Setup the Notification Badge
        auto badge = CCLabelBMFont::create("0", "chatFont.fnt");
        badge->setScale(0.5f);
        badge->setPosition({ btnSprite->getContentSize().width - 5, btnSprite->getContentSize().height - 5 });
        badge->setColor({ 255, 50, 50 });
        badge->setID("chat-badge"_spr);
        badge->setVisible(false);
        btnSprite->addChild(badge);

        auto menu = CCMenu::create();
        menu->addChild(btn);
        menu->setPosition({ winSize.width - 30.f, winSize.height - 30.f });
        this->addChild(menu, 101);

        return true;
    }

    void onToggleChat(CCObject* sender) {
        if (auto chat = typeinfo_cast<ChatLayer*>(this->getChildByIDRecursive("collab-chat-layer"_spr))) {
            chat->toggle();
        }
    }
};