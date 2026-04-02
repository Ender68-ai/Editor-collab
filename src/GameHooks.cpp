#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include "CollabManager.hpp"
#include "CollabPopups.hpp"

using namespace geode::prelude;

class $modify(CollabGameHooks, MenuLayer) {
    struct Fields {
        // Fields can be stored here if needed
    };

    bool init() {
        log::debug("MenuLayer::init called - CollabEditor hook starting");
        
        if (!MenuLayer::init()) {
            log::error("MenuLayer::init failed - aborting CollabEditor hook");
            return false;
        }

        log::debug("MenuLayer initialized successfully");
        
        // Initialize CollabManager on first game load
        auto collabMgr = CollabManager::get();
        if (!collabMgr) {
            log::error("Failed to get CollabManager instance");
            return true;
        }
        
        log::debug("CollabManager instance obtained");
        
        // Check if this is the first time setup
        if (collabMgr->isFirstTimeSetup()) {
            log::info("First time setup detected - scheduling welcome popup with 5 second delay");
            // INCREASED DELAY: 5.0f seconds to ensure everything is fully initialized
            // This overkill delay helps us diagnose if the popup is causing the crash
            this->scheduleOnce(schedule_selector(CollabGameHooks::showFirstTimeSetup), 5.0f);
        } else {
            log::debug("First time setup already completed");
        }

        log::debug("MenuLayer::init hook completed successfully");
        return true;
    }

    void showFirstTimeSetup(float dt) {
        log::debug("showFirstTimeSetup called after 5 second delay");
        
        auto popup = FirstTimeSetupPopup::create();
        if (!popup) {
            log::error("Failed to create FirstTimeSetupPopup");
            return;
        }
        
        log::debug("FirstTimeSetupPopup created successfully, showing to user");
        popup->show();
    }
};

// Hook to load persistent data when the mod is loaded
$execute {
    log::info("CollabEditor mod loading - initializing CollabManager and loading persistent data");
    
    // Load saved data when the mod initializes
    auto collabMgr = CollabManager::get();
    if (!collabMgr) {
        log::error("Failed to get CollabManager instance in $execute block");
    } else {
        log::debug("Loading persistent data from last session");
        collabMgr->loadData();
        log::info("CollabManager initialized and data loaded successfully");
    }
}
