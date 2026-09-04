#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

class UploadServerPopup : public geode::Popup {
protected:
    GJGameLevel* m_level;
    geode::TextInput* m_maxPlayersInput;
    geode::TextInput* m_passwordInput;
    CCMenuItemToggler* m_viewOnlyToggler;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_task;

    bool init(GJGameLevel* level);
    void onHost(cocos2d::CCObject* sender);

public:
    static UploadServerPopup* create(GJGameLevel* level);
};
