#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <cstring>

using namespace geode::prelude;

// ============================================================================
// ENUMS & BASIC STRUCTS
// ============================================================================

enum class CollabRole {
    Editor = 0,
    Suggestor = 1,
    Viewer = 2
};

enum class DeviceType {
    PC = 0,
    Mobile = 1
};

struct CollabPlayer {
    std::string name;
    int accountID;
    CollabRole role;
    bool enabled;
    
    CollabPlayer(std::string n, int id, CollabRole r, bool e = true)
        : name(n), accountID(id), role(r), enabled(e) {}
};

struct CollabLevel {
    int levelID;
    std::string levelName;
    int hostAccountID;
    std::vector<CollabPlayer> players;
    bool collabEnabled;
    
    CollabLevel() : levelID(0), levelName(""), hostAccountID(0), collabEnabled(false) {}
    CollabLevel(int id, std::string name, int hostID)
        : levelID(id), levelName(name), hostAccountID(hostID), collabEnabled(false) {}
};

struct TransferRequest {
    int fromAccountID;
    int toAccountID;
    int levelID;
    std::string levelName;
    std::string timestamp;
    
    TransferRequest() : fromAccountID(0), toAccountID(0), levelID(0), levelName(""), timestamp("") {}
    
    TransferRequest(int from, int to, int level, std::string name)
        : fromAccountID(from), toAccountID(to), levelID(level), levelName(name) {
        timestamp = std::to_string(time(nullptr));
    }
};

// ============================================================================
// NETWORKING PACKET STRUCTURES
// ============================================================================

#pragma pack(push, 1)

enum class PacketType : uint8_t {
    PlayerPosition = 1,
    ObjectLock = 2,
    ObjectUnlock = 3,
    ObjectMove = 4,
    PlayerDisconnect = 5,
    Handshake = 6
};

struct PacketHeader {
    PacketType type;
    uint16_t length;
    int senderAccountID;
    uint32_t timestamp;
};

struct PlayerPositionPacket {
    PacketHeader header;
    float posX;
    float posY;
    uint8_t deviceType;
};

struct ObjectLockPacket {
    PacketHeader header;
    int objectID;
    int accountID;
};

struct ObjectUnlockPacket {
    PacketHeader header;
    int objectID;
    int accountID;
};

struct ObjectMovePacket {
    PacketHeader header;
    int objectID;
    float newX;
    float newY;
};

struct PlayerDisconnectPacket {
    PacketHeader header;
    int accountID;
};

struct HandshakePacket {
    PacketHeader header;
    int accountID;
    uint8_t deviceType;
    char playerName[64];
};

#pragma pack(pop)

// ============================================================================
// REMOTE PLAYER TRACKING
// ============================================================================

struct RemotePlayer {
    int accountID;
    std::string playerName;
    ccColor3B color;
    DeviceType device;
    CCPoint position;
    float lastUpdateTime;
    
    // Visual elements
    CCSprite* cursorSprite = nullptr;
    CCLabelBMFont* nameLabel = nullptr;
    float nameOpacity = 0.0f;
    
    RemotePlayer() 
        : accountID(-1), playerName(""), position(0.0f, 0.0f), 
          device(DeviceType::PC), lastUpdateTime(0.0f), 
          nameOpacity(0.0f) {}
    
    RemotePlayer(int id, const std::string& name, DeviceType dev, ccColor3B col)
        : accountID(id), playerName(name), color(col), device(dev), 
          position(0.0f, 0.0f), lastUpdateTime(0.0f), nameOpacity(0.0f) {}
};

// ============================================================================
// OBJECT LOCK TRACKING
// ============================================================================

struct ObjectLockState {
    int objectID;
    int lockedByAccountID;
    float lockTimeSeconds;
    
    ObjectLockState() : objectID(-1), lockedByAccountID(-1), lockTimeSeconds(0.0f) {}
    ObjectLockState(int id, int accountID) 
        : objectID(id), lockedByAccountID(accountID), lockTimeSeconds(0.0f) {}
};

// ============================================================================
// NETWORK MANAGER (P2P SWARM NETWORKING)
// ============================================================================

class NetworkManager {
protected:
    static NetworkManager* s_instance;
    static std::mutex s_instanceMutex;
    
    int m_localAccountID;
    DeviceType m_localDeviceType;
    std::string m_localPlayerName;
    
    // Network state
    bool m_isConnected = false;
    std::queue<std::vector<uint8_t>> m_incomingPackets;
    mutable std::mutex m_packetQueueMutex;
    
    NetworkManager();
    ~NetworkManager();
    
public:
    static NetworkManager* get();
    static void destroy();
    
    // Initialization
    void initialize(int accountID, const std::string& playerName, DeviceType device);
    
