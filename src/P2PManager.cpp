#include "P2PManager.hpp"
#include "BinaryProtocol.hpp"

#include <rtc/rtc.hpp>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <thread>
#include <chrono>

using namespace geode::prelude;

namespace mpedit {

    // ── Singleton ─────────────────────────────────────────────

    P2PManager& P2PManager::get() {
        static P2PManager instance;
        return instance;
    }

    P2PManager::P2PManager() {
        rtc::InitLogger(rtc::LogLevel::Warning);
    }

    P2PManager::~P2PManager() {
        leaveSession();
    }

    // ── ICE Configuration ─────────────────────────────────────

    rtc::Configuration P2PManager::makeRtcConfig() {
        rtc::Configuration config;
        // Free STUN servers for NAT traversal (~85% of connections)
        config.iceServers.push_back({"stun:stun.l.google.com:19302"});
        config.iceServers.push_back({"stun:stun.cloudflare.com:3478"});
        // Free TURN relay for symmetric NATs (~5-10% of connections)
        rtc::IceServer turn("openrelay.metered.ca", 443, "openrelayproject", "openrelayproject", rtc::IceServer::RelayType::TurnTcp);
        config.iceServers.push_back(turn);
        return config;
    }

    std::string P2PManager::getSignalingUrl() {
        auto url = Mod::get()->getSettingValue<std::string>("signaling-url");
        if (url.empty()) return "https://dewy-flea-9364.d050.deno.net";
        return url;
    }

    // ── State Accessors ───────────────────────────────────────

    P2PManager::State P2PManager::getState() const {
        return m_state.load();
    }

    P2PManager::Role P2PManager::getRole() const {
        std::lock_guard lock(m_stateMutex);
        return m_role;
    }

    bool P2PManager::isConnected() const {
        return m_state.load() == State::Connected;
    }

    std::string P2PManager::getRoomCode() const {
        std::lock_guard lock(m_stateMutex);
        return m_roomCode;
    }

    int P2PManager::getLocalPlayerId() const {
        return m_localPlayerId;
    }

    std::string P2PManager::getError() const {
        std::lock_guard lock(m_stateMutex);
        return m_error;
    }

    // ── Callback Registration ─────────────────────────────────

    void P2PManager::onSessionStarted(SessionStartedCb cb) {
        m_onSessionStarted.push_back(std::move(cb));
    }
    void P2PManager::onPeerConnected(PeerConnectedCb cb) {
        m_onPeerConnected.push_back(std::move(cb));
    }
    void P2PManager::onPeerDisconnected(PeerDisconnectedCb cb) {
        m_onPeerDisconnected.push_back(std::move(cb));
    }
    void P2PManager::onError(ErrorCb cb) {
        m_onError.push_back(std::move(cb));
    }
    void P2PManager::clearCallbacks() {
        m_onSessionStarted.clear();
        m_onPeerConnected.clear();
        m_onPeerDisconnected.clear();
        m_onError.clear();
    }

    // ── Handler Registration ──────────────────────────────────

    void P2PManager::on(proto::Opcode opcode, MessageCallback callback) {
        m_handlers[static_cast<uint8_t>(opcode)].push_back(std::move(callback));
    }

    void P2PManager::clearHandlers() {
        m_handlers.clear();
    }

    // ── Message Dispatch (main thread) ────────────────────────

    void P2PManager::dispatchMessages() {
        if (m_dispatching) return;
        m_dispatching = true;

        std::queue<QueuedMessage> messages;
        {
            std::lock_guard lock(m_incomingMutex);
            std::swap(messages, m_incoming);
        }

        while (!messages.empty()) {
            auto& msg = messages.front();

            if (!msg.data.empty()) {
                try {
                    uint8_t opcodeRaw = msg.data[0];
                    proto::Reader reader(msg.data.data() + 1, msg.data.size() - 1);

                    auto it = m_handlers.find(opcodeRaw);
                    if (it != m_handlers.end()) {
                        // Copy handlers to allow modification during dispatch
                        auto handlersCopy = it->second;
                        for (auto const& handler : handlersCopy) {
                            // Reset reader position for each handler
                            proto::Reader handlerReader(msg.data.data() + 1, msg.data.size() - 1);
                            handler(msg.fromPlayerId, handlerReader);
                            if (m_handlers.empty()) break;
                        }
                    }
                } catch (std::exception const& e) {
                    log::error("P2PManager: Error dispatching message: {}", e.what());
                }
            }

            if (m_handlers.empty()) break;
            messages.pop();
        }

        m_dispatching = false;
    }

