#include "MessageBatcher.hpp"
#include "P2PManager.hpp"
#include "BinaryProtocol.hpp"
#include <Geode/loader/Log.hpp>

namespace mpedit {

    MessageBatcher& MessageBatcher::get() {
        static MessageBatcher instance;
        return instance;
    }

    void MessageBatcher::queueMove(std::string const& uuid, float dx, float dy) {
        auto& accum = m_pendingMoves[uuid];
        accum.dx += dx;
        accum.dy += dy;
    }

    void MessageBatcher::queueTransform(std::string const& uuid,
        ActionSerializer::TransformData const& transform)
    {

        m_pendingTransforms[uuid] = transform;
    }

    void MessageBatcher::update(float dt) {
        m_moveTimer += dt;
        m_transformTimer += dt;

        if (m_moveTimer >= m_flushInterval && !m_pendingMoves.empty()) {

            std::vector<ActionSerializer::MoveData> moves;
            moves.reserve(m_pendingMoves.size());
            for (auto& [uuid, accum] : m_pendingMoves) {

                if (accum.dx == 0.f && accum.dy == 0.f) continue;
                moves.push_back({uuid, accum.dx, accum.dy});
            }
            m_pendingMoves.clear();
            m_moveTimer = 0.f;

            if (!moves.empty()) {
                auto data = proto::serializeMoveBatch(moves);

                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }

        if (m_transformTimer >= m_flushInterval && !m_pendingTransforms.empty()) {
            std::vector<ActionSerializer::TransformData> transforms;
            transforms.reserve(m_pendingTransforms.size());
            for (auto& [uuid, t] : m_pendingTransforms) {
                transforms.push_back(t);
            }
            m_pendingTransforms.clear();
            m_transformTimer = 0.f;

            if (!transforms.empty()) {
                auto data = proto::serializeTransformObjects(transforms);
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }
    }

    void MessageBatcher::flush() {

        if (!m_pendingMoves.empty()) {
            std::vector<ActionSerializer::MoveData> moves;
            moves.reserve(m_pendingMoves.size());
            for (auto& [uuid, accum] : m_pendingMoves) {
                if (accum.dx == 0.f && accum.dy == 0.f) continue;
                moves.push_back({uuid, accum.dx, accum.dy});
            }
            m_pendingMoves.clear();
            m_moveTimer = 0.f;

            if (!moves.empty()) {

                auto data = proto::serializeMoveBatch(moves);
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }

        if (!m_pendingTransforms.empty()) {
            std::vector<ActionSerializer::TransformData> transforms;
            transforms.reserve(m_pendingTransforms.size());
            for (auto& [uuid, t] : m_pendingTransforms) {
                transforms.push_back(t);
            }
            m_pendingTransforms.clear();
            m_transformTimer = 0.f;

            if (!transforms.empty()) {
                auto data = proto::serializeTransformObjects(transforms);
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }
    }

    void MessageBatcher::clear() {
        m_pendingMoves.clear();
        m_pendingTransforms.clear();
        m_moveTimer = 0.f;
        m_transformTimer = 0.f;
    }

    void MessageBatcher::removePending(std::string const& uuid) {
        m_pendingMoves.erase(uuid);
        m_pendingTransforms.erase(uuid);
    }

} // namespace mpedit