    // Packet sending
    void sendPlayerPosition(float x, float y);
    void sendObjectLock(int objectID);
    void sendObjectUnlock(int objectID);
    void sendObjectMove(int objectID, float newX, float newY);
    void sendHandshake();
    
    // Packet receiving
    bool hasIncomingPackets() const;
    std::vector<uint8_t> popIncomingPacket();
    void onPacketReceived(const std::vector<uint8_t>& data);
    
    // Status
    bool isConnected() const { return m_isConnected; }
    void setConnected(bool connected) { m_isConnected = connected; }
};

// ============================================================================
// COLLAB MANAGER (MAIN MANAGER CLASS)
// ============================================================================

class CollabManager {
protected:
    static CollabManager* s_instance;
    static std::mutex s_instanceMutex;
    
    // Collab data
    std::unordered_map<int, CollabLevel> m_collabLevels;
    std::vector<TransferRequest> m_transferRequests;
    std::vector<int> m_hostedLevels;
    std::vector<int> m_collabedLevels;
    bool m_firstTimeSetup = true;
    
    // Real-time collaboration
    std::unordered_map<int, RemotePlayer> m_remotePlayers;  // Key: accountID
    std::unordered_map<int, ObjectLockState> m_lockedObjects; // Key: objectID
    int m_currentLevelID = -1;
    
    // Local player state
    CCPoint m_localPlayerPosition;
    DeviceType m_localDeviceType = DeviceType::PC;
    int m_localAccountID = -1;
    
    CollabManager();
    ~CollabManager();
    
public:
    static CollabManager* get();
    static void destroy();
    
    // ========== LEVEL MANAGEMENT ==========
    void enableCollabForLevel(int levelID, const std::string& levelName, int hostAccountID);
    void disableCollabForLevel(int levelID);
    bool isCollabEnabled(int levelID) const;
    bool isHostOfLevel(int levelID, int accountID) const;
    const std::unordered_map<int, CollabLevel>& getCollabLevels() const { return m_collabLevels; }
    
    // ========== PLAYER MANAGEMENT ==========
    void addPlayerToLevel(int levelID, const CollabPlayer& player);
    void removePlayerFromLevel(int levelID, int accountID);
    void updatePlayerRole(int levelID, int accountID, CollabRole role);
    void togglePlayerStatus(int levelID, int accountID);
    std::vector<CollabPlayer> getPlayersInLevel(int levelID) const;
    
    // ========== TRANSFER SYSTEM ==========
    void requestTransfer(int levelID, int fromAccountID, int toAccountID, const std::string& levelName);
    void acceptTransfer(const TransferRequest& request);
    void declineTransfer(const TransferRequest& request);
    std::vector<TransferRequest> getTransferRequestsForAccount(int accountID) const;
    
    // ========== LEVEL LISTS ==========
    void addHostedLevel(int levelID);
    void addCollabedLevel(int levelID);
    std::vector<int> getHostedLevels() const;
    std::vector<int> getCollabedLevels() const;
    
    // ========== SETUP STATE ==========
    bool isFirstTimeSetup() const { return m_firstTimeSetup; }
    void setFirstTimeSetupComplete() { m_firstTimeSetup = false; }
    
    // ========== REAL-TIME COLLABORATION ==========
    
    // Remote player management
    void addRemotePlayer(int accountID, const std::string& playerName, DeviceType device);
    void removeRemotePlayer(int accountID);
    RemotePlayer* getRemotePlayer(int accountID);
    const std::unordered_map<int, RemotePlayer>& getRemotePlayers() const { return m_remotePlayers; }
    void updateRemotePlayerPosition(int accountID, float x, float y);
    
    // Object locking
    bool isObjectLocked(int objectID) const;
    bool canLockObject(int objectID, int accountID);
    void lockObject(int objectID, int accountID);
    void unlockObject(int objectID);
    int getObjectLockOwner(int objectID) const;
    
    // Local player state
    void setLocalPlayerPosition(CCPoint pos) { m_localPlayerPosition = pos; }
    void setLocalAccountID(int accountID) { m_localAccountID = accountID; }
    void setLocalDeviceType(DeviceType device) { m_localDeviceType = device; }
    void setCurrentLevelID(int levelID) { m_currentLevelID = levelID; }
    
    int getLocalAccountID() const { return m_localAccountID; }
    CCPoint getLocalPlayerPosition() const { return m_localPlayerPosition; }
    DeviceType getLocalDeviceType() const { return m_localDeviceType; }
    int getCurrentLevelID() const { return m_currentLevelID; }
    
    // Network packet processing
    void processIncomingPackets();
    
    // ========== DATA PERSISTENCE ==========
    void saveData();
    void loadData();
};
