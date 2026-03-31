#include "ChatLayer.hpp"
#include "NetworkingManager.hpp"

using namespace geode::prelude;

// Constants for easier tweaking
static constexpr float PANEL_WIDTH = 220.f;
static constexpr float PANEL_HEIGHT = 300.f;
static constexpr float PANEL_MARGIN = 10.f;
static constexpr int MAX_MESSAGE_HISTORY = 100;

ChatLayer* ChatLayer::create() {
    auto ret = new ChatLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ChatLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::get()->getWinSize();

    m_isOpen = false;
    this->setVisible(false);
    this->setTouchEnabled(true);

    // 1. Main Background Panel
    m_bg = CCScale9Sprite::create("GJ_square05.png");
    if (!m_bg) {
        log::warn("Failed to load GJ_square05.png, using fallback");
        m_bg = CCScale9Sprite::create("square02_001.png");
        if (!m_bg) {
            log::error("Failed to create background panel");
            return false;
        }
    }
    m_bg->setContentSize({ PANEL_WIDTH, PANEL_HEIGHT });
    m_bg->setPosition({ winSize.width - (PANEL_WIDTH / 2) - PANEL_MARGIN, winSize.height / 2 });
    this->addChild(m_bg);

    // 2. Corner Moons (Using GJ_moonsIcon_001.png)
    auto addMoon = [this](CCPoint pos, float rotation, bool flipX, bool flipY) {
        auto moon = CCSprite::createWithSpriteFrameName("GJ_moonsIcon_001.png");
        if (!moon) {
            log::warn("Moon sprite 'GJ_moonsIcon_001.png' failed to load");
            return;
        }
        moon->setPosition(pos);
        moon->setRotation(rotation);
        moon->setFlipX(flipX);
        moon->setFlipY(flipY);
        moon->setScale(0.8f);
        m_bg->addChild(moon, 10);
    };
    
    addMoon({10, PANEL_HEIGHT - 10}, 0, false, false);      // Top Left
    addMoon({PANEL_WIDTH - 10, PANEL_HEIGHT - 10}, 0, true, false); // Top Right
    addMoon({10, 10}, 0, false, true);                       // Bottom Left
    addMoon({PANEL_WIDTH - 10, 10}, 0, true, true);          // Bottom Right

    // 3. Title
    auto title = CCLabelBMFont::create("MEMENTO MORI", "goldFont.fnt");
    if (!title) {
        log::warn("Failed to create title with goldFont.fnt, using default font");
        title = CCLabelBMFont::create("MEMENTO MORI", "bigFont.fnt");
    }
    title->setScale(0.55f);
    title->setPosition({ PANEL_WIDTH / 2, PANEL_HEIGHT - 20 });
    m_bg->addChild(title);

    // 4. Close Button (Blue Flame Logo at the top)
    auto flameSpr = CCSprite::create("logo.png"_spr);
    if (flameSpr) flameSpr->setScale(0.12f);
    
    auto closeBtn = CCMenuItemSpriteExtra::create(
        flameSpr ? (CCNode*)flameSpr : (CCNode*)CCLabelBMFont::create("X", "goldFont.fnt"),
        this, menu_selector(ChatLayer::onClose)
    );
    
    auto topMenu = CCMenu::create();
    topMenu->addChild(closeBtn);
    topMenu->setPosition({ PANEL_WIDTH - 20, PANEL_HEIGHT - 20 });
    topMenu->setTouchPriority(-501);
    m_bg->addChild(topMenu);

    // 5. Messages Scroll Area
    m_scroll = ScrollLayer::create({ PANEL_WIDTH - 20, PANEL_HEIGHT - 95 });
    m_scroll->setPosition({ 10, 75 });
    m_bg->addChild(m_scroll);

    // 6. Empty State Hint
    auto hintLabel = CCLabelBMFont::create("No messages yet...", "chatFont.fnt");
    if (!hintLabel) {
        log::warn("Failed to create hint with chatFont.fnt, using goldFont.fnt");
        hintLabel = CCLabelBMFont::create("No messages yet...", "goldFont.fnt");
    }
    hintLabel->setScale(0.4f);
    hintLabel->setOpacity(80);
    hintLabel->setPosition({ PANEL_WIDTH / 2, PANEL_HEIGHT / 2 });
    hintLabel->setTag(999);
    m_bg->addChild(hintLabel);

    // 7. Input Area Background
    auto inputBG = CCScale9Sprite::create("square02b_001.png");
    if (!inputBG) {
        log::warn("Failed to load square02b_001.png, using fallback");
        inputBG = CCScale9Sprite::create("square02_001.png");
    }
    if (inputBG) {
        inputBG->setColor({ 0, 0, 0 });
        inputBG->setOpacity(180);
        inputBG->setContentSize({ PANEL_WIDTH - 25, 35 });
        inputBG->setPosition({ PANEL_WIDTH / 2, 40 });
        m_bg->addChild(inputBG);
    }

    // 8. Chat Input Field
    m_input = CCTextInputNode::create(PANEL_WIDTH - 70, 30.f, "Type here...", "chatFont.fnt");
    if (!m_input) {
        log::warn("Failed to create input with chatFont.fnt, using goldFont.fnt");
        m_input = CCTextInputNode::create(PANEL_WIDTH - 70, 30.f, "Type here...", "goldFont.fnt");
    }
    if (!m_input) {
        log::error("Failed to create input field");
        return false;
    }
    m_input->setPosition({ (PANEL_WIDTH / 2) - 15, 40 });
    m_input->setDelegate(this); 
    m_bg->addChild(m_input);

    // 9. Send Button (Arrow)
    auto arrowSpr = CCSprite::createWithSpriteFrameName("d_arrow_02_001.png");
    if (arrowSpr) arrowSpr->setScale(0.6f);

    auto sendBtn = CCMenuItemSpriteExtra::create(
        arrowSpr ? (CCNode*)arrowSpr : (CCNode*)ButtonSprite::create(">", 20, true, "goldFont.fnt", "GJ_button_01.png", 25, 0.5f),
        this, menu_selector(ChatLayer::onSendMessage)
    );
    
    auto bottomMenu = CCMenu::create();
    bottomMenu->addChild(sendBtn);
    bottomMenu->setPosition({ PANEL_WIDTH - 30, 40 });
    bottomMenu->setTouchPriority(-501); 
    m_bg->addChild(bottomMenu);

    return true;
}

