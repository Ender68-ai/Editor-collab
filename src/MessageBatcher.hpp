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

        // Queue a move delta for batching. Accumulated per-uuid.
        void queueMove(std::string const& uuid, float dx, float dy);

        // Queue a transform for batching. Latest transform per-uuid wins.
        void queueTransform(std::string const& uuid,
            ActionSerializer::TransformData const& transform);

        // Called every frame from networkUpdate(). Flushes when interval elapsed.
        void update(float dt);

        // Force-flush all pending batches immediately (e.g., on mouse release).
        void flush();

        // Clear without sending (e.g., on disconnect).
        void clear();

        // Remove any pending moves or transforms for a specific UUID (e.g., when placing a new object)
        void removePending(std::string const& uuid);

        // Flush interval in seconds (default 0.05 = 20 Hz)
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
