#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>
#include <windows.h>

// Now include Geode and other headers
#include "CollabNetworkManager.hpp"
#include <Geode/Geode.hpp>
#include <sstream>
#include <chrono>

#ifdef WIN32
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

using namespace geode::prelude;

CollabNetworkManager* CollabNetworkManager::s_instance = nullptr;
std::mutex CollabNetworkManager::s_instanceMutex;

CollabNetworkManager::CollabNetworkManager()
    : m_tcpServer("its-velcro.gl.at.ply.gg:11262"),
      m_udpServer("simply-cdt.gl.at.ply.gg:11714"),
      m_accountID(0),
      m_levelID(0),
      m_lastHeartbeat(0) {
    log::info("[CollabNetworkManager] Initialized");
}

CollabNetworkManager::~CollabNetworkManager() {
    stopConnection();
    log::info("[CollabNetworkManager] Destroyed");
}

CollabNetworkManager* CollabNetworkManager::get() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = new CollabNetworkManager();
    }
    return s_instance;
}

void CollabNetworkManager::destroy() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

// ============================================================================
// Connection Management
// ============================================================================

bool CollabNetworkManager::startConnection(uint32_t accountID, const std::string& token, uint32_t levelID,
                                           const std::string& tcpServer, const std::string& udpServer) {
    if (m_isConnecting.exchange(true)) {
        log::warn("[CollabNetworkManager] Connection already in progress");
        return false;
    }
    
    if (m_tcpConnected || m_udpConnected) {
        log::warn("[CollabNetworkManager] Already connected, stopping first");
        stopConnection();
    }
    
    m_accountID = accountID;
    m_token = token;
    m_levelID = levelID;
    m_tcpServer = tcpServer;
    m_udpServer = udpServer;
    m_shouldStop = false;
    
    log::info("[CollabNetworkManager] Starting connection: Account={}, Level={}, TCP={}, UDP={}",
              accountID, levelID, tcpServer, udpServer);
    
    // Start network threads
    try {
        m_tcpThread = std::make_unique<std::thread>(&CollabNetworkManager::tcpThreadLoop, this);
        m_udpThread = std::make_unique<std::thread>(&CollabNetworkManager::udpThreadLoop, this);
        return true;
    } catch (const std::exception& e) {
        log::error("[CollabNetworkManager] Failed to start threads: {}", e.what());
        m_isConnecting = false;
        return false;
    }
}

void CollabNetworkManager::stopConnection() {
    log::info("[CollabNetworkManager] Stopping connection");
    m_shouldStop = true;
    
    if (m_tcpThread && m_tcpThread->joinable()) {
        m_tcpThread->join();
        m_tcpThread.reset();
    }
    
    if (m_udpThread && m_udpThread->joinable()) {
        m_udpThread->join();
        m_udpThread.reset();
    }
    
    m_tcpConnected = false;
    m_udpConnected = false;
    m_isConnecting = false;
    m_remoteCursors.clear();
    
    log::info("[CollabNetworkManager] Connection stopped");
}

// ============================================================================
// TCP Thread Loop
// ============================================================================

