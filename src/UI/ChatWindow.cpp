#include "ChatWindow.hpp"
#include "../CollabManager.hpp"

ChatWindow* ChatWindow::create() {
    auto ret = new ChatWindow();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ChatWindow::init() {
    if (!CCNode::init()) return false;
    
    setContentSize({250, 200});
    
    // Background
    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setContentSize(getContentSize());
    bg->setPosition({getContentSize().width / 2, getContentSize().height / 2});
    bg->setOpacity(200);
    addChild(bg);
    
    // Title
    auto title = CCLabelBMFont::create("Chat", "chatFont.fnt");
    title->setPosition({getContentSize().width / 2, getContentSize().height - 15});
    addChild(title);
    
    // Chat content area
    m_contentNode = CCNode::create();
    m_contentNode->setContentSize({230, 150});
    
    m_scrollView = CCScrollView::create({230, 150}, m_contentNode);
    m_scrollView->setPosition({10, 30});
    m_scrollView->setDirection(kCCScrollViewDirectionVertical);
    addChild(m_scrollView);
    
    // Input field
    m_inputField = CCTextFieldNode::create("Type message...", "chatFont.fnt", 16);
    m_inputField->setPosition({125, 15});
    addChild(m_inputField);
    
    // Send button
    auto sendBtnSprite = ButtonSprite::create("Send");
    m_sendButton = CCMenuItemSpriteExtra::create(
        sendBtnSprite,
        this,
        (SEL_MenuHandler)&ChatWindow::onSend
    );
    
    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    menu->addChild(m_sendButton);
    m_sendButton->setPosition({200, 15});
    addChild(menu);
    
    return true;
}

void ChatWindow::addMessage(const std::string& playerName, const std::string& message, ccColor3B color) {
    ChatMessage msg;
    msg.playerName = playerName;
    msg.message = message;
    msg.color = color;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    m_messages.push_back(msg);
    
    // Limit message count
    if (m_messages.size() > MAX_MESSAGES) {
        m_messages.erase(m_messages.begin());
    }
    
    updateLayout();
}

void ChatWindow::clearMessages() {
    m_messages.clear();
    updateLayout();
}

void ChatWindow::updateLayout() {
    if (!m_contentNode) return;
    
    m_contentNode->removeAllChildrenWithCleanup(true);
    
    float yOffset = 0;
    for (const auto& msg : m_messages) {
        // Format: [PlayerName]: message
        std::string formattedText = msg.playerName + ": " + msg.message;
        
        auto label = CCLabelBMFont::create(formattedText.c_str(), "chatFont.fnt");
        label->setColor(msg.color);
        label->setPosition({5, 140 - yOffset});
        label->setAnchorPoint({0, 0.5f});
        label->setScale(0.8f);
        m_contentNode->addChild(label);
        
        yOffset += 20;
    }
    
    // Update scroll view content size
    m_contentNode->setContentSize({230, yOffset});
    m_scrollView->setContentOffset({0, yOffset > 150 ? 150 - yOffset : 0});
}

void ChatWindow::setVisible(bool visible) {
    CCNode::setVisible(visible);
    if (visible && m_inputField) {
        m_inputField->activate();
    } else if (m_inputField) {
        m_inputField->deactivate();
    }
}

void ChatWindow::onSend(CCObject* sender) {
    if (!m_inputField) return;
    
    std::string message = m_inputField->getString();
    if (!message.empty()) {
        CollabManager::get()->sendChatMessage(message);
        m_inputField->setString("");
    }
}
