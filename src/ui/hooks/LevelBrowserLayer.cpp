#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCTransition.h>
#include "../ui.hpp"

class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* object) {
        if (!LevelBrowserLayer::init(object))
            return false;

        auto myLevelsMenu = this->getChildByID("my-levels-menu");
        if (!myLevelsMenu)
            return true;

        float buttonScale = 0.4f; // adjustable size value
        auto btnSprite = CCSprite::create("button2.png"_spr);
        btnSprite->setScale(buttonScale);

        auto button = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(MyLevelBrowserLayer::onMyButton)
        );
        


        auto menuPos = myLevelsMenu->getPosition();
        button->setPosition({
            menuPos.x - 5.f,
            menuPos.y + 50.f
        });


        auto menu = dynamic_cast<CCMenu*>(myLevelsMenu);
        if (menu) {
            menu->addChild(button);
        }
        return true;
    }

    void onMyButton(CCObject* sender) {
    auto collabLayer = CollabLayer::create();
    auto scene = CCScene::create();
    scene->addChild(collabLayer);
    auto transition = Transition::create(0.5f, scene, {0, 0, 0});
    CCDirector::sharedDirector()->pushScene(transition);
}
};