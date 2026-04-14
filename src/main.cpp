#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>

// Core functionality
#include "core/CollabNetworkManager.hpp"
#include "core/CollabManager.hpp"

// UI functionality
#include "ui/CollabRoomPopup.hpp"
#include "ui/CollabPopups.hpp"

using namespace geode::prelude;

/**
 * Main entry point for the CollabEditor mod
 * Initializes all core and UI components
 */
class $modify(CollabMod, MenuLayer) {
    void onMenuLoad() {
        // Initialize core network manager
        CollabNetworkManager::get()->startConnection(0, "default_token", 0);
        
        log::info("CollabEditor mod loaded successfully");
        log::info("Core components: Network Manager initialized");
        log::info("UI components: All popup systems ready");
    }
};
