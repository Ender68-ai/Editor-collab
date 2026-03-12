#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

struct ChatMessage {
    std::string playerName;
    std::string message;
    ccColor3B color;
    double timestamp;
};

class ChatWindow : public CCNode {
public:
    static ChatWindow* create();
    
    void addMessage(const std::string& playerName, const std::string& message, ccColor3B color);
    void clearMessages();
    void setVisible(bool visible) override;
    
private:
    bool init() override;
    void updateLayout();
    
    std::vector<ChatMessage> m_messages;
    CCScrollView* m_scrollView;
    CCNode* m_contentNode;
    CCTextFieldNode* m_inputField;
    CCMenu* m_sendButton;
    void onSend(CCObject* sender);
    
    static const int MAX_MESSAGES = 50;
};
