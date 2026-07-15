#pragma once

#include "ActionSerializer.hpp"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace mpedit::proto {

    // ── Opcodes (1 byte) ──────────────────────────────────────
    // Reliable channel opcodes
    enum class Opcode : uint8_t {
        // Object editing (reliable channel)
        PlaceObjects      = 0x01,
        DeleteObjects     = 0x02,
        MoveObjects       = 0x03,
        TransformObjects  = 0x04,
        UpdateObjects     = 0x05,
        LockObjects       = 0x06,
        ReconcileObjects  = 0x07,

        // Level sync (reliable channel, chunked)
        SyncLevelStart    = 0x10,
        SyncLevelChunk    = 0x11,
        SyncLevelEnd      = 0x12,

        // Settings (reliable channel)
        UpdateSettings    = 0x20,

        // Session management (reliable channel)
        PlayerJoined      = 0x30,
        PlayerLeft        = 0x31,
        RoomInfo          = 0x32,
        HostMigration     = 0x33,
        Reconnect         = 0x34,

        // Cursor (unreliable channel)
        CursorUpdate      = 0x40,

        // Batched moves during drag (unreliable channel)
        MoveBatch         = 0x41,

        // Error
        Error             = 0xFF,
    };

    // ── Writer ────────────────────────────────────────────────
    // Builds a binary message buffer. Little-endian byte order.

    class Writer {
    public:
        Writer() { m_buf.reserve(256); }
        explicit Writer(size_t reserve) { m_buf.reserve(reserve); }

        void writeU8(uint8_t v) {
            m_buf.push_back(v);
        }

        void writeU16(uint16_t v) {
            m_buf.push_back(static_cast<uint8_t>(v & 0xFF));
            m_buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        }

        void writeU32(uint32_t v) {
            m_buf.push_back(static_cast<uint8_t>(v & 0xFF));
            m_buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            m_buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
            m_buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
        }

        void writeI32(int32_t v) {
            writeU32(static_cast<uint32_t>(v));
        }

        void writeF32(float v) {
            uint32_t bits;
            std::memcpy(&bits, &v, sizeof(bits));
            writeU32(bits);
        }

        // Variable-length integer encoding (1-5 bytes for uint32_t)
        void writeVarInt(uint32_t v) {
            while (v >= 0x80) {
                m_buf.push_back(static_cast<uint8_t>(v | 0x80));
                v >>= 7;
            }
            m_buf.push_back(static_cast<uint8_t>(v));
        }

        void writeString(std::string const& s) {
            writeVarInt(static_cast<uint32_t>(s.size()));
            m_buf.insert(m_buf.end(), s.begin(), s.end());
        }

        void writeBool(bool v) {
            writeU8(v ? 1 : 0);
        }

        void writeBytes(const uint8_t* data, size_t len) {
            m_buf.insert(m_buf.end(), data, data + len);
        }

        // Write opcode as the first byte of a new message
        void writeOpcode(Opcode op) {
            writeU8(static_cast<uint8_t>(op));
        }

        std::vector<uint8_t> const& data() const { return m_buf; }
        std::vector<uint8_t>&& takeData() { return std::move(m_buf); }
        size_t size() const { return m_buf.size(); }

    private:
        std::vector<uint8_t> m_buf;
    };

    // ── Reader ────────────────────────────────────────────────
    // Reads from a binary message buffer. Little-endian byte order.

    class Reader {
    public:
        Reader(const uint8_t* data, size_t len)
            : m_data(data), m_len(len), m_pos(0) {}

        uint8_t readU8() {
            checkRemaining(1);
            return m_data[m_pos++];
        }

        uint16_t readU16() {
            checkRemaining(2);
            uint16_t v = static_cast<uint16_t>(m_data[m_pos])
                       | (static_cast<uint16_t>(m_data[m_pos + 1]) << 8);
            m_pos += 2;
            return v;
        }

        uint32_t readU32() {
            checkRemaining(4);
            uint32_t v = static_cast<uint32_t>(m_data[m_pos])
                       | (static_cast<uint32_t>(m_data[m_pos + 1]) << 8)
                       | (static_cast<uint32_t>(m_data[m_pos + 2]) << 16)
                       | (static_cast<uint32_t>(m_data[m_pos + 3]) << 24);
            m_pos += 4;
            return v;
        }

        int32_t readI32() {
            return static_cast<int32_t>(readU32());
        }

        float readF32() {
            uint32_t bits = readU32();
            float v;
            std::memcpy(&v, &bits, sizeof(v));
            return v;
        }

        uint32_t readVarInt() {
            uint32_t v = 0;
            int shift = 0;
            while (true) {
                checkRemaining(1);
                uint8_t b = m_data[m_pos++];
                v |= static_cast<uint32_t>(b & 0x7F) << shift;
                if ((b & 0x80) == 0) break;
                shift += 7;
                if (shift >= 35) throw std::runtime_error("VarInt too large");
            }
            return v;
        }

        std::string readString() {
            uint32_t len = readVarInt();
            checkRemaining(len);
            std::string s(reinterpret_cast<const char*>(m_data + m_pos), len);
            m_pos += len;
            return s;
        }

        bool readBool() {
            return readU8() != 0;
        }

        Opcode readOpcode() {
            return static_cast<Opcode>(readU8());
        }

        bool hasRemaining() const { return m_pos < m_len; }
        size_t remaining() const { return m_len - m_pos; }
        size_t position() const { return m_pos; }

        // Access raw remaining bytes (useful for chunked data)
        const uint8_t* currentPtr() const { return m_data + m_pos; }
        void skip(size_t bytes) {
            checkRemaining(bytes);
            m_pos += bytes;
        }

    private:
        void checkRemaining(size_t need) const {
            if (m_pos + need > m_len)
                throw std::runtime_error("BinaryProtocol::Reader: unexpected end of data");
        }

        const uint8_t* m_data;
        size_t m_len;
        size_t m_pos;
    };

    // ── Serialization helpers ─────────────────────────────────
    // Each returns a complete binary message with opcode prefix.

    // ObjectData write/read (shared by PlaceObjects, UpdateObjects, SyncLevelEnd)
    void writeObjectData(Writer& w, ActionSerializer::ObjectData const& obj);
    ActionSerializer::ObjectData readObjectData(Reader& r);

    // MoveData write/read
    void writeMoveData(Writer& w, ActionSerializer::MoveData const& move);
    ActionSerializer::MoveData readMoveData(Reader& r);

    // TransformData write/read
    void writeTransformData(Writer& w, ActionSerializer::TransformData const& t);
    ActionSerializer::TransformData readTransformData(Reader& r);

    // LockData write/read
    void writeLockData(Writer& w, ActionSerializer::LockData const& lock);
    ActionSerializer::LockData readLockData(Reader& r);

    // LevelSettingsData write/read
    void writeSettingsData(Writer& w, ActionSerializer::LevelSettingsData const& s);
    ActionSerializer::LevelSettingsData readSettingsData(Reader& r);

    // ── Complete message serializers ──────────────────────────

    // Objects placed: [opcode][count:varint][ObjectData...]
    std::vector<uint8_t> serializePlaceObjects(
        std::vector<ActionSerializer::ObjectData> const& objects);

    // Objects deleted: [opcode][count:varint][uuid:string...]
    std::vector<uint8_t> serializeDeleteObjects(
        std::vector<std::string> const& uuids);

    // Objects moved: [opcode][count:varint][MoveData...]
    std::vector<uint8_t> serializeMoveObjects(
        std::vector<ActionSerializer::MoveData> const& moves);

    // Objects transformed: [opcode][count:varint][TransformData...]
    std::vector<uint8_t> serializeTransformObjects(
        std::vector<ActionSerializer::TransformData> const& transforms);

    // Objects reconciled: [opcode][count:varint][ReconcileData...]
    std::vector<uint8_t> serializeReconcileObjects(
        std::vector<ActionSerializer::ReconcileData> const& reconciles);

    // Objects updated: [opcode][count:varint][ObjectData...]
    std::vector<uint8_t> serializeUpdateObjects(
        std::vector<ActionSerializer::ObjectData> const& objects);

    // Lock/unlock objects: [opcode][locked:bool][count:varint][uuid:string...]
    std::vector<uint8_t> serializeLockObjects(
        std::vector<std::string> const& uuids, bool locked);

    // Cursor update (unreliable): [opcode][x:f32][y:f32][status:string]
    std::vector<uint8_t> serializeCursorUpdate(
        float x, float y, std::string const& status);

    // Batched moves (unreliable): [opcode][count:varint][MoveData...]
    std::vector<uint8_t> serializeMoveBatch(
        std::vector<ActionSerializer::MoveData> const& moves);

    // Update settings: [opcode][SettingsData]
    std::vector<uint8_t> serializeUpdateSettings(
        ActionSerializer::LevelSettingsData const& settings);

    // Sync level (chunked) - start message:
    // [opcode][totalChunks:varint][totalObjects:varint][SettingsData]
    std::vector<uint8_t> serializeSyncLevelStart(
        uint32_t totalChunks, uint32_t totalObjects,
        ActionSerializer::LevelSettingsData const& settings);

    // Sync level - chunk:
    // [opcode][chunkIndex:varint][dataLen:varint][compressedData:bytes]
    std::vector<uint8_t> serializeSyncLevelChunk(
        uint32_t chunkIndex, const uint8_t* data, size_t dataLen,
        std::vector<std::string> const& uuids);

    // Sync level - end:
    // [opcode][lockCount:varint][LockData...]
    std::vector<uint8_t> serializeSyncLevelEnd(
        std::vector<ActionSerializer::LockData> const& locks);

    // Session messages
    std::vector<uint8_t> serializePlayerJoined(
        int playerId, std::string const& name, int colorIndex);
    std::vector<uint8_t> serializePlayerLeft(int playerId);
    std::vector<uint8_t> serializeError(std::string const& message);

    struct RoomInfoPlayer {
        int id;
        std::string name;
        int colorIndex;
    };
    struct RoomInfoMsg {
        std::vector<RoomInfoPlayer> players;
    };

    std::vector<uint8_t> serializeRoomInfo(std::vector<RoomInfoPlayer> const& players);
    RoomInfoMsg deserializeRoomInfo(Reader& r);

    // ── Deserialization ───────────────────────────────────────
    // These read from a Reader positioned AFTER the opcode byte.

    struct PlaceObjectsMsg {
        std::vector<ActionSerializer::ObjectData> objects;
    };
    PlaceObjectsMsg deserializePlaceObjects(Reader& r);

    struct DeleteObjectsMsg {
        std::vector<std::string> uuids;
    };
    DeleteObjectsMsg deserializeDeleteObjects(Reader& r);

    struct MoveObjectsMsg {
        std::vector<ActionSerializer::MoveData> moves;
    };
    MoveObjectsMsg deserializeMoveObjects(Reader& r);

    struct TransformObjectsMsg {
        std::vector<ActionSerializer::TransformData> transforms;
    };
    TransformObjectsMsg deserializeTransformObjects(Reader& r);

    struct ReconcileObjectsMsg {
        std::vector<ActionSerializer::ReconcileData> reconciles;
    };
    ReconcileObjectsMsg deserializeReconcileObjects(Reader& r);

    struct UpdateObjectsMsg {
        std::vector<ActionSerializer::ObjectData> objects;
    };
    UpdateObjectsMsg deserializeUpdateObjects(Reader& r);

    struct LockObjectsMsg {
        bool locked;
        std::vector<std::string> uuids;
    };
    LockObjectsMsg deserializeLockObjects(Reader& r);

    struct CursorUpdateMsg {
        float x, y;
        std::string status;
    };
    CursorUpdateMsg deserializeCursorUpdate(Reader& r);

    struct MoveBatchMsg {
        std::vector<ActionSerializer::MoveData> moves;
    };
    MoveBatchMsg deserializeMoveBatch(Reader& r);

    struct SyncLevelStartMsg {
        uint32_t totalChunks;
        uint32_t totalObjects;
        ActionSerializer::LevelSettingsData settings;
    };
    SyncLevelStartMsg deserializeSyncLevelStart(Reader& r);

    struct SyncLevelChunkMsg {
        uint32_t chunkIndex;
        std::vector<uint8_t> data;  // raw or compressed object data
        std::vector<std::string> uuids;
    };
    SyncLevelChunkMsg deserializeSyncLevelChunk(Reader& r);

    struct SyncLevelEndMsg {
        std::vector<ActionSerializer::LockData> locks;
    };
    SyncLevelEndMsg deserializeSyncLevelEnd(Reader& r);

    struct PlayerJoinedMsg {
        int playerId;
        std::string name;
        int colorIndex;
    };
    PlayerJoinedMsg deserializePlayerJoined(Reader& r);

    struct PlayerLeftMsg {
        int playerId;
    };
    PlayerLeftMsg deserializePlayerLeft(Reader& r);

    struct ErrorMsg {
        std::string message;
    };
    ErrorMsg deserializeError(Reader& r);

    struct UpdateSettingsMsg {
        ActionSerializer::LevelSettingsData settings;
    };
    UpdateSettingsMsg deserializeUpdateSettings(Reader& r);

} // namespace mpedit::proto
