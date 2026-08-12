#pragma once

#include "BinaryProtocol.hpp"
#include "P2PManager.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mpedit {

    /**
     * Coalesces high-frequency messages (moves, transforms) into periodic
     * batched sends.
     */
    class MessageBatcher {
    public:
        static MessageBatcher& get();

        void queueMove(std::string const& uuid, float dx, float dy);

        void queueTransform(std::string const& uuid,
            ActionSerializer::TransformData const& transform);

        void update(float dt);

        void flush();

        void clear();

        void removePending(std::string const& uuid);

        bool hasPending(std::string const& uuid) const;

        void flushMoves();
        void flushTransforms();

        void setFlushInterval(float interval) { m_flushInterval = interval; }
        float getFlushInterval() const { return m_flushInterval; }

    private:
        MessageBatcher() = default;

        struct MoveAccum {
            float dx = 0.f;
            float dy = 0.f;
        };

        std::unordered_map<std::string, MoveAccum> m_pendingMoves;
        std::unordered_map<std::string, ActionSerializer::TransformData> m_pendingTransforms;

        float m_flushInterval = 0.05f;
        float m_moveTimer = 0.f;
        float m_transformTimer = 0.f;
    };

}
