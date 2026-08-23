#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class SettingsLayer : public CCLayer {
protected:
    bool init() override;
    void onBack(CCObject*);
public:
    static SettingsLayer* create();
};


class Panel : public CCNode {
public:
    static Panel* create(
        const char* title,
        CCSize size
    );

    bool init(
        const char* title,
        CCSize size
    );

    CCNode* getContent() {
        return m_content;
    }

private:
    CCScale9Sprite* m_background;
    CCLabelBMFont* m_title;
    CCNode* m_content;
};