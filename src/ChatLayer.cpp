#include "ChatLayer.hpp"
#include "NetworkingManager.hpp"

using namespace geode::prelude;

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
    float panelW = 220.f; 
    float panelH = 300.f;
    float margin = 10.f;

    m_isOpen = false;
    this->setVisible(false);

    // --- Dynamic Level Name ---
    std::string levelName = "Editor Chat";
    if (auto* editor = LevelEditorLayer::get()) {
        if (editor->m_level && !std::string(editor->m_level->m_levelName).empty()) {
            levelName = editor->m_level->m_levelName;
        }
    }

    // 1. The Main Panel (The dark grey frame from your screenshot)
    m_bg = CCScale9Sprite::create("GJ_square05.png");
    m_bg->setContentSize({ panelW, panelH });
    
    float posX = winSize.width - (panelW / 2) - margin;
    float posY = winSize.height / 2;
    m_bg->setPosition({ posX, posY });
    this->addChild(m_bg);

    // 2. Corner Moons (Crescent sprites from your sketch)
    auto addMoon = [&](CCPoint pos, float rotation, bool flipX, bool flipY) {
        auto moon = CCSprite::createWithSpriteFrameName("GJ_commentSide_001.png");
        if (moon) {
            moon->setPosition(pos);
            moon->setRotation(rotation);
            moon->setFlipX(flipX);
            moon->setFlipY(flipY);
            moon->setScale(0.8f);
            m_bg->addChild(moon, 10);
        }
    };
    // Positioning them to wrap the corners like crescents
    addMoon({3, panelH - 3}, 0, false, false);      // Top Left
    addMoon({panelW - 3, panelH - 3}, 0, true, false); // Top Right
    addMoon({3, 3}, 0, false, true);               // Bottom Left
    addMoon({panelW - 3, 3}, 0, true, true);        // Bottom Right

    // 3. Title (Level Name)
    auto title = CCLabelBMFont::create(levelName.c_str(), "goldFont.fnt");
    title->setScale(0.55f);
    title->limitLabelWidth(panelW - 40, 0.55f, 0.1f);
    title->setPosition({ panelW / 2, panelH - 20 });
    m_bg->addChild(title);

    // 4. Scroll Layer for alternating bubbles
    m_scroll = ScrollLayer::create({ panelW - 20, panelH - 95 });
    m_scroll->setPosition({ 10, 75 });
    m_bg->addChild(m_scroll);

    // 5. Input Tray
    auto inputBG = CCScale9Sprite::create("square02b_001.png");
    inputBG->setColor({ 0, 0, 0 });
    inputBG->setOpacity(120);
    inputBG->setContentSize({ panelW - 25, 35 });
    inputBG->setPosition({ panelW / 2, 40 });
    m_bg->addChild(inputBG);

    m_input = CCTextInputNode::create(panelW - 65, 30.f, "Type here...", "chatFont.fnt");
    m_input->setPosition({ (panelW / 2) - 15, 40 });
    m_bg->addChild(m_input);

    // 6. Send Button (Flame Logo)
    auto logoSpr = CCSprite::create("logo.png"_spr);
    CCNode* sendNode = nullptr;
    if (logoSpr) {
        logoSpr->setScale(0.12f);
        sendNode = logoSpr;
    } else {
        sendNode = ButtonSprite::create("Send", 20, true, "goldFont.fnt", "GJ_button_01.png", 25, 0.5f);
    }

    auto sendBtn = CCMenuItemSpriteExtra::create(
        sendNode, this, menu_selector(ChatLayer::onSendMessage)
    );
    auto menu = CCMenu::create();
    menu->addChild(sendBtn);
    menu->setPosition({ panelW - 30, 40 });
    m_bg->addChild(menu);

    this->setKeyboardEnabled(true);
    this->setKeypadEnabled(true);
    
    return true;
}

void ChatLayer::toggle() {
    m_isOpen = !m_isOpen;
    m_bg->stopAllActions();
    
    if (m_isOpen) {
        this->setVisible(true);
        m_bg->setScale(0.1f);
        m_bg->setOpacity(0);
        m_bg->runAction(CCSpawn::create(
            CCEaseBackOut::create(CCScaleTo::create(0.3f, 1.0f)),
            CCFadeIn::create(0.2f),
            nullptr
        ));
    } else {
        m_bg->runAction(CCSequence::create(
            CCEaseIn::create(CCScaleTo::create(0.2f, 0.1f), 2.0f),
            CCHide::create(),
            nullptr
        ));
    }
}

void ChatLayer::addMessage(std::string const& user, std::string const& message, cocos2d::ccColor3B color) {
    if (!m_scroll || !m_scroll->m_contentLayer) return;

    // Determine if it's "You" for alternating sides
    bool isMe = (user == "You"); 
    auto container = CCNode::create();
    
    auto bubble = CCScale9Sprite::create("square02_001.png");
    bubble->setOpacity(140);
    bubble->setAnchorPoint({ isMe ? 1.0f : 0.0f, 1.0f });
    
    auto nameLabel = CCLabelBMFont::create(user.c_str(), "goldFont.fnt");
    nameLabel->setScale(0.32f);
    nameLabel->setColor(color);
    nameLabel->setAnchorPoint({ 0, 1 });
    nameLabel->setPosition({ 8, -5 });
    
    auto msgLabel = CCLabelBMFont::create(message.c_str(), "chatFont.fnt");
    msgLabel->setScale(0.42f);
    msgLabel->setAnchorPoint({ 0, 1 });
    msgLabel->setWidth(135.f);
    msgLabel->setPosition({ 8, -16 });

    bubble->addChild(nameLabel);
    bubble->addChild(msgLabel);
    
    float h = msgLabel->getScaledContentSize().height + 25;
    bubble->setContentSize({ 150, h });
    container->addChild(bubble);
    container->setContentSize(bubble->getContentSize());

    m_scroll->m_contentLayer->addChild(container);
    
    // Stack and align bubbles
    float totalH = 0;
    for (auto* child : CCArrayExt<CCNode*>(m_scroll->m_contentLayer->getChildren())) {
        totalH += child->getContentSize().height + 6;
    }

    float scrollH = std::max(m_scroll->getContentSize().height, totalH);
    m_scroll->m_contentLayer->setContentSize({ m_scroll->getContentSize().width, scrollH });

    float curY = scrollH;
    for (auto* child : CCArrayExt<CCNode*>(m_scroll->m_contentLayer->getChildren())) {
        curY -= (child->getContentSize().height + 6);
        // Align to right if "isMe", otherwise left
        float xPos = isMe ? m_scroll->getContentSize().width - 5 : 5;
        child->setPosition({ xPos, curY + child->getContentSize().height });
    }
    m_scroll->moveToTop();
}

void ChatLayer::onSendMessage(CCObject*) {
    if (!m_input) return;
    std::string text = m_input->getString();
    if (!text.empty()) {
        NetworkingManager::get()->sendMessage(text);
        m_input->setString("");
    }
}

void ChatLayer::keyDown(cocos2d::enumKeyCodes key, double repeat) {
    if (key == cocos2d::enumKeyCodes::KEY_Enter && m_isOpen) {
        onSendMessage(nullptr);
    }
    CCLayer::keyDown(key, repeat);
}