#include "ChatLayer.hpp"
#include <Geode/loader/Loader.hpp>

bool ChatLayer::init() {
    if (!CCLayer::init()) return false;

    m_bg = CCScale9Sprite::create("square02_001.png");
    m_bg->setContentSize({ 250, 180 });
    m_bg->setColor(getThemeColor());
    m_bg->setOpacity(255);
    m_bg->setPosition(CCDirector::get()->getWinSize() / 2);
    this->addChild(m_bg);

    auto title = CCLabelBMFont::create("Collab Chat", "goldFont.fnt");
    title->setScale(0.6f);
    title->setPosition({ m_bg->getContentSize().width / 2, m_bg->getContentSize().height - 15 });
    m_bg->addChild(title);

    this->setTouchEnabled(true);
    return true;
}

ccColor3B ChatLayer::getThemeColor() {
    auto loader = Loader::get();
    if (loader->getLoadedMod("geode.geodify") || loader->getLoadedMod("nekit.betteredit")) {
        return { 30, 30, 35 };
    }
    return { 40, 50, 70 };
}

void ChatLayer::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
}

bool ChatLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    auto pos = m_bg->convertTouchToNodeSpace(touch);
    auto size = m_bg->getContentSize();

    if (pos.x > size.width - 25 && pos.y < 25) {
        m_isResizing = true;
        return true;
    }
    
    if (CCRect{0, 0, size.width, size.height}.containsPoint(pos)) {
        m_isDragging = true;
        m_dragOffset = m_bg->getPosition() - this->convertTouchToNodeSpace(touch);
        return true;
    }
    return false;
}

void ChatLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    auto touchPos = this->convertTouchToNodeSpace(touch);
    if (m_isDragging) m_bg->setPosition(touchPos + m_dragOffset);
    if (m_isResizing) {
        float newW = std::max(150.0f, touchPos.x - m_bg->getPositionX() + (m_bg->getContentSize().width / 2));
        float newH = std::max(100.0f, m_bg->getPositionY() + (m_bg->getContentSize().height / 2) - touchPos.y);
        m_bg->setContentSize({ newW, newH });
    }
}

void ChatLayer::ccTouchEnded(CCTouch*, CCEvent*) {
    m_isDragging = m_isResizing = false;
}

ChatLayer* ChatLayer::create() {
    auto ret = new ChatLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}