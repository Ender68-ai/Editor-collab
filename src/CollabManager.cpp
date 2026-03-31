#include "CollabManager.hpp"
#include <Geode/Geode.hpp>
#include <ctime>

using namespace geode::prelude;

CollabManager* CollabManager::s_instance = nullptr;
std::mutex CollabManager::s_instanceMutex;

CollabManager* CollabManager::get() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) s_instance = new CollabManager();
    return s_instance;
}

void CollabManager::destroy() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    delete s_instance;
    s_instance = nullptr;
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

void CollabManager::saveData() {
    // Save data to file (simplified version)
    auto save = matjson::Value();
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
}

void CollabManager::loadData() {
    auto save = geode::Mod::get()->getSavedValue<matjson::Value>("collab-data");
    
    if (save.isNull()) return;
    
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
}
