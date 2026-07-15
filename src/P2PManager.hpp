#pragma once

#include "BinaryProtocol.hpp"
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <vector>
#include <memory>
#include <atomic>

// Forward-declare libdatachannel types to avoid header pollution
namespace rtc {
    class PeerConnection;
    class DataChannel;
    struct Configuration;
}

namespace mpedit {

    enum class ChannelType {
        Reliable,    // ordered, reliable — edits, sync, locks
        Unreliable   // unordered, maxRetransmits=0 — cursors, move batches
    };

    /**
     * Manages peer-to-peer connections via WebRTC data channels.
     *
     * Star topology: Host connects to all clients. Clients connect only to host.
     * Host relays messages between clients.
     *
     * Replaces NetworkManager entirely — no central server needed.
     * Only a lightweight signaling server is used for connection setup.
     */
    class P2PManager {
    public:
        enum class State {
            Disconnected,
            Connecting,     // signaling / ICE negotiation in progress
            Connected,      // at least one peer connected (host) or connected to host (client)
            Reconnecting,   // lost connection, trying to re-establish
            Error
        };

        enum class Role {
            None,
            Host,
            Client
        };

        using MessageCallback = std::function<void(int playerId, proto::Reader& reader)>;

        static P2PManager& get();

        // ── Session lifecycle ─────────────────────────────────
        void hostSession(std::string const& playerName);
        void joinSession(std::string const& roomCode, std::string const& playerName);
        void leaveSession();

        // ── State queries ─────────────────────────────────────
        State getState() const;
        Role getRole() const;
        bool isConnected() const;
        std::string getRoomCode() const;
        int getLocalPlayerId() const;
        std::string getError() const;

        // ── Sending ───────────────────────────────────────────

        // Send to host (client) or broadcast to all clients (host)
        void send(std::vector<uint8_t> const& data, ChannelType channel = ChannelType::Reliable);
        void send(std::vector<uint8_t>&& data, ChannelType channel = ChannelType::Reliable);

        // Send to a specific peer (host only)
        void sendTo(int playerId, std::vector<uint8_t> const& data, ChannelType channel = ChannelType::Reliable);

        // Broadcast to all peers except one (host only, used for relaying)
        void broadcast(std::vector<uint8_t> const& data, ChannelType channel = ChannelType::Reliable, int excludePlayerId = -1);

        // ── Message handling ──────────────────────────────────

        // Register a handler for a binary opcode. Called on main thread via dispatchMessages().
        void on(proto::Opcode opcode, MessageCallback callback);
        void clearHandlers();

        // Must be called on the main/game thread (e.g. from networkUpdate) to
        // drain the incoming queue and invoke handlers.
        void dispatchMessages();

        // ── Session event callbacks ───────────────────────────
        using SessionStartedCb = std::function<void(std::string const& roomCode, int localPlayerId)>;
        using PeerConnectedCb  = std::function<void(int playerId, std::string const& name, int colorIndex)>;
        using PeerDisconnectedCb = std::function<void(int playerId)>;
        using ErrorCb = std::function<void(std::string const& error)>;

        void onSessionStarted(SessionStartedCb cb);
        void onPeerConnected(PeerConnectedCb cb);
        void onPeerDisconnected(PeerDisconnectedCb cb);
        void onError(ErrorCb cb);
        void clearCallbacks();

        // ── Signaling URL ─────────────────────────────────────
        static std::string getSignalingUrl();

    private:
        P2PManager();
        ~P2PManager();

        P2PManager(P2PManager const&) = delete;
        P2PManager& operator=(P2PManager const&) = delete;

        // ── ICE / WebRTC ──────────────────────────────────────
        struct PendingMessage {
            std::vector<uint8_t> data;
            ChannelType channel;
        };

        struct PeerInfo {
            std::shared_ptr<rtc::PeerConnection> pc;
            std::shared_ptr<rtc::DataChannel> reliable;
            std::shared_ptr<rtc::DataChannel> unreliable;
            int playerId = -1;
            std::string playerName;
            int colorIndex = 0;
            bool ready = false; // both channels open
            std::vector<PendingMessage> pendingMessages;
        };

        rtc::Configuration makeRtcConfig();
        void createHostPeer(int clientPlayerId, std::string const& clientName);

        // Signaling HTTP helpers (use Geode web::WebRequest)
        void signalingCreateRoom(std::string const& playerName);
        void signalingPollForClients();
        void signalingJoinRoom(std::string const& roomCode, std::string const& playerName);
        void signalingPollForAnswer();
        void pollClientAnswer(int clientId);

        // Called on data channel threads — enqueues for main-thread dispatch
        void onPeerMessage(int fromPlayerId, const uint8_t* data, size_t len);
        void onPeerDisconnected(int playerId, bool unexpected);

        // Host relay: forward a message from one client to all others
        void relayMessage(int fromPlayerId, const uint8_t* data, size_t len, ChannelType channel);
        void checkPeerReady(int playerId);

        // ── State ─────────────────────────────────────────────
        std::atomic<State> m_state{State::Disconnected};
        Role m_role = Role::None;
        std::string m_roomCode;
        int m_localPlayerId = -1;
        std::string m_localPlayerName;
        std::string m_error;
        mutable std::mutex m_stateMutex;

        // ── Peers ─────────────────────────────────────────────
        std::unordered_map<int, PeerInfo> m_peers;
        std::mutex m_peersMutex;
        int m_nextPlayerId = 1; // host assigns IDs (host = 0)

        // ── Incoming message queue ────────────────────────────
        struct QueuedMessage {
            int fromPlayerId;
            std::vector<uint8_t> data;
        };
        std::queue<QueuedMessage> m_incoming;
        std::mutex m_incomingMutex;
        bool m_dispatching = false;

        // ── Handlers ──────────────────────────────────────────
        std::unordered_map<uint8_t, std::vector<MessageCallback>> m_handlers;

        // ── Callbacks ─────────────────────────────────────────
        std::vector<SessionStartedCb> m_onSessionStarted;
        std::vector<PeerConnectedCb> m_onPeerConnected;
        std::vector<PeerDisconnectedCb> m_onPeerDisconnected;
        std::vector<ErrorCb> m_onError;

        // ── Signaling polling ─────────────────────────────────
        bool m_pollingSignaling = false;
        std::string m_pendingLocalSdp;   // our SDP offer/answer
        std::string m_signalingRoomId;   // server-side room ID for polling
        geode::async::TaskHolder<geode::utils::web::WebResponse> m_signalingListener;
        geode::async::TaskHolder<geode::utils::web::WebResponse> m_pollingListener;
        std::unordered_map<int, geode::async::TaskHolder<geode::utils::web::WebResponse>> m_answerListeners;

        // ── Host migration ────────────────────────────────────
        // (Phase 4 — placeholder for now)
    };

} // namespace mpedit
