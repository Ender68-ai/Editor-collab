#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;

        auto winSize = CCDirector::get()->getWinSize();

        auto label = CCLabelBMFont::create("Hello, World!", "bigFont.fnt");
        label->setPosition(winSize / 2);
        this->addChild(label);

        return true;
    }
};