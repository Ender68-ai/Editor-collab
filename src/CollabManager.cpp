#include "CollabManager.hpp"
#include <Geode/Geode.hpp>
#include <ctime>
#include <cmath>
#include <algorithm>

using namespace geode::prelude;

// ============================================================================
// NETWORK MANAGER IMPLEMENTATION
// ============================================================================

NetworkManager* NetworkManager::s_instance = nullptr;
std::mutex NetworkManager::s_instanceMutex;

NetworkManager::NetworkManager() 
    : m_localAccountID(-1), m_localDeviceType(DeviceType::PC), 
      m_localPlayerName(""), m_isConnected(false) {}

NetworkManager::~NetworkManager() {}

NetworkManager* NetworkManager::get() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) s_instance = new NetworkManager();
    return s_instance;
}

void NetworkManager::destroy() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void NetworkManager::initialize(int accountID, const std::string& playerName, DeviceType device) {
    m_localAccountID = accountID;
    m_localPlayerName = playerName;
    m_localDeviceType = device;
    log::info("NetworkManager initialized: AccountID={}, PlayerName={}, Device={}", 
              accountID, playerName, static_cast<int>(device));
}

void NetworkManager::sendPlayerPosition(float x, float y) {
    if (!m_isConnected) return;
    
    PlayerPositionPacket packet;
    packet.header.type = PacketType::PlayerPosition;
    packet.header.length = sizeof(PlayerPositionPacket);
    packet.header.senderAccountID = m_localAccountID;
    packet.header.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.posX = x;
    packet.posY = y;
    packet.deviceType = static_cast<uint8_t>(m_localDeviceType);
    
    log::debug("Sending PlayerPosition: ({}, {})", x, y);
}

void NetworkManager::sendObjectLock(int objectID) {
    if (!m_isConnected) return;
    
    ObjectLockPacket packet;
    packet.header.type = PacketType::ObjectLock;
    packet.header.length = sizeof(ObjectLockPacket);
    packet.header.senderAccountID = m_localAccountID;
    packet.header.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.objectID = objectID;
    packet.accountID = m_localAccountID;
    
    log::debug("Sending ObjectLock for object: {}", objectID);
}

void NetworkManager::sendObjectUnlock(int objectID) {
    if (!m_isConnected) return;
    
    ObjectUnlockPacket packet;
    packet.header.type = PacketType::ObjectUnlock;
    packet.header.length = sizeof(ObjectUnlockPacket);
    packet.header.senderAccountID = m_localAccountID;
    packet.header.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.objectID = objectID;
    packet.accountID = m_localAccountID;
    
    log::debug("Sending ObjectUnlock for object: {}", objectID);
}

void NetworkManager::sendObjectMove(int objectID, float newX, float newY) {
    if (!m_isConnected) return;
    
    ObjectMovePacket packet;
    packet.header.type = PacketType::ObjectMove;
    packet.header.length = sizeof(ObjectMovePacket);
    packet.header.senderAccountID = m_localAccountID;
    packet.header.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.objectID = objectID;
    packet.newX = newX;
    packet.newY = newY;
    
    log::debug("Sending ObjectMove: objectID={}, pos=({}, {})", objectID, newX, newY);
}

void NetworkManager::sendHandshake() {
    if (m_localAccountID < 0) return;
    
    HandshakePacket packet;
    packet.header.type = PacketType::Handshake;
    packet.header.length = sizeof(HandshakePacket);
    packet.header.senderAccountID = m_localAccountID;
    packet.header.timestamp = static_cast<uint32_t>(time(nullptr));
    packet.accountID = m_localAccountID;
    packet.deviceType = static_cast<uint8_t>(m_localDeviceType);
    #ifdef _WIN32
    strncpy_s(packet.playerName, sizeof(packet.playerName), m_localPlayerName.c_str(), _TRUNCATE);
    #else
    std::strncpy(packet.playerName, m_localPlayerName.c_str(), sizeof(packet.playerName) - 1);
    packet.playerName[sizeof(packet.playerName) - 1] = '\0';
    #endif
    
    log::info("Sending Handshake: AccountID={}, PlayerName={}", m_localAccountID, m_localPlayerName);
}

bool NetworkManager::hasIncomingPackets() const {
    std::lock_guard<std::mutex> lock(m_packetQueueMutex);
    return !m_incomingPackets.empty();
}

std::vector<uint8_t> NetworkManager::popIncomingPacket() {
    std::lock_guard<std::mutex> lock(m_packetQueueMutex);
    if (m_incomingPackets.empty()) return {};
    
    auto packet = m_incomingPackets.front();
    m_incomingPackets.pop();
    return packet;
}

