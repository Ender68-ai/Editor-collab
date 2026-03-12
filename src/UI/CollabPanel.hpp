#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class CollabPanel : public CCNode {
public:
    static CollabPanel* create();
    static void setup();
    static CollabPanel* get();
    
    void show();
    void hide();
    void toggle();
    void updatePlayerList();
    void addChatMessage(const std::string& playerName, const std::string& message);
    
private:
    bool init() override;
    void createUI();
    void onHostSession(CCObject* sender);
    void onJoinSession(CCObject* sender);
    void onDisconnect(CCObject* sender);
    void onSendMessage(CCObject* sender);
    void onSettings(CCObject* sender);
    
    static CollabPanel* s_instance;
    
    // UI Components
    CCMenu* m_mainMenu;
    CCNode* m_playerListNode;
    CCNode* m_chatNode;
    CCTextFieldNode* m_chatInput;
    CCLabelBMFont* m_statusLabel;
    CCScrollView* m_chatScroll;
    
    // Settings
    CCMenuItemToggler* m_lowLagToggle;
    CCMenuItemToggler* m_showCursorsToggle;
    CCMenuItemToggler* m_showOwnershipToggle;
};
