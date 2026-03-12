#include "CollabManager.hpp"
#include "Hooks.hpp"
#include <nlohmann/json.hpp>

using namespace geode::prelude;
using json = nlohmann::json;

class PacketHandler {
public:
    static void handlePacket(const json& packet) {
        std::string type = packet.value("type", "");
        
        if (type == "PLAYER_JOIN") {
            handlePlayerJoin(packet);
        } else if (type == "PLAYER_LEAVE") {
            handlePlayerLeave(packet);
        } else if (type == "CURSOR_UPDATE") {
            handleCursorUpdate(packet);
        } else if (type == "CHAT_MESSAGE") {
            handleChatMessage(packet);
        } else if (type == "ADD_OBJECT") {
            Hooks::handleAddObject(packet);
        } else if (type == "REMOVE_OBJECT") {
            Hooks::handleRemoveObject(packet);
        } else if (type == "MOVE_OBJECT") {
            Hooks::handleMoveObject(packet);
        } else if (type == "CLAIM_OBJECT") {
            Hooks::handleClaimObject(packet);
        } else if (type == "TRANSFER_OWNERSHIP") {
            Hooks::handleTransferOwnership(packet);
        } else if (type == "REQUEST_SYNC") {
            handleSyncRequest(packet);
        } else if (type == "FULL_SYNC") {
            handleFullSync(packet);
        }
    }
    
private:
    static void handlePlayerJoin(const json& packet) {
        auto& manager = CollabManager::get();
        PlayerInfo player;
        player.id = packet["playerID"];
        player.name = packet["playerName"];
        auto colorData = packet["color"];
        player.color = ccColor3B{colorData[0], colorData[1], colorData[2]};
        player.cursorPos = CCPointZero;
        player.isConnected = true;
        manager.addPlayer(player);
    }
    
    static void handlePlayerLeave(const json& packet) {
        auto& manager = CollabManager::get();
        manager.removePlayer(packet["playerID"]);
    }
    
    static void handleCursorUpdate(const json& packet) {
        auto& manager = CollabManager::get();
        std::string playerID = packet["playerID"];
        CCPoint pos{packet["x"], packet["y"]};
        
        const auto& players = manager.getPlayers();
        auto it = players.find(playerID);
        if (it != players.end()) {
            // Update cursor position through manager
            // This would be handled by CursorIndicator component
        }
    }
    
    static void handleChatMessage(const json& packet) {
        std::string playerName = packet["playerName"];
        std::string message = packet["message"];
        ccColor3B color = {255, 255, 255}; // Default color
        
        auto& manager = CollabManager::get();
        auto playerColor = manager.getPlayerColor(packet["playerID"]);
        if (playerColor) {
            color = *playerColor;
        }
        
        // Add to chat window
        log::info("Chat: {}: {}", playerName, message);
    }
    
    static void handleSyncRequest(const json& packet) {
        // Send current level state to requesting player
        // This would integrate with ResyncManager
    }
    
    static void handleFullSync(const json& packet) {
        // Apply received level state
        // This would integrate with ResyncManager
    }
};
