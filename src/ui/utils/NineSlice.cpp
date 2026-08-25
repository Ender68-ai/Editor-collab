#include <Geode/ui/NineSlice.hpp>

#include "NineSlice.hpp"

NineSliceBox* NineSliceBox::create(float width, float height) {
    auto ret = new NineSliceBox();

    if (ret && ret->init(width, height)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool NineSliceBox::init(float width, float height) {
    if (!CCNode::init()) return false;

    auto bg = geode::NineSlice::create(
        "square02b_001.png",
        {},
        {
            .top = 10.f,
            .right = 10.f,
            .bottom = 10.f,
            .left = 10.f
        }
    );

    bg->setContentSize({width, height});
    bg->setAnchorPoint({0, 0});
    bg->setOpacity(255);
    bg->setColor({139, 69, 19});
    this->addChild(bg);

    return true;
}