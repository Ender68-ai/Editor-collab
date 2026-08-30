#include "ui/collablayer/CollabLayer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include "ui/ui.hpp"

using namespace geode::prelude;

class $modify(MPEditorPauseLayer, EditorPauseLayer) {
    void onSaveAndExit(CCObject* sender) {
        auto collabLayer = CollabLayer::create();
        auto scene = CCScene::create();
        scene->addChild(collabLayer);
        auto transition = Transition::create(0.5f, scene, {0, 0, 0});
        CCDirector::sharedDirector()->pushScene(transition);
    }
};