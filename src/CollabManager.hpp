#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>

// Simplified Geometry Dash types (avoiding Geode headers for now)
struct GameObject {
    int id;
    float x, y;
    std::string type;
    std::string data;
    
    GameObject(int objId, float posX, float posY, const std::string& objType, const std::string& objData)
        : id(objId), x(posX), y(posY), type(objType), data(objData) {}
};

struct PlayerInfo {
    std::string playerId;
    std::string playerName;
    int colorR, colorG, colorB;  // RGB values for player color
    float cursorX, cursorY;
    bool isConnected;
    bool isHost;
    
    PlayerInfo(const std::string& id, const std::string& name, int r, int g, int b, bool host = false)
        : playerId(id), playerName(name), colorR(r), colorG(g), colorB(b), 
          cursorX(0), cursorY(0), isConnected(false), isHost(host) {}
};

struct NetworkPacket {
    std::string type;
    std::string data;
    std::string senderId;
    double timestamp;
    
    NetworkPacket(const std::string& t, const std::string& d, const std::string& sender = "")
        : type(t), data(d), senderId(sender), timestamp(0) {}
};

class CollabManager {
public:
    static CollabManager* get();
    
    // Core session management
    bool hostSession(int port = 8080);
    bool joinSession(const std::string& serverUrl, const std::string& playerName);
    void leaveSession();
    bool isConnected() const;
    
    // Player management
    void addPlayer(const PlayerInfo& player);
    void removePlayer(const std::string& playerId);
    const std::vector<PlayerInfo>& getPlayers() const;
    const PlayerInfo* getCurrentPlayer() const;
    const PlayerInfo* getPlayer(const std::string& playerId) const;
    
    // Object synchronization
    void syncObjectAdd(const GameObject& obj);
    void syncObjectRemove(int objectId);
    void syncObjectMove(int objectId, float x, float y);
    void claimObject(int objectId, const std::string& playerId);
    
    // Network communication
    void sendPacket(const NetworkPacket& packet);
    void processNetworkEvents();
    
    // Chat functionality
    void sendChatMessage(const std::string& message);
    const std::vector<std::string>& getChatMessages() const;
    
    // Cursor tracking
    void updateLocalCursor(float x, float y);
    void updatePlayerCursor(const std::string& playerId, float x, float y);
    
    // Settings
    void setUpdateFrequency(float frequency);
    float getUpdateFrequency() const;
    void setLowLagMode(bool enabled);
    bool isLowLagMode() const;
    
    // Thread safety
    void lockNetworkQueue();
    void unlockNetworkQueue();
    
private:
    CollabManager();
    ~CollabManager();
    
    static CollabManager* s_instance;
    
    // Network connection (simplified Windows socket for now)
    SOCKET m_socket;
    std::thread m_networkThread;
    std::queue<NetworkPacket> m_packetQueue;
    std::mutex m_queueMutex;
    std::mutex m_connectionMutex;
    bool m_isConnected;
    bool m_isHost;
    
    // Player management
    std::vector<PlayerInfo> m_players;
    std::unordered_map<int, std::string> m_objectOwnership;
    PlayerInfo* m_currentPlayer;
    
    // Network state
    std::string m_serverUrl;
    int m_serverPort;
    
    // Chat system
    std::vector<std::string> m_chatMessages;
    
    // Settings
    float m_updateFrequency;
    bool m_lowLagMode;
    double m_lastCursorUpdate;
    
    // Internal methods
    void networkThreadLoop();
    void handlePacket(const NetworkPacket& packet);
    void broadcastToAll(const NetworkPacket& packet);
    void assignPlayerColor(PlayerInfo& player);
    std::string generatePlayerID();
    bool connectToServer(const std::string& address, int port);
    void disconnectFromServer();
};