    // ── Sending ───────────────────────────────────────────────

    void P2PManager::send(std::vector<uint8_t> const& data, ChannelType channel) {
        if (m_role == Role::Host) {
            broadcast(data, channel);
        } else if (m_role == Role::Client) {
            // Client sends only to host (playerId 0)
            sendTo(0, data, channel);
        }
    }

    void P2PManager::send(std::vector<uint8_t>&& data, ChannelType channel) {
        send(static_cast<std::vector<uint8_t> const&>(data), channel);
    }

    void P2PManager::sendTo(int playerId, std::vector<uint8_t> const& data, ChannelType channel) {
        std::lock_guard lock(m_peersMutex);
        auto it = m_peers.find(playerId);
        if (it == m_peers.end() || !it->second.ready) return;

        auto& peer = it->second;
        auto& dc = (channel == ChannelType::Reliable) ? peer.reliable : peer.unreliable;

        if (dc && dc->isOpen()) {
            try {
                dc->send(reinterpret_cast<const std::byte*>(data.data()), data.size());
            } catch (std::exception const& e) {
                log::error("P2PManager: Send to player {} failed: {}", playerId, e.what());
            }
        }
    }

    void P2PManager::broadcast(std::vector<uint8_t> const& data, ChannelType channel, int excludePlayerId) {
        std::vector<int> peerIds;
        {
            std::lock_guard lock(m_peersMutex);
            for (auto& [id, peer] : m_peers) {
                if (id != excludePlayerId && peer.ready) {
                    peerIds.push_back(id);
                }
            }
        }

        for (int id : peerIds) {
            sendTo(id, data, channel);
        }
    }

    // ── Peer Message Handling ─────────────────────────────────

    void P2PManager::onPeerMessage(int fromPlayerId, const uint8_t* data, size_t len) {
        if (len == 0) return;

        // Enqueue for local main-thread dispatch
        {
            std::lock_guard lock(m_incomingMutex);
            m_incoming.push(QueuedMessage{
                fromPlayerId,
                std::vector<uint8_t>(data, data + len)
            });
        }

        // Host relays to all other clients
        if (m_role == Role::Host) {
            // Determine channel from opcode for relay
            uint8_t opcode = data[0];
            ChannelType ch = ChannelType::Reliable;
            if (opcode == static_cast<uint8_t>(proto::Opcode::CursorUpdate)) {
                ch = ChannelType::Unreliable;
            }
            relayMessage(fromPlayerId, data, len, ch);
        }
    }

    void P2PManager::relayMessage(int fromPlayerId, const uint8_t* data, size_t len, ChannelType channel) {
        std::vector<uint8_t> relayData(data, data + len);
        broadcast(relayData, channel, fromPlayerId);
    }

    void P2PManager::onPeerDisconnected(int playerId, bool unexpected) {
        {
            std::lock_guard lock(m_peersMutex);
            auto it = m_peers.find(playerId);
            if (it != m_peers.end()) {
                if (it->second.pc) it->second.pc->close();
                m_peers.erase(it);
            }
        }

        log::info("P2PManager: Player {} disconnected (unexpected={})", playerId, unexpected);

        queueInMainThread([this, playerId, unexpected]() {
            // If we're a client and the host disconnected, session is over
            if (m_role == Role::Client && playerId == 0) {
                for (auto& cb : m_onError) {
                    cb("Host disconnected");
                }
                return;
            }

            // Notify callbacks
            for (auto& cb : m_onPeerDisconnected) {
                cb(playerId);
            }

            // Host notifies remaining clients about the departure
            if (m_role == Role::Host) {
                auto msg = proto::serializePlayerLeft(playerId);
                broadcast(msg, ChannelType::Reliable);
            }
        });
    }

    // ── Host Session ──────────────────────────────────────────

    void P2PManager::hostSession(std::string const& playerName) {
        {
            std::lock_guard lock(m_stateMutex);
            m_role = Role::Host;
            m_localPlayerId = 0;
            m_localPlayerName = playerName;
            m_error.clear();
        }
        m_state.store(State::Connecting);
        m_nextPlayerId = 1;

        signalingCreateRoom(playerName);
    }

