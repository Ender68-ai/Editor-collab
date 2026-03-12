#include "CollabManager.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

CollabManager* CollabManager::s_instance = nullptr;

CollabManager* CollabManager::get() {
    if (!s_instance) {
        s_instance = new CollabManager();
    }
    return s_instance;
}

CollabManager::CollabManager() 
    : m_socket(INVALID_SOCKET), m_isConnected(false), m_isHost(false), 
      m_serverPort(8080), m_updateFrequency(30.0f), m_lowLagMode(false),
      m_lastCursorUpdate(0), m_currentPlayer(nullptr) {
    
    OutputDebugStringA("CollabManager initialized");
}

CollabManager::~CollabManager() {
    leaveSession();
    OutputDebugStringA("CollabManager destroyed");
}

bool CollabManager::hostSession(int port) {
    m_serverPort = port;
    m_isHost = true;
    
    // Create current player (host)
    m_currentPlayer = new PlayerInfo(generatePlayerID(), "Host", 255, 100, 100, true);
    addPlayer(*m_currentPlayer);
    
    OutputDebugStringA(("Hosting session on port " + std::to_string(port)).c_str());
    return true;
}

bool CollabManager::joinSession(const std::string& serverUrl, const std::string& playerName) {
    m_serverUrl = serverUrl;
    m_isHost = false;
    
    OutputDebugStringA(("Joining session at " + serverUrl).c_str());
    
    // Create current player
    m_currentPlayer = new PlayerInfo(generatePlayerID(), playerName, 0, 0, 0, false);
    addPlayer(*m_currentPlayer);
    
    m_isConnected = true;
    return true;
}

void CollabManager::leaveSession() {
    m_isConnected = false;
    
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
    
    m_players.clear();
    m_objectOwnership.clear();
    m_chatMessages.clear();
    
    OutputDebugStringA("Left session");
}

bool CollabManager::isConnected() const {
    return m_isConnected;
}

void CollabManager::addPlayer(const PlayerInfo& player) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_players.push_back(player);
    
    OutputDebugStringA(("Added player: " + player.playerName).c_str());
}

void CollabManager::removePlayer(const std::string& playerId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_players.erase(
        std::remove_if(m_players.begin(), m_players.end(),
            [&playerId](const PlayerInfo& p) { return p.playerId == playerId; })
    );
    
    OutputDebugStringA(("Removed player: " + playerId).c_str());
}

const std::vector<PlayerInfo>& CollabManager::getPlayers() const {
    return m_players;
}

const PlayerInfo* CollabManager::getCurrentPlayer() const {
    return m_currentPlayer;
}

const PlayerInfo* CollabManager::getPlayer(const std::string& playerId) const {
    for (const auto& player : m_players) {
        if (player.playerId == playerId) {
            return &player;
        }
    }
    return nullptr;
}

void CollabManager::syncObjectAdd(const GameObject& obj) {
    OutputDebugStringA(("Sync object add: " + std::to_string(obj.id)).c_str());
}

void CollabManager::syncObjectRemove(int objectId) {
    OutputDebugStringA(("Sync object remove: " + std::to_string(objectId)).c_str());
}

void CollabManager::syncObjectMove(int objectId, float x, float y) {
    OutputDebugStringA(("Sync object move: " + std::to_string(objectId)).c_str());
}

void CollabManager::claimObject(int objectId, const std::string& playerId) {
    m_objectOwnership[objectId] = playerId;
    OutputDebugStringA(("Object claimed: " + std::to_string(objectId) + " by " + playerId).c_str());
}

void CollabManager::sendPacket(const NetworkPacket& packet) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_packetQueue.push(packet);
}

void CollabManager::processNetworkEvents() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_packetQueue.empty()) {
        NetworkPacket packet = m_packetQueue.front();
        m_packetQueue.pop();
        handlePacket(packet);
    }
}

void CollabManager::sendChatMessage(const std::string& message) {
    if (m_currentPlayer) {
        OutputDebugStringA(("Chat: " + m_currentPlayer->playerName + ": " + message).c_str());
        
        // Add to local chat
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_chatMessages.push_back(m_currentPlayer->playerName + ": " + message);
        if (m_chatMessages.size() > 100) {
            m_chatMessages.erase(m_chatMessages.begin());
        }
    }
}

const std::vector<std::string>& CollabManager::getChatMessages() const {
    return m_chatMessages;
}

void CollabManager::updateLocalCursor(float x, float y) {
    if (m_currentPlayer) {
        m_currentPlayer->cursorX = x;
        m_currentPlayer->cursorY = y;
        
        // Throttle cursor updates
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (now - m_lastCursorUpdate > (1000.0 / m_updateFrequency)) {
            OutputDebugStringA(("Cursor update: " + std::to_string(x) + "," + std::to_string(y)).c_str());
            m_lastCursorUpdate = now;
        }
    }
}

void CollabManager::updatePlayerCursor(const std::string& playerId, float x, float y) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (auto& player : m_players) {
        if (player.playerId == playerId) {
            player.cursorX = x;
            player.cursorY = y;
            break;
        }
    }
}

void CollabManager::setUpdateFrequency(float frequency) {
    m_updateFrequency = frequency;
}

float CollabManager::getUpdateFrequency() const {
    return m_updateFrequency;
}

void CollabManager::setLowLagMode(bool enabled) {
    m_lowLagMode = enabled;
}

bool CollabManager::isLowLagMode() const {
    return m_lowLagMode;
}

void CollabManager::lockNetworkQueue() {
    m_queueMutex.lock();
}

void CollabManager::unlockNetworkQueue() {
    m_queueMutex.unlock();
}

void CollabManager::networkThreadLoop() {
    // Simplified network loop for demonstration
    while (m_isConnected) {
        // Simulate network processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void CollabManager::handlePacket(const NetworkPacket& packet) {
    OutputDebugStringA(("Handling packet: " + packet.type + " - " + packet.data).c_str());
}

void CollabManager::broadcastToAll(const NetworkPacket& packet) {
    OutputDebugStringA(("Broadcasting: " + packet.type + " - " + packet.data).c_str());
}

void CollabManager::assignPlayerColor(PlayerInfo& player) {
    // Assign distinct colors to players
    static int colorIndex = 0;
    int colors[][3] = {
        {255, 100, 100},  // Red
        {100, 255, 100},  // Green
        {100, 100, 255},  // Blue
        {255, 255, 100},  // Yellow
        {255, 100, 255},  // Magenta
        {100, 255, 255},  // Cyan
        {255, 165, 0},    // Orange
        {128, 0, 128}     // Purple
    };
    
    int colorCount = sizeof(colors) / sizeof(colors[0]);
    player.colorR = colors[colorIndex % colorCount][0];
    player.colorG = colors[colorIndex % colorCount][1];
    player.colorB = colors[colorIndex % colorCount][2];
    colorIndex++;
}

std::string CollabManager::generatePlayerID() {
    static int counter = 1;
    return "player_" + std::to_string(counter++);
}

bool CollabManager::connectToServer(const std::string& address, int port) {
    m_isConnected = true;
    OutputDebugStringA(("Connected to server " + address + ":" + std::to_string(port)).c_str());
    return true;
}

void CollabManager::disconnectFromServer() {
    m_isConnected = false;
    OutputDebugStringA("Disconnected from server");
}