void NetworkManager::onPacketReceived(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(m_packetQueueMutex);
    m_incomingPackets.push(data);
}

// ============================================================================
// COLLAB MANAGER IMPLEMENTATION
// ============================================================================

CollabManager* CollabManager::s_instance = nullptr;
std::mutex CollabManager::s_instanceMutex;

CollabManager::CollabManager() 
    : m_firstTimeSetup(true), m_currentLevelID(-1), 
      m_localAccountID(-1), m_localDeviceType(DeviceType::PC),
      m_localPlayerPosition(0.0f, 0.0f) {}

CollabManager::~CollabManager() {}

CollabManager* CollabManager::get() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) s_instance = new CollabManager();
    return s_instance;
}

void CollabManager::destroy() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void CollabManager::enableCollabForLevel(int levelID, const std::string& levelName, int hostAccountID) {
    m_collabLevels[levelID] = CollabLevel(levelID, levelName, hostAccountID);
    m_collabLevels[levelID].collabEnabled = true;
    addHostedLevel(levelID);
    saveData();
    log::info("Enabled collaboration for level {} ({})", levelID, levelName);
}

void CollabManager::disableCollabForLevel(int levelID) {
    auto it = m_collabLevels.find(levelID);
    if (it != m_collabLevels.end()) {
        it->second.collabEnabled = false;
        saveData();
        log::info("Disabled collaboration for level {}", levelID);
    }
}

bool CollabManager::isCollabEnabled(int levelID) const {
    auto it = m_collabLevels.find(levelID);
    return it != m_collabLevels.end() && it->second.collabEnabled;
}

bool CollabManager::isHostOfLevel(int levelID, int accountID) const {
    auto it = m_collabLevels.find(levelID);
    return it != m_collabLevels.end() && it->second.hostAccountID == accountID;
}

void CollabManager::addPlayerToLevel(int levelID, const CollabPlayer& player) {
    auto it = m_collabLevels.find(levelID);
    if (it != m_collabLevels.end()) {
        // Check if player already exists
        for (auto& existingPlayer : it->second.players) {
            if (existingPlayer.accountID == player.accountID) {
                existingPlayer = player; // Update existing player
                saveData();
                return;
            }
        }
        it->second.players.push_back(player);
        saveData();
        log::info("Added player {} to level {}", player.name, levelID);
    }
}

void CollabManager::removePlayerFromLevel(int levelID, int accountID) {
    auto it = m_collabLevels.find(levelID);
    if (it != m_collabLevels.end()) {
        auto& players = it->second.players;
        players.erase(
            std::remove_if(players.begin(), players.end(),
                [accountID](const CollabPlayer& p) { return p.accountID == accountID; }),
            players.end()
        );
        saveData();
        log::info("Removed player {} from level {}", accountID, levelID);
    }
}

void CollabManager::updatePlayerRole(int levelID, int accountID, CollabRole role) {
    auto it = m_collabLevels.find(levelID);
    if (it != m_collabLevels.end()) {
        for (auto& player : it->second.players) {
            if (player.accountID == accountID) {
                player.role = role;
                saveData();
                log::info("Updated role for player {} in level {}", accountID, levelID);
                return;
            }
        }
    }
}

void CollabManager::togglePlayerStatus(int levelID, int accountID) {
    auto it = m_collabLevels.find(levelID);
    if (it != m_collabLevels.end()) {
        for (auto& player : it->second.players) {
            if (player.accountID == accountID) {
                player.enabled = !player.enabled;
                saveData();
                log::info("Toggled status for player {} in level {}", accountID, levelID);
                return;
            }
        }
    }
}

std::vector<CollabPlayer> CollabManager::getPlayersInLevel(int levelID) const {
    auto it = m_collabLevels.find(levelID);
    if (it != m_collabLevels.end()) {
        return it->second.players;
    }
    return {};
}

void CollabManager::requestTransfer(int levelID, int fromAccountID, int toAccountID, const std::string& levelName) {
    TransferRequest request(fromAccountID, toAccountID, levelID, levelName);
    m_transferRequests.push_back(request);
    saveData();
    log::info("Created transfer request for level {} from {} to {}", levelID, fromAccountID, toAccountID);
}

