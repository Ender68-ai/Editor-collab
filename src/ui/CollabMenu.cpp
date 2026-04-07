// ============================================================================
// CollabMenuPopup Implementation
// TEMPORARILY DISABLED TO PREVENT TEMPLATE ERRORS
// ============================================================================

/*
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/GJGameLevel.hpp>

using namespace geode::prelude;

class CollabMenuPopup : public geode::Popup<GJGameLevel*> {
public:
    static CollabMenuPopup* create() {
        auto ret = new CollabMenuPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!geode::Popup<GJGameLevel*>::init(300.f, 200.f, nullptr))
            return false;

        this->setTitle("Collab Menu");

        // Host button
        auto hostBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Host", "bigFont.fnt", "GJ_button_01.png"_spr, 0.8f),
            this,
            menu_selector(CollabMenuPopup::onHost)
        );
        hostBtn->setPosition({150.f, 120.f});

        // Join button
        auto joinBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Join", "bigFont.fnt", "GJ_button_01.png"_spr, 0.8f),
            this,
            menu_selector(CollabMenuPopup::onJoin)
        );
        joinBtn->setPosition({150.f, 70.f});

        auto menu = CCMenu::create();
        menu->addChild(hostBtn);
        menu->addChild(joinBtn);
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        return true;
    }

    void onHost(cocos2d::CCObject* sender) {
        FLAlertLayer::create("Host", "Host functionality coming soon!", "OK")->show();
        this->onClose(nullptr);
    }

    void onJoin(cocos2d::CCObject* sender) {
        FLAlertLayer::create("Join", "Join functionality coming soon!", "OK")->show();
        this->onClose(nullptr);
    }
};
*/