    void P2PManager::signalingCreateRoom(std::string const& playerName) {
        auto url = getSignalingUrl() + "/rooms";
        log::info("P2PManager: Creating room on signaling server: {}", url);

        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        auto body = matjson::Value();
        body["playerName"] = playerName;
        req.bodyJSON(body);

        m_signalingListener.spawn(
            req.post(url),
            [this](web::WebResponse res) {
                if (res.ok()) {
                    auto json = res.json().unwrapOr(matjson::Value());
                    auto roomCode = json.get<std::string>("roomCode").unwrapOr("");
                    m_signalingRoomId = json.get<std::string>("roomId").unwrapOr("");

                     if (roomCode.empty()) {
                        std::vector<ErrorCb> callbacks;
                        std::string err;
                        {
                            std::lock_guard lock(m_stateMutex);
                            m_error = "Failed to create room: no room code";
                            m_state.store(State::Error);
                            callbacks = m_onError;
                            err = m_error;
                        }
                        for (auto& cb : callbacks) cb(err);
                        return;
                    }

                    {
                        std::lock_guard lock(m_stateMutex);
                        m_roomCode = roomCode;
                    }
                    m_state.store(State::Connected);

                    log::info("P2PManager: Room created with code: {}", roomCode);

                    for (auto& cb : m_onSessionStarted) {
                        cb(roomCode, 0);
                    }

                    // Start polling for new clients
                    m_pollingSignaling = true;
                    signalingPollForClients();
                } else {
                    std::vector<ErrorCb> callbacks;
                    std::string err;
                    {
                        std::lock_guard lock(m_stateMutex);
                        m_error = "Signaling server error: " + std::to_string(res.code());
                        m_state.store(State::Error);
                        callbacks = m_onError;
                        err = m_error;
                    }
                    for (auto& cb : callbacks) cb(err);
                }
            }
        );
    }

