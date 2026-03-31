#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <unordered_map>

using namespace geode::prelude;

enum class CollabRole {
    Editor = 0,
    Suggestor = 1,
    Viewer = 2
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
    
    CollabLevel() : levelID(0), levelName(""), hostAccountID(0) {}
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

class CollabManager {
protected:
    static CollabManager* s_instance;
    static std::mutex s_instanceMutex;
    
    std::unordered_map<int, CollabLevel> m_collabLevels;
    std::vector<TransferRequest> m_transferRequests;
    std::vector<int> m_hostedLevels;
    std::vector<int> m_collabedLevels;
    bool m_firstTimeSetup = true;
    
    CollabManager() = default;
    
public:
    static CollabManager* get();
    static void destroy();
    
    // Access to collab levels for popups
    const std::unordered_map<int, CollabLevel>& getCollabLevels() const { return m_collabLevels; }
    
    // Level management
    void enableCollabForLevel(int levelID, const std::string& levelName, int hostAccountID);
    void disableCollabForLevel(int levelID);
    bool isCollabEnabled(int levelID) const;
    bool isHostOfLevel(int levelID, int accountID) const;
    
    // Player management
    void addPlayerToLevel(int levelID, const CollabPlayer& player);
    void removePlayerFromLevel(int levelID, int accountID);
    void updatePlayerRole(int levelID, int accountID, CollabRole role);
    void togglePlayerStatus(int levelID, int accountID);
    std::vector<CollabPlayer> getPlayersInLevel(int levelID) const;
    
    // Transfer system
    void requestTransfer(int levelID, int fromAccountID, int toAccountID, const std::string& levelName);
    void acceptTransfer(const TransferRequest& request);
    void declineTransfer(const TransferRequest& request);
    std::vector<TransferRequest> getTransferRequestsForAccount(int accountID) const;
    
    // Level lists
    void addHostedLevel(int levelID);
    void addCollabedLevel(int levelID);
    std::vector<int> getHostedLevels() const;
    std::vector<int> getCollabedLevels() const;
    
    // Setup state
    bool isFirstTimeSetup() const { return m_firstTimeSetup; }
    void setFirstTimeSetupComplete() { m_firstTimeSetup = false; }
    
    // Data persistence
    void saveData();
    void loadData();
};
