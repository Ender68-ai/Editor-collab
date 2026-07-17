#pragma once

#include "BinaryProtocol.hpp"
#include "P2PManager.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mpedit {

    /**
     * Coalesces high-frequency messages (moves, transforms) into periodic
     * batched sends. Instead of sending per-pixel-per-object during drag
     * (which can be 3000+ msg/sec), this batches moves into 20 Hz flushes.
     *
     * Usage:
     *   - EditorHooks calls queueMove() instead of sending immediately
     *   - networkUpdate() calls update(dt) each tick to auto-flush
     *   - On deselect/release, call flush() to send any remaining deltas
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

        float m_flushInterval = 0.05f; // 20 Hz
        float m_moveTimer = 0.f;
        float m_transformTimer = 0.f;
    };

} // namespace mpedit
