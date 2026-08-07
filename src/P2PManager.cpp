#include "P2PManager.hpp"
#include "BinaryProtocol.hpp"

#include <rtc/rtc.hpp>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <thread>
#include <chrono>

using namespace geode::prelude;

namespace mpedit {



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



    rtc::Configuration P2PManager::makeRtcConfig() {
        rtc::Configuration config;
        config.iceServers.push_back({"stun:stun.l.google.com:19302"});
        config.iceServers.push_back({"stun:stun.cloudflare.com:3478"});
        rtc::IceServer turn("openrelay.metered.ca", 443, "openrelayproject", "openrelayproject", rtc::IceServer::RelayType::TurnTcp);
        config.iceServers.push_back(turn);
        return config;
    }

    std::string P2PManager::getSignalingUrl() {
        auto url = Mod::get()->getSettingValue<std::string>("signaling-url");
        if (url.empty()) return "https://dewy-flea-9364.d050.deno.net";
        return url;
    }



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



    void P2PManager::on(proto::Opcode opcode, MessageCallback callback) {
        m_handlers[static_cast<uint8_t>(opcode)].push_back(std::move(callback));
    }

    void P2PManager::clearHandlers() {
        m_handlers.clear();
    }



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
                uint8_t opcodeRaw = msg.data[0];

