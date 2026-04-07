#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <map>

using namespace geode::prelude;

// ============================================================================
// Remote Cursor Data Structure
// ============================================================================

struct RemoteCursor {
    uint32_t account_id;
    float x;
    float y;
    float rotation;
    int64_t last_update; // timestamp in ms
    
    RemoteCursor()
        : account_id(0), x(0), y(0), rotation(0), last_update(0) {}
    
    RemoteCursor(uint32_t id, float pos_x, float pos_y, float rot)
        : account_id(id), x(pos_x), y(pos_y), rotation(rot) {
        last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

// ============================================================================
// TCP/WebSocket + UDP Network Manager
// ============================================================================

class CollabNetworkManager {
protected:
    static CollabNetworkManager* s_instance;
    static std::mutex s_instanceMutex;
    
    // Connection state
    std::atomic<bool> m_tcpConnected{false};
    std::atomic<bool> m_udpConnected{false};
    std::atomic<bool> m_isConnecting{false};
    
    // Server info
    std::string m_tcpServer;      // e.g., "your-playit.gg-domain.com:8080"
    std::string m_udpServer;      // e.g., "your-playit.gg-domain.com:41234"
    uint32_t m_accountID{0};
    std::string m_token;
    uint32_t m_levelID{0};
    
    // Network threads
    std::unique_ptr<std::thread> m_tcpThread;
    std::unique_ptr<std::thread> m_udpThread;
    std::atomic<bool> m_shouldStop{false};
    
    // Remote cursors tracking
    std::map<uint32_t, RemoteCursor> m_remoteCursors;
    std::mutex m_cursorMutex;
    std::queue<RemoteCursor> m_cursorUpdates;
    
    // Local cursor tracking for heartbeat
    float m_lastSentX{0};
    float m_lastSentY{0};
    float m_lastSentRotation{0};
    int64_t m_lastHeartbeat{0};
    std::mutex m_localCursorMutex;
    
    CollabNetworkManager();
    ~CollabNetworkManager();
    
    // Network thread functions
    void tcpThreadLoop();
    void udpThreadLoop();
    
    // TCP WebSocket handlers
    bool connectTCP();
    void disconnectTCP();
    void sendTCPAuth();
    void processTCPMessages();
    
    // UDP handlers
    bool connectUDP();
    void disconnectUDP();
    void sendUDPHeartbeat(float x, float y, float rotation);
    void processUDPPackets();
    
    // Validation (will call argon)
    bool validateWithArgon();
    
public:
    static CollabNetworkManager* get();
    static void destroy();
    
    // Connection management
    bool startConnection(uint32_t accountID, const std::string& token, uint32_t levelID,
                       const std::string& tcpServer = "localhost:8080",
                       const std::string& udpServer = "localhost:41234");
    void stopConnection();
    
    bool isTCPConnected() const { return m_tcpConnected; }
    bool isUDPConnected() const { return m_udpConnected; }
    bool isConnecting() const { return m_isConnecting; }
    
    // Cursor tracking
    void updateLocalCursor(float x, float y, float rotation);
    std::map<uint32_t, RemoteCursor> getRemoteCursors();
    void clearRemoteCursors();
    
    // Message processing (called by main thread)
    void processNetworkUpdates();
    
    // Getters
    uint32_t getAccountID() const { return m_accountID; }
    uint32_t getLevelID() const { return m_levelID; }
};
