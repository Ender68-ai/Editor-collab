#pragma once
#include <Geode/Geode.hpp>
#include <nlohmann/json.hpp>

using namespace geode::prelude;
using json = nlohmann::json;

class Hooks {
public:
    // Network packet handlers
    static void handleAddObject(const json& data);
    static void handleRemoveObject(const json& data);
    static void handleMoveObject(const json& data);
    static void handleClaimObject(const json& data);
    static void handleTransferOwnership(const json& data);
    
    // Anti-loop prevention
    static bool shouldIgnoreAction();
    static void setIgnoreNext(bool ignore);
    
private:
    static bool s_ignoreNext;
};
