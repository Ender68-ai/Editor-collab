#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include "SessionManager.hpp"
#include "P2PManager.hpp"
#include "RemoteActionHandler.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("Editor collab v{} loaded!", Mod::get()->getVersion().toNonVString());
}

$on_mod(DataSaved) {
    // Clean up on mod data save
    auto& session = mpedit::SessionManager::get();
    if (session.isInSession()) {
        session.leaveSession();
    }
}