    void P2PManager::signalingPollForClients() {
        if (!m_pollingSignaling || m_role != Role::Host ||
            m_state.load() != State::Connected) return;

        auto code = getRoomCode();
        auto url = getSignalingUrl() + "/rooms/" + code;

        auto req = web::WebRequest();
        m_pollingListener.spawn(
            req.get(url),
            [this](web::WebResponse res) {
                if (res.ok()) {
                    auto json = res.json().unwrapOr(matjson::Value());
                    int playerCount = json.get<int>("playerCount").unwrapOr(0);

                    while (m_nextPlayerId < playerCount) {
                        int clientId = m_nextPlayerId++;
                        std::string clientName = "Player " + std::to_string(clientId);
                        log::info("P2PManager: Client {} connecting", clientId);
                        createHostPeer(clientId, clientName);
                        pollClientAnswer(clientId);
                    }
                }
                if (m_pollingSignaling) {
                    std::thread([this]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                        queueInMainThread([this]() { signalingPollForClients(); });
                    }).detach();
                }
            }
        );
    }

    void P2PManager::pollClientAnswer(int clientId) {
        if (!m_pollingSignaling || m_role != Role::Host) return;

        auto code = getRoomCode();
        auto url = getSignalingUrl() + "/rooms/" + code + "/answer?playerId=" + std::to_string(clientId);

        auto req = web::WebRequest();
        m_answerListeners[clientId].spawn(
            req.get(url),
            [this, clientId](web::WebResponse res) {
                if (res.ok()) {
                    auto json = res.json().unwrapOr(matjson::Value());
                    auto sdp = json.get<std::string>("sdp").unwrapOr("");
                    if (!sdp.empty()) {
                        log::info("P2PManager: Received SDP answer from client {}", clientId);
                        
                        // Force answer role to active to fix libdatachannel crash
                        size_t setupPos = sdp.find("a=setup:actpass");
                        while (setupPos != std::string::npos) {
                            sdp.replace(setupPos, 15, "a=setup:active");
                            setupPos = sdp.find("a=setup:actpass", setupPos);
                        }
                        
                        log::info("Answer SDP:\n{}", sdp);
                        std::lock_guard lock(m_peersMutex);
                        auto it = m_peers.find(clientId);
                        if (it != m_peers.end() && it->second.pc) {
                            it->second.pc->setRemoteDescription(
                                rtc::Description(sdp, rtc::Description::Type::Answer, rtc::Description::Role::Active));
                        }
                        return;
                    }
                }
                if (m_pollingSignaling) {
                    std::thread([this, clientId]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        queueInMainThread([this, clientId]() { pollClientAnswer(clientId); });
                    }).detach();
                }
            }
        );
    }

    // ── Join Session ──────────────────────────────────────────

    void P2PManager::joinSession(std::string const& roomCode, std::string const& playerName) {
        {
            std::lock_guard lock(m_stateMutex);
            m_role = Role::Client;
            m_roomCode = roomCode;
            m_localPlayerName = playerName;
            m_error.clear();
        }
        m_state.store(State::Connecting);

        signalingJoinRoom(roomCode, playerName);
    }

    void P2PManager::signalingJoinRoom(std::string const& roomCode, std::string const& playerName) {
        auto url = getSignalingUrl() + "/rooms/" + roomCode + "/join";
        log::info("P2PManager: Joining room {} on signaling server", roomCode);

        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        auto body = matjson::Value();
        body["playerName"] = playerName;
        req.bodyJSON(body);

        m_signalingListener.spawn(
            req.post(url),
            [this, roomCode, playerName](web::WebResponse res) {
                if (res.ok()) {
                    auto json = res.json().unwrapOr(matjson::Value());
                    m_localPlayerId = json.get<int>("playerId").unwrapOr(-1);
                    auto hostName = json.get<std::string>("hostName").unwrapOr("Host");

                    if (m_localPlayerId < 0) {
                        std::vector<ErrorCb> callbacks;
                        std::string err;
                        {
                            std::lock_guard lock(m_stateMutex);
                            m_error = "Failed to join room";
                            m_state.store(State::Error);
                            callbacks = m_onError;
                            err = m_error;
                        }
                        for (auto& cb : callbacks) cb(err);
                        return;
                    }

                    log::info("P2PManager: Joined room {} as player {}", roomCode, m_localPlayerId);

                    // Create peer connection to host
                    auto pc = std::make_shared<rtc::PeerConnection>(makeRtcConfig());

                    PeerInfo hostPeer;
                    hostPeer.pc = pc;
                    hostPeer.playerId = 0;
                    hostPeer.playerName = hostName;
                    hostPeer.colorIndex = 0;

                    int myId = m_localPlayerId;

                    // Client receives data channels from host
                    pc->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc) {
                        bool isReliable = dc->label() == "reliable";
                        log::info("P2PManager: Received {} data channel", dc->label());
                        
                        {
                            std::lock_guard lock(m_peersMutex);
                            auto it = m_peers.find(0);
                            if (it != m_peers.end()) {
                                if (isReliable) it->second.reliable = dc;
                                else it->second.unreliable = dc;
                            }
                        }

                        dc->onOpen([this, isReliable]() {
                            log::info("P2PManager: {} channel to host opened", isReliable ? "Reliable" : "Unreliable");
                            checkPeerReady(0);
                        });

                        dc->onMessage([this](auto data) {
                            if (auto* binaryMsg = std::get_if<rtc::binary>(&data)) {
                                onPeerMessage(0, reinterpret_cast<const uint8_t*>(binaryMsg->data()), binaryMsg->size());
                            }
                        });

                        dc->onClosed([this]() { log::info("P2PManager: Channel to host closed"); });
                    });

                    // Handle ICE gathering completion — post SDP to signaling
                    pc->onGatheringStateChange([this, pc, myId, roomCode](
                        rtc::PeerConnection::GatheringState state)
                    {
                        if (state == rtc::PeerConnection::GatheringState::Complete) {
                            auto desc = pc->localDescription();
                            if (desc.has_value()) {
                                std::string sdp = std::string(desc.value());
                                
                                // Force answer role to active to fix libdatachannel crash
                                size_t setupPos = sdp.find("a=setup:actpass");
                                while (setupPos != std::string::npos) {
                                    sdp.replace(setupPos, 15, "a=setup:active");
                                    setupPos = sdp.find("a=setup:actpass", setupPos);
                                }
                                
                                log::info("P2PManager: ICE gathering complete, posting SDP answer");

                                queueInMainThread([this, sdp, myId, roomCode]() {
                                    auto url = getSignalingUrl() + "/rooms/" + roomCode + "/answer";
                                    auto req = web::WebRequest();
                                    req.header("Content-Type", "application/json");
                                    auto body = matjson::Value();
                                    body["sdp"] = sdp;
                                    body["playerId"] = myId;
                                    req.bodyJSON(body);
                                    // Fire-and-forget
                                    async::spawn(req.post(url));
                                });
                            }
                        }
                    });

                    // Handle connection state changes
                    pc->onStateChange([this](rtc::PeerConnection::State state) {
                        if (state == rtc::PeerConnection::State::Disconnected ||
                            state == rtc::PeerConnection::State::Failed ||
                            state == rtc::PeerConnection::State::Closed) {
                            queueInMainThread([this]() {
                                onPeerDisconnected(0, true);
                            });
                        }
                    });

                    {
                        std::lock_guard lock(m_peersMutex);
                        m_peers[0] = std::move(hostPeer);
                    }

                    // Now poll for the host's SDP offer
                    signalingPollForAnswer();

                } else if (res.code() == 404) {
                    std::vector<ErrorCb> callbacks;
                    std::string err;
                    {
                        std::lock_guard lock(m_stateMutex);
                        m_error = "Room not found";
                        m_state.store(State::Error);
                        callbacks = m_onError;
                        err = m_error;
                    }
                    for (auto& cb : callbacks) cb(err);
                } else {
                    std::vector<ErrorCb> callbacks;
                    std::string err;
                    {
                        std::lock_guard lock(m_stateMutex);
                        m_error = "Failed to join room: " + std::to_string(res.code());
                        m_state.store(State::Error);
                        callbacks = m_onError;
                        err = m_error;
                    }
                    for (auto& cb : callbacks) cb(err);
                }
            }
        );
    }

    void P2PManager::signalingPollForAnswer() {
        auto code = getRoomCode();
        auto url = getSignalingUrl() + "/rooms/" + code + "/offer?playerId=" + std::to_string(m_localPlayerId);

        auto req = web::WebRequest();
        m_pollingListener.spawn(
            req.get(url),
            [this](web::WebResponse res) {
                if (res.ok()) {
                    auto json = res.json().unwrapOr(matjson::Value());
                    auto sdp = json.get<std::string>("sdp").unwrapOr("");

                    if (!sdp.empty()) {
                        log::info("P2PManager: Received host's SDP offer");
                        log::info("Offer SDP:\n{}", sdp);
                        std::lock_guard lock(m_peersMutex);
                        auto it = m_peers.find(0);
                        if (it != m_peers.end() && it->second.pc) {
                            it->second.pc->setRemoteDescription(
                                rtc::Description(sdp, rtc::Description::Type::Offer, rtc::Description::Role::ActPass));
                            it->second.pc->setLocalDescription();
                        }
                        return;
                    }
                }
                // Not ready yet, retry in 1 second
                if (m_state.load() == State::Connecting) {
                    std::thread([this]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        queueInMainThread([this]() {
                            signalingPollForAnswer();
                        });
                    }).detach();
                }
            }
        );
    }

    // ── Host: Create Peer Connection for Client ───────────────

    void P2PManager::createHostPeer(int clientPlayerId, std::string const& clientName) {
        auto pc = std::make_shared<rtc::PeerConnection>(makeRtcConfig());

        // Create both data channels
        auto reliable = pc->createDataChannel("reliable");

        rtc::DataChannelInit unreliableInit;
        unreliableInit.reliability.maxRetransmits = 0;
        auto unreliable = pc->createDataChannel("unreliable", unreliableInit);

        PeerInfo peer;
        peer.pc = pc;
        peer.reliable = reliable;
        peer.unreliable = unreliable;
        peer.playerId = clientPlayerId;
        peer.playerName = clientName;
        peer.colorIndex = clientPlayerId % 6;

        // Wire up data channel callbacks
        auto setupChannelCallbacks = [this, clientPlayerId](std::shared_ptr<rtc::DataChannel> dc, bool isReliable) {
            dc->onOpen([this, clientPlayerId, isReliable]() {
                log::info("P2PManager: {} channel to player {} opened",
                    isReliable ? "Reliable" : "Unreliable", clientPlayerId);
                checkPeerReady(clientPlayerId);
            });

            dc->onMessage([this, clientPlayerId](auto data) {
                if (auto* binaryMsg = std::get_if<rtc::binary>(&data)) {
                    onPeerMessage(clientPlayerId,
                        reinterpret_cast<const uint8_t*>(binaryMsg->data()),
                        binaryMsg->size());
                }
            });

            dc->onClosed([this, clientPlayerId]() {
                log::info("P2PManager: Channel to player {} closed", clientPlayerId);
            });
        };

        setupChannelCallbacks(reliable, true);
        setupChannelCallbacks(unreliable, false);

        // Post SDP offer to signaling server when ICE gathering completes
        pc->onGatheringStateChange([this, pc, clientPlayerId](
            rtc::PeerConnection::GatheringState state)
        {
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto desc = pc->localDescription();
                if (desc.has_value()) {
                    std::string sdp = std::string(desc.value());
                    auto code = getRoomCode();
                    log::info("P2PManager: Posting SDP offer for player {}", clientPlayerId);

                    queueInMainThread([this, sdp, clientPlayerId, code]() {
                        auto url = getSignalingUrl() + "/rooms/" + code + "/offer";
                        auto req = web::WebRequest();
                        req.header("Content-Type", "application/json");
                        auto body = matjson::Value();
                        body["sdp"] = sdp;
                        body["targetPlayerId"] = clientPlayerId;
                        req.bodyJSON(body);
                        // Use a separate listener for this fire-and-forget
                        async::spawn(req.post(url));
                    });
                }
            }
        });

        pc->onStateChange([this, clientPlayerId](rtc::PeerConnection::State state) {
            if (state == rtc::PeerConnection::State::Disconnected ||
                state == rtc::PeerConnection::State::Failed ||
                state == rtc::PeerConnection::State::Closed) {
                queueInMainThread([this, clientPlayerId]() {
                    onPeerDisconnected(clientPlayerId, true);
                });
            }
        });

        // Generate the offer
        pc->setLocalDescription();

        {
            std::lock_guard lock(m_peersMutex);
            m_peers[clientPlayerId] = std::move(peer);
        }
    }

    // ── Peer Ready Check ──────────────────────────────────────

    void P2PManager::checkPeerReady(int playerId) {
        std::lock_guard lock(m_peersMutex);
        auto it = m_peers.find(playerId);
        if (it == m_peers.end()) return;

        auto& peer = it->second;
        bool reliableOpen = peer.reliable && peer.reliable->isOpen();
        bool unreliableOpen = peer.unreliable && peer.unreliable->isOpen();

        if (reliableOpen && unreliableOpen && !peer.ready) {
            peer.ready = true;
            int pid = peer.playerId;
            std::string name = peer.playerName;
            int colorIdx = peer.colorIndex;

            log::info("P2PManager: Player {} ({}) fully connected", pid, name);

            // If client connecting to host, we're now Connected
            if (m_role == Role::Client && pid == 0) {
                m_state.store(State::Connected);
            }

            queueInMainThread([this, pid, name, colorIdx]() {
                if (m_role == Role::Client && pid == 0) {
                    auto roomCode = getRoomCode();
                    for (auto& cb : m_onSessionStarted) {
                        cb(roomCode, m_localPlayerId);
                    }
                }

                for (auto& cb : m_onPeerConnected) {
                    cb(pid, name, colorIdx);
                }

                // Host notifies existing clients about the new player
                if (m_role == Role::Host) {
                    auto msg = proto::serializePlayerJoined(pid, name, colorIdx);
                    broadcast(msg, ChannelType::Reliable, pid);
                }
            });
        }
    }

    // ── Leave Session ─────────────────────────────────────────

    void P2PManager::leaveSession() {
        m_pollingSignaling = false;

        // Close all peer connections
        {
            std::lock_guard lock(m_peersMutex);
            for (auto& [id, peer] : m_peers) {
                if (peer.reliable) peer.reliable->close();
                if (peer.unreliable) peer.unreliable->close();
                if (peer.pc) peer.pc->close();
            }
            m_peers.clear();
        }

        // Delete room on signaling server if host
        if (m_role == Role::Host && !m_roomCode.empty()) {
            auto url = getSignalingUrl() + "/rooms/" + m_roomCode;
            auto req = web::WebRequest();
            async::spawn(req.send("DELETE", url)); // Fire-and-forget
        }

        // Clear incoming queue
        {
            std::lock_guard lock(m_incomingMutex);
            std::queue<QueuedMessage> empty;
            std::swap(m_incoming, empty);
        }

        {
            std::lock_guard lock(m_stateMutex);
            m_role = Role::None;
            m_roomCode.clear();
            m_localPlayerId = -1;
            m_localPlayerName.clear();
            m_error.clear();
        }

        m_state.store(State::Disconnected);
        m_nextPlayerId = 1;
        m_signalingRoomId.clear();
        m_pendingLocalSdp.clear();

        log::info("P2PManager: Session ended");
    }

} // namespace mpedit
