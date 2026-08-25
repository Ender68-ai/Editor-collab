#pragma once

#include <Geode/Geode.hpp>

class NineSliceBox : public cocos2d::CCNode {
public:

    static NineSliceBox* create(float width, float height);

private:

    bool init(float width, float height);
};
