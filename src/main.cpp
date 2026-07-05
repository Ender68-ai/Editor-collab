#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include "ui/ui.hpp"

using namespace geode::prelude;

class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* object) {
        if (!LevelBrowserLayer::init(object))
            return false;

        auto myLevelsMenu = this->getChildByID("my-levels-menu");
        if (!myLevelsMenu)
            return true;

        auto btnSprite = CCSprite::create("button.png"_spr);

        auto button = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(MyLevelBrowserLayer::onMyButton)
        );
        

        // Position the button above the menu
        auto menuPos = myLevelsMenu->getPosition();
        button->setPosition({
            menuPos.x - 5.f,
            menuPos.y + 50.f
        });

        // Add it to the same parent as the menu
        auto menu = typeinfo_cast<CCMenu*>(myLevelsMenu);
        if (menu) {
            menu->addChild(button);
        }
        return true;
    }

    void onMyButton(CCObject* sender) {
    auto collabLayer = CollabLayer::create();
    auto scene = CCScene::create();
    scene->addChild(collabLayer);
    CCDirector::sharedDirector()->pushScene(scene);
}
};