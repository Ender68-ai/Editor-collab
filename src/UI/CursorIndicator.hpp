#pragma once
#include <Geode/Geode.hpp>
#include <unordered_map>

using namespace geode::prelude;

struct CursorData {
    std::string playerID;
    CCPoint position;
    ccColor3B color;
    CCSprite* sprite;
    CCLabelBMFont* label;
};

class CursorIndicator : public CCNode {
public:
    static CursorIndicator* create();
    
    void updateCursor(const std::string& playerID, CCPoint position, ccColor3B color);
    void removeCursor(const std::string& playerID);
    void clearAllCursors();
    
private:
    bool init() override;
    
    std::unordered_map<std::string, CursorData> m_cursors;
    
    CCSprite* createCursorSprite(ccColor3B color);
};
