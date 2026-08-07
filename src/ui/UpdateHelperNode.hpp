#pragma once

#include <Geode/Geode.hpp>
#include <functional>

namespace mpedit {

    class UpdateHelperNode : public cocos2d::CCNode {
    public:
        using UpdateCallback = std::function<void(float)>;

        static UpdateHelperNode* create(UpdateCallback callback, float interval) {
            auto* ret = new UpdateHelperNode();
            if (ret && ret->init(std::move(callback), interval)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        bool init(UpdateCallback callback, float interval) {
            if (!CCNode::init()) return false;
            m_callback = std::move(callback);
            this->schedule(schedule_selector(UpdateHelperNode::onUpdate), interval);
            return true;
        }

        void onUpdate(float dt) {
            if (m_callback) {
                m_callback(dt);
            }
        }

    private:
        UpdateCallback m_callback;
    };

} // namespace mpedit