                auto it = m_handlers.find(opcodeRaw);
                if (it != m_handlers.end()) {
                    auto handlersCopy = it->second;
                    for (auto const& handler : handlersCopy) {
                        proto::Reader handlerReader(msg.data.data() + 1, msg.data.size() - 1);
                        handler(msg.fromPlayerId, handlerReader);
                        if (m_handlers.empty()) break;
                    }
                }
            }

            if (m_handlers.empty()) break;
            messages.pop();
        }

        m_dispatching = false;
    }



    void P2PManager::send(std::vector<uint8_t> const& data, ChannelType channel) {
        if (m_role == Role::Host) {
            broadcast(data, channel);
        } else if (m_role == Role::Client) {
            sendTo(0, data, channel);
        }
    }

    void P2PManager::send(std::vector<uint8_t>&& data, ChannelType channel) {
        send(static_cast<std::vector<uint8_t> const&>(data), channel);
    }

    void P2PManager::sendTo(int playerId, std::vector<uint8_t> const& data, ChannelType channel) {
        std::lock_guard lock(m_peersMutex);
        auto it = m_peers.find(playerId);
        if (it == m_peers.end()) return;

        auto& peer = it->second;
        if (!peer.ready) {
            peer.pendingMessages.push_back({data, channel});
            return;
        }
        auto& dc = (channel == ChannelType::Reliable) ? peer.reliable : peer.unreliable;

        if (dc && dc->isOpen()) {
            dc->send(reinterpret_cast<const std::byte*>(data.data()), data.size());
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



    void P2PManager::onPeerMessage(int fromPlayerId, const uint8_t* data, size_t len) {
        if (len == 0) return;

        {
            std::lock_guard lock(m_incomingMutex);
            m_incoming.push(QueuedMessage{
                fromPlayerId,
                std::vector<uint8_t>(data, data + len)
            });
        }

        if (m_role == Role::Host) {
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
            if (m_role == Role::Client && playerId == 0) {
                for (auto& cb : m_onError) {
                    cb("Host disconnected");
                }
                return;
            }

            for (auto& cb : m_onPeerDisconnected) {
                cb(playerId);
            }

            if (m_role == Role::Host) {
                auto msg = proto::serializePlayerLeft(playerId);
                broadcast(msg, ChannelType::Reliable);
            }
        });
    }



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

                    startSignalPolling(roomCode, "host", 0);
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



    void P2PManager::startSignalPolling(std::string const& code, std::string const& role, int playerId) {
        m_signalingActive.store(true);
        log::info("P2PManager: Starting signaling long poll (role={}, playerId={})", role, playerId);
        pollSignalOnce(code, role, playerId);
    }

    void P2PManager::pollSignalOnce(std::string const& code, std::string const& role, int playerId) {
        if (!m_signalingActive.load()) return;

        auto url = getSignalingUrl() + "/rooms/" + code + "/signal?role=" + role + "&playerId=" + std::to_string(playerId);

        auto req = web::WebRequest();
        req.timeout(std::chrono::seconds(30));

        m_signalPollListener.spawn(
            req.get(url),
            [this, code, role, playerId](web::WebResponse res) {
                if (!m_signalingActive.load()) return;

                if (res.ok()) {
                    auto json = res.json().unwrapOr(matjson::Value());
                    handleSignalingMessages(json);
                } else {
                    log::warn("P2PManager: Signal poll returned {}", res.code());
                }

                if (m_signalingActive.load()) {
                    pollSignalOnce(code, role, playerId);
                }
            }
        );
    }

    void P2PManager::stopSignalPolling() {
        m_signalingActive.store(false);
        m_signalPollListener.cancel();
    }

    void P2PManager::sendSignalingMessage(std::string const& roomCode, matjson::Value const& msg) {
        auto url = getSignalingUrl() + "/rooms/" + roomCode + "/signal";
        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        req.bodyJSON(msg);
        async::spawn(req.post(url));
    }

    void P2PManager::handleSignalingMessages(matjson::Value const& messages) {
        if (!messages.isArray()) return;

        for (size_t i = 0; i < messages.size(); i++) {
            auto msgOpt = messages.get(i);
            if (!msgOpt.isOk()) continue;
            auto msg = msgOpt.unwrap();

            auto type = msg.get<std::string>("type").unwrapOr("");

            if (type == "client_joined") {
                int clientId = msg.get<int>("playerId").unwrapOr(-1);
                auto clientName = msg.get<std::string>("playerName").unwrapOr("Player " + std::to_string(clientId));
                if (clientId >= 0) {
                    log::info("P2PManager: Client {} ({}) connecting via signal poll", clientId, clientName);
                    m_nextPlayerId = std::max(m_nextPlayerId, clientId + 1);
                    createHostPeer(clientId, clientName);
                }
            } else if (type == "answer") {
                auto sdp = msg.get<std::string>("sdp").unwrapOr("");
                int clientId = msg.get<int>("playerId").unwrapOr(-1);
                if (!sdp.empty() && clientId >= 0) {
                    log::info("P2PManager: Received SDP answer from client {} via poll", clientId);

                    size_t setupPos = sdp.find("a=setup:actpass");
                    while (setupPos != std::string::npos) {
                        sdp.replace(setupPos, 15, "a=setup:active");
                        setupPos = sdp.find("a=setup:actpass", setupPos);
                    }

                    log::info("Answer SDP:\n{}", sdp);
                    std::lock_guard lock(m_peersMutex);
                    auto it = m_peers.find(clientId);
                    if (it != m_peers.end() && it->second.pc) {
                        auto state = it->second.pc->signalingState();
                        if (state == rtc::PeerConnection::SignalingState::HaveLocalOffer) {
                            it->second.pc->setRemoteDescription(
                                rtc::Description(sdp, rtc::Description::Type::Answer, rtc::Description::Role::Active));
                            
                            for (auto const& pCand : it->second.pendingCandidates) {
                                it->second.pc->addRemoteCandidate(rtc::Candidate(pCand.candidate, pCand.mid));
                            }
                            it->second.pendingCandidates.clear();
                        } else {
                            log::warn("P2PManager: Ignoring duplicate answer from client {} (state={})", clientId, (int)state);
                        }
                    }
                }
            } else if (type == "offer") {
                auto sdp = msg.get<std::string>("sdp").unwrapOr("");
                if (!sdp.empty()) {
                    std::lock_guard lock(m_peersMutex);
                    auto it = m_peers.find(0);
                    if (it != m_peers.end() && it->second.pc) {
                        auto state = it->second.pc->signalingState();
                        if (state == rtc::PeerConnection::SignalingState::Stable) {
                            log::info("P2PManager: Received host's SDP offer via poll");
                            it->second.pc->setRemoteDescription(
                                rtc::Description(sdp, rtc::Description::Type::Offer, rtc::Description::Role::ActPass));
                            
                            for (auto const& pCand : it->second.pendingCandidates) {
                                it->second.pc->addRemoteCandidate(rtc::Candidate(pCand.candidate, pCand.mid));
                            }
                            it->second.pendingCandidates.clear();

                            it->second.pc->setLocalDescription();
                        } else {
                            log::warn("P2PManager: Ignoring duplicate offer (state={})", (int)state);
                        }
                    }
                }
            } else if (type == "candidate") {
                auto cand = msg.get<std::string>("candidate").unwrapOr("");
                auto mid = msg.get<std::string>("mid").unwrapOr("");
                int clientId = msg.get<int>("playerId").unwrapOr(-1);
                int fromId = (m_role == Role::Host) ? clientId : 0;
                
                if (!cand.empty() && !mid.empty() && fromId >= 0) {
                    std::lock_guard lock(m_peersMutex);
                    auto it = m_peers.find(fromId);
                    if (it != m_peers.end() && it->second.pc) {
                        if (it->second.pc->remoteDescription().has_value()) {
                            rtc::Candidate rtcCand(cand, mid);
                            it->second.pc->addRemoteCandidate(rtcCand);
                        } else {
                            log::info("P2PManager: Remote description not set, buffering candidate from {}", fromId);
                            it->second.pendingCandidates.push_back({cand, mid});
                        }
                    }
                }
            }
        }
    }



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

                    auto pc = std::make_shared<rtc::PeerConnection>(makeRtcConfig());

                    PeerInfo hostPeer;
                    hostPeer.pc = pc;
                    hostPeer.playerId = 0;
                    hostPeer.playerName = hostName;
                    hostPeer.colorIndex = 0;

                    int myId = m_localPlayerId;

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

                    auto answerSent = std::make_shared<bool>(false);

                    pc->onLocalCandidate([this, myId, roomCode](rtc::Candidate candidate) {
                        auto body = matjson::Value();
                        body["type"] = "candidate";
                        body["candidate"] = std::string(candidate.candidate());
                        body["mid"] = std::string(candidate.mid());
                        body["playerId"] = myId;
                        queueInMainThread([this, roomCode, body]() {
                            sendSignalingMessage(roomCode, body);
                        });
                    });

                    pc->onLocalDescription([this, pc, myId, roomCode, answerSent](rtc::Description desc) {
                        std::string sdp = std::string(desc);
                        
                        size_t setupPos = sdp.find("a=setup:actpass");
                        while (setupPos != std::string::npos) {
                            sdp.replace(setupPos, 15, "a=setup:active");
                            setupPos = sdp.find("a=setup:actpass", setupPos);
                        }
                        
                        log::info("P2PManager: Local description set, sending SDP answer via HTTP (early/trickle)");

                        queueInMainThread([this, sdp, myId, roomCode, answerSent]() {
                            if (*answerSent) return;
                            *answerSent = true;
                            auto body = matjson::Value();
                            body["type"] = "answer";
                            body["sdp"] = sdp;
                            body["playerId"] = myId;
                            sendSignalingMessage(roomCode, body);
                        });
                    });

                    pc->onGatheringStateChange([this, pc, myId, roomCode, answerSent](
                        rtc::PeerConnection::GatheringState state)
                    {
                        if (state == rtc::PeerConnection::GatheringState::Complete) {
                            auto desc = pc->localDescription();
                            if (desc.has_value()) {
                                std::string sdp = std::string(desc.value());

                                size_t setupPos = sdp.find("a=setup:actpass");
                                while (setupPos != std::string::npos) {
                                    sdp.replace(setupPos, 15, "a=setup:active");
                                    setupPos = sdp.find("a=setup:actpass", setupPos);
                                }

                                queueInMainThread([this, sdp, myId, roomCode, answerSent]() {
                                    if (*answerSent) return;
                                    *answerSent = true;
                                    log::info("P2PManager: ICE gathering complete, sending SDP answer via HTTP (fallback)");
                                    auto body = matjson::Value();
                                    body["type"] = "answer";
                                    body["sdp"] = sdp;
                                    body["playerId"] = myId;
                                    sendSignalingMessage(roomCode, body);
                                });
                            }
                        }
                    });

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

                    startSignalPolling(roomCode, "client", m_localPlayerId);

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



    void P2PManager::createHostPeer(int clientPlayerId, std::string const& clientName) {
        auto pc = std::make_shared<rtc::PeerConnection>(makeRtcConfig());

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

        auto roomCode = getRoomCode();

        auto offerSent = std::make_shared<bool>(false);

        pc->onLocalCandidate([this, clientPlayerId, roomCode](rtc::Candidate candidate) {
            auto body = matjson::Value();
            body["type"] = "candidate";
            body["candidate"] = std::string(candidate.candidate());
            body["mid"] = std::string(candidate.mid());
            body["targetPlayerId"] = clientPlayerId;
            queueInMainThread([this, roomCode, body]() {
                sendSignalingMessage(roomCode, body);
            });
        });

        pc->onLocalDescription([this, clientPlayerId, roomCode, offerSent](rtc::Description desc) {
            std::string sdp = std::string(desc);
            log::info("P2PManager: Local description set, sending SDP offer for player {} via HTTP (early/trickle)", clientPlayerId);

            queueInMainThread([this, sdp, clientPlayerId, roomCode, offerSent]() {
                if (*offerSent) return;
                *offerSent = true;
                auto body = matjson::Value();
                body["type"] = "offer";
                body["sdp"] = sdp;
                body["targetPlayerId"] = clientPlayerId;
                sendSignalingMessage(roomCode, body);
            });
        });

        pc->onGatheringStateChange([this, pc, clientPlayerId, roomCode, offerSent](
            rtc::PeerConnection::GatheringState state)
        {
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto desc = pc->localDescription();
                if (desc.has_value()) {
                    std::string sdp = std::string(desc.value());

                    queueInMainThread([this, sdp, clientPlayerId, roomCode, offerSent]() {
                        if (*offerSent) return;
                        *offerSent = true;
                        log::info("P2PManager: ICE gathering complete, sending SDP offer for player {} via HTTP (fallback)", clientPlayerId);
                        auto body = matjson::Value();
                        body["type"] = "offer";
                        body["sdp"] = sdp;
                        body["targetPlayerId"] = clientPlayerId;
                        sendSignalingMessage(roomCode, body);
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

        pc->setLocalDescription();

        {
            std::lock_guard lock(m_peersMutex);
            m_peers[clientPlayerId] = std::move(peer);
        }
    }



    void P2PManager::checkPeerReady(int playerId) {
        bool becameReady = false;
        int pid = -1;
        std::string name;
        int colorIdx = 0;
        std::vector<PendingMessage> pending;

        {
            std::lock_guard lock(m_peersMutex);
            auto it = m_peers.find(playerId);
            if (it == m_peers.end()) return;

            auto& peer = it->second;
            bool reliableOpen = peer.reliable && peer.reliable->isOpen();
            bool unreliableOpen = peer.unreliable && peer.unreliable->isOpen();

            if (reliableOpen && unreliableOpen && !peer.ready) {
                peer.ready = true;
                pid = peer.playerId;
                name = peer.playerName;
                colorIdx = peer.colorIndex;
                becameReady = true;

                pending = std::move(peer.pendingMessages);
                peer.pendingMessages.clear();

                log::info("P2PManager: Player {} ({}) fully connected", pid, name);
            }
        }

        if (!becameReady) return;

        for (auto& msg : pending) {
            sendTo(pid, msg.data, msg.channel);
        }

        if (m_role == Role::Client && pid == 0) {
            m_state.store(State::Connected);
            stopSignalPolling();
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

            if (m_role == Role::Host) {
                auto msg = proto::serializePlayerJoined(pid, name, colorIdx);
                broadcast(msg, ChannelType::Reliable, pid);
            }
        });
    }



    void P2PManager::leaveSession() {
        stopSignalPolling();

        {
            std::lock_guard lock(m_peersMutex);
            for (auto& [id, peer] : m_peers) {
                if (peer.reliable) peer.reliable->close();
                if (peer.unreliable) peer.unreliable->close();
                if (peer.pc) peer.pc->close();
            }
            m_peers.clear();
        }

        if (m_role == Role::Host && !m_roomCode.empty()) {
            auto url = getSignalingUrl() + "/rooms/" + m_roomCode;
            auto req = web::WebRequest();
            async::spawn(req.send("DELETE", url));
        }

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

        log::info("P2PManager: Session ended");
    }

} // namespace mpedit