void ChatLayer::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
}

void ChatLayer::onExit() {
    CCDirector::get()->getTouchDispatcher()->removeDelegate(this);
    CCLayer::onExit();
}

bool ChatLayer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (!this->isVisible() || !m_isOpen) return false;

    auto pos = m_bg->convertToNodeSpace(touch->getLocation());
    auto rect = CCRect{ 0, 0, m_bg->getContentSize().width, m_bg->getContentSize().height };

    if (rect.containsPoint(pos)) {
        // Focus the input node if it was clicked
        auto inputPos = m_input->convertToNodeSpace(touch->getLocation());
        auto inputRect = CCRect{ 0, 0, m_input->getContentSize().width, m_input->getContentSize().height };
        
        if (inputRect.containsPoint(inputPos)) {
            m_input->onClickTrackNode(true);
        } else {
            m_input->onClickTrackNode(false);
        }
        return true; 
    }
    return false; 
}

void ChatLayer::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    // Swallow touch events while panel is open
}

void ChatLayer::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    // Swallow touch events while panel is open
}

void ChatLayer::textChanged(CCTextInputNode* p0) {}

void ChatLayer::onClose(CCObject*) {
    this->toggle();
}

void ChatLayer::toggle() {
    m_isOpen = !m_isOpen;
    m_bg->stopAllActions();
    
    if (m_isOpen) {
        this->setVisible(true);
        m_bg->setScale(0.1f);
        m_bg->runAction(CCEaseBackOut::create(CCScaleTo::create(0.3f, 1.0f)));
        
        // Reset unread count when opening
        auto netManager = NetworkingManager::get();
        if (netManager) {
            netManager->resetUnreadCount();
        }
        
        // Hide badge when opening
        auto scene = CCDirector::get()->getRunningScene();
        if (scene) {
            if (auto badge = typeinfo_cast<CCLabelBMFont*>(scene->getChildByIDRecursive("chat-badge"_spr))) {
                badge->setVisible(false);
            }
        }
    } else {
        // Unfocus input when closing
        if (m_input) {
            m_input->onClickTrackNode(false);
        }
        
        m_bg->runAction(CCSequence::create(
            CCEaseIn::create(CCScaleTo::create(0.2f, 0.1f), 2.0f),
            CCHide::create(),
            nullptr
        ));
    }
}

