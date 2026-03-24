#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "ChatLayer.hpp"

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lek) {
        if (!EditorUI::init(lek)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        
        // Use logo.png (Geode will look in resources/root)
        auto btnSprite = CCSprite::create("logo.png");
        if (!btnSprite) btnSprite = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        btnSprite->setScale(0.5f);

        auto btn = CCMenuItemSpriteExtra::create(
            btnSprite, this, menu_selector(MyEditorUI::onOpenChat)
        );

        auto menu = CCMenu::create();
        menu->addChild(btn);
        menu->setPosition({ winSize.width - 40.f, winSize.height - 40.f });
        menu->setID("chat-button-menu"_spr);
        
        this->addChild(menu);
        return true;
    }

    void onOpenChat(CCObject*) {
        if (this->getChildByID("collab-chat-window"_spr)) return;

        auto chat = ChatLayer::create();
        chat->setID("collab-chat-window"_spr);
        this->addChild(chat, 999); 
    }
};