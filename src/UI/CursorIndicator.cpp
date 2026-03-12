#include "CursorIndicator.hpp"
#include "../CollabManager.hpp"

CursorIndicator* CursorIndicator::create() {
    auto ret = new CursorIndicator();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CursorIndicator::init() {
    if (!CCNode::init()) return false;
    
    scheduleUpdate();
    return true;
}

void CursorIndicator::updateCursor(const std::string& playerID, CCPoint position, ccColor3B color) {
    // Don't show cursor for local player
    if (playerID == CollabManager::get()->getLocalPlayerID()) {
        return;
    }
    
    auto it = m_cursors.find(playerID);
    if (it != m_cursors.end()) {
        // Update existing cursor
        it->second.position = position;
        it->second.sprite->setPosition(position);
        it->second.label->setPosition({position.x, position.y + 15});
    } else {
        // Create new cursor
        CursorData cursor;
        cursor.playerID = playerID;
        cursor.position = position;
        cursor.color = color;
        cursor.sprite = createCursorSprite(color);
        cursor.label = CCLabelBMFont::create(playerID.c_str(), "chatFont.fnt");
        cursor.label->setColor(color);
        cursor.label->setScale(0.7f);
        
        cursor.sprite->setPosition(position);
        cursor.label->setPosition({position.x, position.y + 15});
        
        addChild(cursor.sprite);
        addChild(cursor.label);
        
        m_cursors[playerID] = cursor;
    }
}

void CursorIndicator::removeCursor(const std::string& playerID) {
    auto it = m_cursors.find(playerID);
    if (it != m_cursors.end()) {
        if (it->second.sprite) {
            it->second.sprite->removeFromParent();
        }
        if (it->second.label) {
            it->second.label->removeFromParent();
        }
        m_cursors.erase(it);
    }
}

void CursorIndicator::clearAllCursors() {
    for (auto& [playerID, cursor] : m_cursors) {
        if (cursor.sprite) {
            cursor.sprite->removeFromParent();
        }
        if (cursor.label) {
            cursor.label->removeFromParent();
        }
    }
    m_cursors.clear();
}

CCSprite* CursorIndicator::createCursorSprite(ccColor3B color) {
    // Create a simple cursor sprite
    auto cursor = CCSprite::createWithSpriteFrameName("square02_001.png");
    cursor->setColor(color);
    cursor->setScale(0.3f);
    cursor->setRotation(45.0f); // Diamond shape
    return cursor;
}

void CursorIndicator::update(float dt) {
    CCNode::update(dt);
    
    // Update cursor positions from CollabManager
    auto& manager = CollabManager::get();
    const auto& players = manager.getPlayers();
    
    for (const auto& [playerID, player] : players) {
        updateCursor(playerID, player.cursorPos, player.color);
    }
    
    // Remove cursors for disconnected players
    std::vector<std::string> toRemove;
    for (const auto& [playerID, cursor] : m_cursors) {
        if (players.find(playerID) == players.end()) {
            toRemove.push_back(playerID);
        }
    }
    
    for (const auto& playerID : toRemove) {
        removeCursor(playerID);
    }
}
