#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class PlayerList : public CCNode {
public:
    static PlayerList* create();
    
    void updatePlayerList();
    void addPlayer(const std::string& playerID, const std::string& playerName, ccColor3B color);
    void removePlayer(const std::string& playerID);
    
private:
    bool init() override;
    
    struct PlayerEntry {
        std::string id;
        std::string name;
        ccColor3B color;
        CCLabelBMFont* label;
        CCSprite* colorIndicator;
    };
    
    std::unordered_map<std::string, PlayerEntry> m_players;
    CCScrollView* m_scrollView;
    CCNode* m_contentNode;
};
