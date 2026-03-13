#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <queue>

namespace CollabEditor {
    // Player information structure
    struct PlayerInfo {
        std::string playerId;
        std::string playerName;
        float cursorX;
        float cursorY;
        int colorR;
        int colorG;
        int colorB;
        bool isHost;
        
        PlayerInfo() : cursorX(0), cursorY(0), colorR(255), colorG(255), colorB(255), isHost(false) {}
    };
    
    // Game object structure
    struct GameObject {
        int id;
        float x;
        float y;
        int type;
        std::string data;
        std::string ownerId;
        
        GameObject() : id(0), x(0), y(0), type(0) {}
    };
    
    // Network packet structure
    struct NetworkPacket {
        std::string type;
        std::string data;
        
        NetworkPacket(const std::string& t = "", const std::string& d = "") 
            : type(t), data(d) {}
    };
}

// Main manager class interface
class CollabManager {
public:
    static CollabManager* get();
    
    // Session management
    bool hostSession(int port);
    bool joinSession(const std::string& serverUrl, const std::string& playerName);
    void leaveSession();
    bool isConnected() const;
    
    // Player management
    void addPlayer(const CollabEditor::PlayerInfo& player);
    void removePlayer(const std::string& playerId);
    const std::vector<CollabEditor::PlayerInfo>& getPlayers() const;
    const CollabEditor::PlayerInfo* getCurrentPlayer() const;
    
    // Object synchronization
    void syncObjectAdd(const CollabEditor::GameObject& obj);
    void syncObjectRemove(int objectId);
    void syncObjectMove(int objectId, float x, float y);
    void claimObject(int objectId, const std::string& playerId);
    
    // Network events
    void sendPacket(const CollabEditor::NetworkPacket& packet);
    void processNetworkEvents();
    
    // Chat system
    void sendChatMessage(const std::string& message);
    const std::vector<std::string>& getChatMessages() const;
    
    // Cursor tracking
    void updateLocalCursor(float x, float y);
    void updatePlayerCursor(const std::string& playerId, float x, float y);
    
    // Performance settings
    void setUpdateFrequency(float frequency);
    float getUpdateFrequency() const;
    void setLowLagMode(bool enabled);
    bool isLowLagMode() const;
    
    // Thread safety
    void lockNetworkQueue();
    void unlockNetworkQueue();
};
