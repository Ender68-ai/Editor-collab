#include <Geode/Geode.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include "menu/UploadServerPopup.hpp"

using namespace geode::prelude;

class $modify(MyEditLevelLayer, EditLevelLayer) {
    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) return false;

        if (auto menu = this->getChildByID("level-edit-menu")) {
            auto sprite = CCSprite::createWithSpriteFrameName("GJ_shareBtn_001.png");
            sprite->setScale(0.65f);

            auto hostBtn = CCMenuItemSpriteExtra::create(
                sprite,
                this,
                menu_selector(MyEditLevelLayer::onHostLevel)
            );
            hostBtn->setID("cloud-host-btn"_spr);
            
            auto spacer = CCNode::create();
            spacer->setContentSize(sprite->getScaledContentSize());
            spacer->setID("cloud-host-spacer"_spr);
            
            if (menu->getChildrenCount() > 0) {
                auto firstChild = static_cast<CCNode*>(menu->getChildren()->objectAtIndex(0));
                spacer->setZOrder(firstChild->getZOrder() - 1);
            }
            
            menu->addChild(spacer);
            menu->addChild(hostBtn);
            menu->updateLayout();
        }

        return true;
    }

    void onHostLevel(CCObject* sender) {
        UploadServerPopup::create(m_level)->show();
    }
};
