#include "PlayerList.hpp"
#include "../CollabManager.hpp"

PlayerList* PlayerList::create() {
    auto ret = new PlayerList();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool PlayerList::init() {
    if (!CCNode::init()) return false;
    
    setContentSize({200, 150});
    
    // Background
    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setContentSize(getContentSize());
    bg->setPosition({getContentSize().width / 2, getContentSize().height / 2});
    bg->setOpacity(180);
    addChild(bg);
    
    // Title
    auto title = CCLabelBMFont::create("Players", "chatFont.fnt");
    title->setPosition({getContentSize().width / 2, getContentSize().height - 20});
    addChild(title);
    
    // Scroll view for player list
    m_contentNode = CCNode::create();
    m_contentNode->setContentSize({180, 100});
    
    m_scrollView = CCScrollView::create({180, 100}, m_contentNode);
    m_scrollView->setPosition({10, 10});
    m_scrollView->setDirection(kCCScrollViewDirectionVertical);
    addChild(m_scrollView);
    
    scheduleUpdate();
    return true;
}

void PlayerList::updatePlayerList() {
    if (!m_contentNode) return;
    
    m_contentNode->removeAllChildrenWithCleanup(true);
    m_players.clear();
    
    auto& manager = CollabManager::get();
    const auto& players = manager.getPlayers();
    
    float yOffset = 0;
    for (const auto& [playerID, player] : players) {
        addPlayer(playerID, player.name, player.color);
    }
}

void PlayerList::addPlayer(const std::string& playerID, const std::string& playerName, ccColor3B color) {
    if (!m_contentNode) return;
    
    PlayerEntry entry;
    entry.id = playerID;
    entry.name = playerName;
    entry.color = color;
    
    // Color indicator
    entry.colorIndicator = CCSprite::createWithSpriteFrameName("square02_001.png");
    entry.colorIndicator->setColor(color);
    entry.colorIndicator->setScale(0.3f);
    entry.colorIndicator->setPosition({10, 90 - m_players.size() * 20});
    m_contentNode->addChild(entry.colorIndicator);
    
    // Player name
    entry.label = CCLabelBMFont::create(playerName.c_str(), "chatFont.fnt");
    entry.label->setColor(color);
    entry.label->setPosition({25, 90 - m_players.size() * 20});
    entry.label->setAnchorPoint({0, 0.5f});
    m_contentNode->addChild(entry.label);
    
    m_players[playerID] = entry;
    
    // Update content size
    m_contentNode->setContentSize({180, static_cast<float>(m_players.size() * 20 + 20)});
    m_scrollView->setContentSize(m_contentNode->getContentSize());
}

void PlayerList::removePlayer(const std::string& playerID) {
    auto it = m_players.find(playerID);
    if (it != m_players.end()) {
        if (it->second.label) {
            it->second.label->removeFromParent();
        }
        if (it->second.colorIndicator) {
            it->second.colorIndicator->removeFromParent();
        }
        m_players.erase(it);
        
        // Recreate list to update positions
        updatePlayerList();
    }
}