void CollabManager::acceptTransfer(const TransferRequest& request) {
    // Update level host
    auto it = m_collabLevels.find(request.levelID);
    if (it != m_collabLevels.end()) {
        // Remove old host from players and add as regular player if they're not the host
        if (it->second.hostAccountID != request.toAccountID) {
            // Add old host as editor if they're not already in players
            bool found = false;
            for (const auto& player : it->second.players) {
                if (player.accountID == it->second.hostAccountID) {
                    found = true;
                    break;
                }
            }
            if (!found && it->second.hostAccountID != request.toAccountID) {
                it->second.players.emplace_back("Previous Host", it->second.hostAccountID, CollabRole::Editor);
            }
        }
        
        it->second.hostAccountID = request.toAccountID;
        
        // Remove transfer request
        m_transferRequests.erase(
            std::remove_if(m_transferRequests.begin(), m_transferRequests.end(),
                [&request](const TransferRequest& r) {
                    return r.fromAccountID == request.fromAccountID && 
                           r.toAccountID == request.toAccountID && 
                           r.levelID == request.levelID;
                }),
            m_transferRequests.end()
        );
        
        saveData();
        log::info("Transfer accepted for level {} - new host: {}", request.levelID, request.toAccountID);
    }
}

void CollabManager::declineTransfer(const TransferRequest& request) {
    // Remove transfer request
    m_transferRequests.erase(
        std::remove_if(m_transferRequests.begin(), m_transferRequests.end(),
            [&request](const TransferRequest& r) {
                return r.fromAccountID == request.fromAccountID && 
                       r.toAccountID == request.toAccountID && 
                       r.levelID == request.levelID;
            }),
        m_transferRequests.end()
    );
    
    saveData();
    log::info("Transfer declined for level {} from {} to {}", request.levelID, request.fromAccountID, request.toAccountID);
}

std::vector<TransferRequest> CollabManager::getTransferRequestsForAccount(int accountID) const {
    std::vector<TransferRequest> requests;
    for (const auto& request : m_transferRequests) {
        if (request.toAccountID == accountID) {
            requests.push_back(request);
        }
    }
    return requests;
}

void CollabManager::addHostedLevel(int levelID) {
    if (std::find(m_hostedLevels.begin(), m_hostedLevels.end(), levelID) == m_hostedLevels.end()) {
        m_hostedLevels.push_back(levelID);
        saveData();
    }
}

void CollabManager::addCollabedLevel(int levelID) {
    if (std::find(m_collabedLevels.begin(), m_collabedLevels.end(), levelID) == m_collabedLevels.end()) {
        m_collabedLevels.push_back(levelID);
        saveData();
    }
}

std::vector<int> CollabManager::getHostedLevels() const {
    return m_hostedLevels;
}

std::vector<int> CollabManager::getCollabedLevels() const {
    return m_collabedLevels;
}

// ========== REAL-TIME COLLABORATION ==========

void CollabManager::addRemotePlayer(int accountID, const std::string& playerName, DeviceType device) {
    // Generate a random, highly visible color
    ccColor3B randomColor = {
        static_cast<uint8_t>(50 + rand() % 205),
        static_cast<uint8_t>(50 + rand() % 205),
        static_cast<uint8_t>(50 + rand() % 205)
    };
    
    m_remotePlayers[accountID] = RemotePlayer(accountID, playerName, device, randomColor);
    log::info("Added remote player: AccountID={}, Name={}, Device={}", 
              accountID, playerName, static_cast<int>(device));
}

void CollabManager::removeRemotePlayer(int accountID) {
    auto it = m_remotePlayers.find(accountID);
    if (it != m_remotePlayers.end()) {
        auto& remotePlayer = it->second;
        
        // Clean up visual elements
        if (remotePlayer.cursorSprite) {
            remotePlayer.cursorSprite->removeFromParent();
            remotePlayer.cursorSprite = nullptr;
        }
        if (remotePlayer.nameLabel) {
            remotePlayer.nameLabel->removeFromParent();
            remotePlayer.nameLabel = nullptr;
        }
        
        m_remotePlayers.erase(it);
        log::info("Removed remote player: AccountID={}", accountID);
    }
}

RemotePlayer* CollabManager::getRemotePlayer(int accountID) {
    auto it = m_remotePlayers.find(accountID);
    if (it != m_remotePlayers.end()) {
        return &it->second;
    }
    return nullptr;
}

void CollabManager::updateRemotePlayerPosition(int accountID, float x, float y) {
    auto player = getRemotePlayer(accountID);
    if (player) {
        player->position = CCPoint(x, y);
        player->lastUpdateTime = static_cast<float>(CCDirector::sharedDirector()->getTotalFrames()) / 60.0f;
        
        // Update visual position if sprite exists
        if (player->cursorSprite) {
            player->cursorSprite->setPosition({x, y});
        }
    }
}

// ========== OBJECT LOCKING ==========

bool CollabManager::isObjectLocked(int objectID) const {
    auto it = m_lockedObjects.find(objectID);
    return it != m_lockedObjects.end() && it->second.lockedByAccountID >= 0;
}