void ChatLayer::addMessage(std::string const& user, std::string const& message, cocos2d::ccColor3B color, bool isMe) {
    if (!m_scroll || !m_scroll->m_contentLayer) return;

    // Remove "no messages" hint on first message
    if (auto hint = m_bg->getChildByTag(999)) {
        hint->removeFromParent();
    }

    // Limit message history - remove oldest messages first
    auto children = m_scroll->m_contentLayer->getChildren();
    while (children && children->count() >= MAX_MESSAGE_HISTORY) {
        auto oldestMsg = static_cast<CCNode*>(children->objectAtIndex(0));
        if (oldestMsg) oldestMsg->removeFromParent();
        children = m_scroll->m_contentLayer->getChildren(); // Refresh list
    }

    // Create message bubble with null checks
    auto container = CCNode::create();
    if (!container) return;
    container->setUserData(reinterpret_cast<void*>(isMe ? 1 : 0));
    
    auto bubble = CCScale9Sprite::create("square02_001.png");
    if (!bubble) {
        log::warn("Failed to create message bubble");
        container->release();
        return;
    }
    bubble->setOpacity(140);
    bubble->setAnchorPoint({ isMe ? 1.0f : 0.0f, 1.0f });
    
    auto nameLabel = CCLabelBMFont::create(user.c_str(), "goldFont.fnt");
    if (!nameLabel) {
        nameLabel = CCLabelBMFont::create(user.c_str(), "bigFont.fnt");
    }
    if (nameLabel) {
        nameLabel->setScale(0.32f);
        nameLabel->setColor(color);
        nameLabel->setAnchorPoint({ 0, 1 });
        nameLabel->setPosition({ 8, -5 });
    }
    
    auto msgLabel = CCLabelBMFont::create(message.c_str(), "chatFont.fnt");
    if (!msgLabel) {
        msgLabel = CCLabelBMFont::create(message.c_str(), "goldFont.fnt");
    }
    if (msgLabel) {
        msgLabel->setScale(0.42f);
        msgLabel->setAnchorPoint({ 0, 1 });
        msgLabel->setWidth(135.f);
        msgLabel->setPosition({ 8, -16 });
    }

    // Add labels to bubble if they exist
    if (nameLabel) bubble->addChild(nameLabel);
    if (msgLabel) bubble->addChild(msgLabel);
    
    // Calculate bubble height
    float h = 25; // Base height
    if (msgLabel) {
        h += msgLabel->getScaledContentSize().height;
    }
    bubble->setContentSize({ 150, h });
    container->addChild(bubble);
    container->setContentSize(bubble->getContentSize());
    m_scroll->m_contentLayer->addChild(container);
    
    // Optimized layout recalculation - only update positions
    this->updateMessageLayout();
    m_scroll->moveToTop();
}

void ChatLayer::updateMessageLayout() {
    if (!m_scroll || !m_scroll->m_contentLayer) return;
    
    auto children = m_scroll->m_contentLayer->getChildren();
    if (!children) return;
    
    // Calculate total height
    float totalH = 0;
    for (auto* child : CCArrayExt<CCNode*>(children)) {
        if (child) {
            totalH += child->getContentSize().height + 6;
        }
    }

    // Update content size - ensure it's at least as tall as the scroll area
    float scrollH = std::max(m_scroll->getContentSize().height, totalH);
    m_scroll->m_contentLayer->setContentSize({ m_scroll->getContentSize().width, scrollH });

    // Update positions - layout messages from top to bottom
    float curY = scrollH - 10; // Start 10px from top
    for (auto* child : CCArrayExt<CCNode*>(children)) {
        if (!child) continue;
        
        // Use stored alignment data
        bool childIsMe = reinterpret_cast<intptr_t>(child->getUserData()) == 1;
        float xPos = childIsMe ? m_scroll->getContentSize().width - child->getContentSize().width - 5 : 5;
        
        // Position with anchor point at top-left/right
        child->setAnchorPoint({ childIsMe ? 1.0f : 0.0f, 1.0f });
        child->setPosition({ xPos, curY });
        
        curY -= (child->getContentSize().height + 6);
    }
}

void ChatLayer::onSendMessage(CCObject*) {
    if (!m_input) return;
    
    std::string text = m_input->getString();
    if (text.empty()) return;
    
    // Null check for NetworkingManager
    auto netManager = NetworkingManager::get();
    if (!netManager) {
        log::error("NetworkingManager is null, cannot send message");
        return;
    }
    
    netManager->sendMessage(text);
    m_input->setString("");
    m_input->onClickTrackNode(false); // Unfocus after sending
}

void ChatLayer::keyDown(cocos2d::enumKeyCodes key, double repeat) {
    if (key == cocos2d::enumKeyCodes::KEY_Enter && m_isOpen && m_input) {
        onSendMessage(nullptr);
        return;
    }
    CCLayer::keyDown(key, repeat);
}