void CollabNetworkManager::tcpThreadLoop() {
    log::info("[TCP] Thread started");
    
    while (!m_shouldStop) {
        if (!m_tcpConnected) {
            if (connectTCP()) {
                log::info("[TCP] Connected successfully - HIT SERVER!");
                m_tcpConnected = true;
                m_isConnecting = false;
                sendTCPAuth();
            } else {
                log::warn("[TCP] Connection failed, retrying in 5 seconds...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        
        if (m_tcpConnected) {
            processTCPMessages();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    disconnectTCP();
    log::info("[TCP] Thread ended");
}

bool CollabNetworkManager::connectTCP() {
    // Parse server address
    size_t colonPos = m_tcpServer.find(':');
    if (colonPos == std::string::npos) {
        log::error("[TCP] Invalid server format: {}", m_tcpServer);
        return false;
    }
    
    std::string host = m_tcpServer.substr(0, colonPos);
    std::string portStr = m_tcpServer.substr(colonPos + 1);
    int port = std::stoi(portStr);
    
    log::info("[TCP] Connecting to {}:{}", host, port);
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        log::error("[TCP] Failed to create socket");
        return false;
    }
    
    // Resolve hostname
    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
        log::error("[TCP] Failed to resolve hostname: {}", host);
        closesocket(sock);
        return false;
    }
    
    // Connect
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        log::error("[TCP] Failed to connect to {}:{}", host, port);
        closesocket(sock);
        return false;
    }
    
    // TODO: Upgrade to WebSocket
    // For now, store the socket for basic communication
    // In production, use a WebSocket library like boost::beast
    
    return true;
}

void CollabNetworkManager::disconnectTCP() {
    m_tcpConnected = false;
    log::info("[TCP] Disconnected");
}

void CollabNetworkManager::sendTCPAuth() {
    log::info("[TCP] Sending auth packet: account_id={}, token={}, level_id={}",
              m_accountID, m_token, m_levelID);
    
    // Format: {"account_id": <ID>, "token": "<TOKEN>", "level_id": <LEVEL_ID>}
    std::stringstream ss;
    ss << "{\"account_id\": " << m_accountID << ", \"token\": \"" << m_token
       << "\", \"level_id\": " << m_levelID << "}";
    
    std::string authPacket = ss.str();
    log::debug("[TCP] Auth packet: {}", authPacket);
    
    // TODO: Send over WebSocket
    // This would be implemented with a proper WebSocket library
}

void CollabNetworkManager::processTCPMessages() {
    // TODO: Implement WebSocket message processing
    // Listen for Chat/Object sync messages from server
}

// ============================================================================
// UDP Thread Loop
// ============================================================================

void CollabNetworkManager::udpThreadLoop() {
    log::info("[UDP] Thread started");
    
    while (!m_shouldStop) {
        if (!m_udpConnected) {
            if (connectUDP()) {
                log::info("[UDP] Connected successfully - HIT SERVER!");
                m_udpConnected = true;
            } else {
                log::warn("[UDP] Connection failed, retrying in 5 seconds...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        
        if (m_udpConnected) {
            processUDPPackets();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    disconnectUDP();
    log::info("[UDP] Thread ended");
}

bool CollabNetworkManager::connectUDP() {
    // Parse server address
    size_t colonPos = m_udpServer.find(':');
    if (colonPos == std::string::npos) {
        log::error("[UDP] Invalid server format: {}", m_udpServer);
        return false;
    }
    
    std::string host = m_udpServer.substr(0, colonPos);
    std::string portStr = m_udpServer.substr(colonPos + 1);
    int port = std::stoi(portStr);
    
    log::info("[UDP] Connecting to {}:{}", host, port);
    
    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        log::error("[UDP] Failed to create socket");
        return false;
    }
    
    // TODO: Store socket for later use
    // For now, just mark as connected
    
    return true;
}

void CollabNetworkManager::disconnectUDP() {
    m_udpConnected = false;
    log::info("[UDP] Disconnected");
}

void CollabNetworkManager::sendUDPHeartbeat(float x, float y, float rotation) {
    if (!m_udpConnected) return;
    
    // Format: [AccountID]:[X]:[Y]:[Rotation]
    std::stringstream ss;
    ss << m_accountID << ":" << x << ":" << y << ":" << rotation;
    
    std::string packet = ss.str();
    log::debug("[UDP] Sending heartbeat: {}", packet);
    
    // TODO: Send UDP packet
    // This would require actual UDP socket implementation
}

void CollabNetworkManager::processUDPPackets() {
    // TODO: Listen for incoming UDP packets and parse cursor data
    // Expected format: CURSOR:[AccountID]:[X]:[Y]:[Rotation]
}

// ============================================================================
// Validation
// ============================================================================

bool CollabNetworkManager::validateWithArgon() {
    log::info("[Auth] Validating credentials with Argon...");
    
    // TODO: Call argon::startAuth() and wait for token
    // This would be integrated with argon's authentication system
    
    return true;
}

// ============================================================================
// Cursor Tracking
// ============================================================================

void CollabNetworkManager::updateLocalCursor(float x, float y, float rotation) {
    std::lock_guard<std::mutex> lock(m_localCursorMutex);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Only send heartbeat if cursor moved or 100ms has passed
    if ((m_lastSentX != x || m_lastSentY != y || m_lastSentRotation != rotation) ||
        (now - m_lastHeartbeat > 100)) {
        
        m_lastSentX = x;
        m_lastSentY = y;
        m_lastSentRotation = rotation;
        m_lastHeartbeat = now;
        
        // Queue for sending in UDP thread
        sendUDPHeartbeat(x, y, rotation);
    }
}

std::map<uint32_t, RemoteCursor> CollabNetworkManager::getRemoteCursors() {
    std::lock_guard<std::mutex> lock(m_cursorMutex);
    return m_remoteCursors;
}

void CollabNetworkManager::clearRemoteCursors() {
    std::lock_guard<std::mutex> lock(m_cursorMutex);
    m_remoteCursors.clear();
}

// ============================================================================
// Network Updates Processing (Main Thread)
// ============================================================================

void CollabNetworkManager::processNetworkUpdates() {
    // This is called from the main thread to process queued cursor updates
    std::lock_guard<std::mutex> lock(m_cursorMutex);
    
    while (!m_cursorUpdates.empty()) {
        RemoteCursor cursor = m_cursorUpdates.front();
        m_cursorUpdates.pop();
        
        if (cursor.account_id != m_accountID) {
            m_remoteCursors[cursor.account_id] = cursor;
        }
    }
}
