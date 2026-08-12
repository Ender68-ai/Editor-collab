#pragma once
#include <Geode/Geode.hpp>

namespace mpedit {

template<typename T>
class Build {
    T* m_node;
public:
    Build(T* node) : m_node(node) {}

    static Build<T> create() {
        return Build(T::create());
    }

    template<typename... Args>
    static Build<T> create(Args... args) {
        return Build(T::create(args...));
    }

    Build<T>& id(const char* id) {
        m_node->setID(id);
        return *this;
    }

    Build<T>& pos(cocos2d::CCPoint const& p) {
        m_node->setPosition(p);
        return *this;
    }

    Build<T>& pos(float x, float y) {
        m_node->setPosition({x, y});
        return *this;
    }

    Build<T>& scale(float s) {
        m_node->setScale(s);
        return *this;
    }

    Build<T>& anchorPoint(float x, float y) {
        m_node->setAnchorPoint({x, y});
        return *this;
    }

    Build<T>& contentSize(cocos2d::CCSize const& s) {
        m_node->setContentSize(s);
        return *this;
    }

    Build<T>& contentSize(float w, float h) {
        m_node->setContentSize({w, h});
        return *this;
    }

    Build<T>& zOrder(int z) {
        m_node->setZOrder(z);
        return *this;
    }

    Build<T>& parent(cocos2d::CCNode* p) {
        if (p) p->addChild(m_node);
        return *this;
    }

    Build<T>& child(cocos2d::CCNode* c) {
        if (c) m_node->addChild(c);
        return *this;
    }

    Build<T>& layout(geode::Layout* l) {
        m_node->setLayout(l);
        return *this;
    }

    Build<T>& updateLayout() {
        m_node->updateLayout();
        return *this;
    }

    Build<T>& opacity(uint8_t op) {
        if (auto rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(m_node)) {
            rgba->setOpacity(op);
        }
        return *this;
    }

    Build<T>& color(cocos2d::ccColor3B const& c) {
        if (auto rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(m_node)) {
            rgba->setColor(c);
        }
        return *this;
    }

    template<typename Callable>
    Build<T>& with(Callable cb) {
        cb(m_node);
        return *this;
    }

    template<typename C>
    Build<CCMenuItemSpriteExtra> intoMenuItem(C callback) {
        auto btn = geode::cocos::CCMenuItemExt::createSpriteExtra(m_node, callback);
        return Build<CCMenuItemSpriteExtra>(btn);
    }

    T* collect() {
        return m_node;
    }
    
    operator T*() {
        return m_node;
    }
};

template<>
class Build<cocos2d::CCSprite> {
    cocos2d::CCSprite* m_node;
public:
    Build(cocos2d::CCSprite* node) : m_node(node) {}

    static Build<cocos2d::CCSprite> create(const char* name) {
        return Build(cocos2d::CCSprite::create(name));
    }

    static Build<cocos2d::CCSprite> createSpriteName(const char* name) {
        return Build(cocos2d::CCSprite::createWithSpriteFrameName(name));
    }

    Build<cocos2d::CCSprite>& id(const char* id) { m_node->setID(id); return *this; }
    Build<cocos2d::CCSprite>& pos(cocos2d::CCPoint const& p) { m_node->setPosition(p); return *this; }
    Build<cocos2d::CCSprite>& pos(float x, float y) { m_node->setPosition({x, y}); return *this; }
    Build<cocos2d::CCSprite>& scale(float s) { m_node->setScale(s); return *this; }
    Build<cocos2d::CCSprite>& anchorPoint(float x, float y) { m_node->setAnchorPoint({x, y}); return *this; }
    Build<cocos2d::CCSprite>& zOrder(int z) { m_node->setZOrder(z); return *this; }
    Build<cocos2d::CCSprite>& parent(cocos2d::CCNode* p) { if (p) p->addChild(m_node); return *this; }
    Build<cocos2d::CCSprite>& opacity(uint8_t op) { m_node->setOpacity(op); return *this; }
    Build<cocos2d::CCSprite>& color(cocos2d::ccColor3B const& c) { m_node->setColor(c); return *this; }

    template<typename Callable>
    Build<cocos2d::CCSprite>& with(Callable cb) {
        cb(m_node);
        return *this;
    }

    template<typename C>
    Build<CCMenuItemSpriteExtra> intoMenuItem(C callback) {
        auto btn = geode::cocos::CCMenuItemExt::createSpriteExtra(m_node, callback);
        return Build<CCMenuItemSpriteExtra>(btn);
    }

    cocos2d::CCSprite* collect() { return m_node; }
    operator cocos2d::CCSprite*() { return m_node; }
};

}