bool CollabManager::canLockObject(int objectID, int accountID) {
    auto it = m_lockedObjects.find(objectID);
    if (it == m_lockedObjects.end()) {
        return true; // Object not locked
    }
    return it->second.lockedByAccountID == accountID;
}

void CollabManager::lockObject(int objectID, int accountID) {
    m_lockedObjects[objectID] = ObjectLockState(objectID, accountID);
    log::debug("Object locked: objectID={}, accountID={}", objectID, accountID);
    
    // Notify network manager
    auto netMgr = NetworkManager::get();
    if (netMgr && netMgr->isConnected()) {
        netMgr->sendObjectLock(objectID);
    }
}

void CollabManager::unlockObject(int objectID) {
    auto it = m_lockedObjects.find(objectID);
    if (it != m_lockedObjects.end()) {
        int accountID = it->second.lockedByAccountID;
        m_lockedObjects.erase(it);
        log::debug("Object unlocked: objectID={}, accountID={}", objectID, accountID);
        
        // Notify network manager
        auto netMgr = NetworkManager::get();
        if (netMgr && netMgr->isConnected()) {
            netMgr->sendObjectUnlock(objectID);
        }
    }
}

int CollabManager::getObjectLockOwner(int objectID) const {
    auto it = m_lockedObjects.find(objectID);
    if (it != m_lockedObjects.end()) {
        return it->second.lockedByAccountID;
    }
    return -1;
}

// ========== PACKET PROCESSING ==========

void CollabManager::processIncomingPackets() {
    auto netMgr = NetworkManager::get();
    if (!netMgr) return;
    
    while (netMgr->hasIncomingPackets()) {
        auto data = netMgr->popIncomingPacket();
        if (data.empty() || data.size() < sizeof(PacketHeader)) continue;
        
        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data.data());
        
        switch (header->type) {
            case PacketType::PlayerPosition: {
                if (data.size() >= sizeof(PlayerPositionPacket)) {
                    const PlayerPositionPacket* packet = reinterpret_cast<const PlayerPositionPacket*>(data.data());
                    updateRemotePlayerPosition(header->senderAccountID, packet->posX, packet->posY);
                }
                break;
            }
            
            case PacketType::ObjectLock: {
                if (data.size() >= sizeof(ObjectLockPacket)) {
                    const ObjectLockPacket* packet = reinterpret_cast<const ObjectLockPacket*>(data.data());
                    m_lockedObjects[packet->objectID] = ObjectLockState(packet->objectID, packet->accountID);
                    log::debug("Remote lock received: objectID={}, accountID={}", packet->objectID, packet->accountID);
                }
                break;
            }
            
            case PacketType::ObjectUnlock: {
                if (data.size() >= sizeof(ObjectUnlockPacket)) {
                    const ObjectUnlockPacket* packet = reinterpret_cast<const ObjectUnlockPacket*>(data.data());
                    unlockObject(packet->objectID);
                }
                break;
            }
            
            case PacketType::ObjectMove: {
                if (data.size() >= sizeof(ObjectMovePacket)) {
                    const ObjectMovePacket* packet = reinterpret_cast<const ObjectMovePacket*>(data.data());
                    log::debug("Remote object move: objectID={}, pos=({}, {})", 
                               packet->objectID, packet->newX, packet->newY);
                }
                break;
            }
            
            case PacketType::Handshake: {
                if (data.size() >= sizeof(HandshakePacket)) {
                    const HandshakePacket* packet = reinterpret_cast<const HandshakePacket*>(data.data());
                    addRemotePlayer(packet->accountID, packet->playerName, 
                                    static_cast<DeviceType>(packet->deviceType));
                }
                break;
            }
            
            default:
                log::warn("Unknown packet type: {}", static_cast<int>(header->type));
                break;
        }
    }
}

