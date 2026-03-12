#include <Geode/Geode.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("Collaborative Editor Mod loaded");
}

$on_mod(Unloaded) {
    log::info("Collaborative Editor Mod unloaded");
}
