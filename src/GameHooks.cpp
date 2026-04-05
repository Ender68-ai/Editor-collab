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
        
        // REMOVED: First time setup check - moved to EditLevelLayer
        log::debug("First time setup check moved to EditLevelLayer");

        log::debug("MenuLayer::init hook completed successfully");
        return true;
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