void CollabManager::saveData() {
    // Save data to file (simplified version)
    auto save = matjson::Value::object();
    save["firstTimeSetup"] = m_firstTimeSetup;
    
    auto hostedArray = matjson::Value::array();
    for (int levelID : m_hostedLevels) {
        hostedArray.push(levelID);
    }
    save["hostedLevels"] = hostedArray;
    
    auto collabedArray = matjson::Value::array();
    for (int levelID : m_collabedLevels) {
        collabedArray.push(levelID);
    }
    save["collabedLevels"] = collabedArray;
    
    // Save collab levels
    auto levelsArray = matjson::Value::array();
    for (const auto& [levelID, level] : m_collabLevels) {
        auto levelData = matjson::Value();
        levelData["levelID"] = level.levelID;
        levelData["levelName"] = level.levelName;
        levelData["hostAccountID"] = level.hostAccountID;
        levelData["collabEnabled"] = level.collabEnabled;
        
        auto playersArray = matjson::Value::array();
        for (const auto& player : level.players) {
            auto playerData = matjson::Value();
            playerData["name"] = player.name;
            playerData["accountID"] = player.accountID;
            playerData["role"] = static_cast<int>(player.role);
            playerData["enabled"] = player.enabled;
            playersArray.push(playerData);
        }
        levelData["players"] = playersArray;
        
        levelsArray.push(levelData);
    }
    save["collabLevels"] = levelsArray;
    
    // Save transfer requests
    auto requestsArray = matjson::Value::array();
    for (const auto& request : m_transferRequests) {
        auto requestData = matjson::Value();
        requestData["fromAccountID"] = request.fromAccountID;
        requestData["toAccountID"] = request.toAccountID;
        requestData["levelID"] = request.levelID;
        requestData["levelName"] = request.levelName;
        requestData["timestamp"] = request.timestamp;
        requestsArray.push(requestData);
    }
    save["transferRequests"] = requestsArray;
    
    geode::Mod::get()->setSavedValue("collab-data", save);
    log::debug("Data saved to persistent storage");
}

void CollabManager::loadData() {
    auto save = geode::Mod::get()->getSavedValue<matjson::Value>("collab-data");
    
    if (save.isNull()) {
        log::info("No saved collab data found, starting fresh");
        return;
    }
    
    m_firstTimeSetup = save["firstTimeSetup"].asBool().unwrapOr(true);
    
    // Load hosted levels
    if (auto hostedArray = save["hostedLevels"].asArray()) {
        for (const auto& levelIDValue : *hostedArray) {
            if (auto levelID = levelIDValue.asInt()) {
                m_hostedLevels.push_back(*levelID);
            }
        }
    }
    
    // Load collabed levels
    if (auto collabedArray = save["collabedLevels"].asArray()) {
        for (const auto& levelIDValue : *collabedArray) {
            if (auto levelID = levelIDValue.asInt()) {
                m_collabedLevels.push_back(*levelID);
            }
        }
    }
    
    // Load collab levels
    if (auto levelsArray = save["collabLevels"].asArray()) {
        for (const auto& levelDataValue : *levelsArray) {
            CollabLevel level;
            
            if (auto levelID = levelDataValue["levelID"].asInt()) {
                level.levelID = *levelID;
            }
            if (auto levelName = levelDataValue["levelName"].asString()) {
                level.levelName = *levelName;
            }
            if (auto hostAccountID = levelDataValue["hostAccountID"].asInt()) {
                level.hostAccountID = *hostAccountID;
            }
            if (auto collabEnabled = levelDataValue["collabEnabled"].asBool()) {
                level.collabEnabled = *collabEnabled;
            }
            
            if (auto playersArray = levelDataValue["players"].asArray()) {
                for (const auto& playerDataValue : *playersArray) {
                    CollabPlayer player("", 0, CollabRole::Editor, false);
                    
                    if (auto name = playerDataValue["name"].asString()) {
                        player.name = *name;
                    }
                    if (auto accountID = playerDataValue["accountID"].asInt()) {
                        player.accountID = *accountID;
                    }
                    if (auto role = playerDataValue["role"].asInt()) {
                        player.role = static_cast<CollabRole>(*role);
                    }
                    if (auto enabled = playerDataValue["enabled"].asBool()) {
                        player.enabled = *enabled;
                    }
                    
                    level.players.push_back(player);
                }
            }
            
            m_collabLevels[level.levelID] = level;
        }
    }
    
    // Load transfer requests
    if (auto requestsArray = save["transferRequests"].asArray()) {
        for (const auto& requestDataValue : *requestsArray) {
            TransferRequest request(0, 0, 0, "");
            
            if (auto fromAccountID = requestDataValue["fromAccountID"].asInt()) {
                request.fromAccountID = *fromAccountID;
            }
            if (auto toAccountID = requestDataValue["toAccountID"].asInt()) {
                request.toAccountID = *toAccountID;
            }
            if (auto levelID = requestDataValue["levelID"].asInt()) {
                request.levelID = *levelID;
            }
            if (auto levelName = requestDataValue["levelName"].asString()) {
                request.levelName = *levelName;
            }
            if (auto timestamp = requestDataValue["timestamp"].asString()) {
                request.timestamp = *timestamp;
            }
            
            m_transferRequests.push_back(request);
        }
    }
    
    log::info("Data loaded from persistent storage");
}
