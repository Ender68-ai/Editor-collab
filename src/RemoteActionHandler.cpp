#include "RemoteActionHandler.hpp"
#include "P2PManager.hpp"
#include "BinaryProtocol.hpp"
#include "SessionManager.hpp"
#include "MessageBatcher.hpp"
#include "ui/MultiplayerPopup.hpp"
#include <Geode/Geode.hpp>
#include <random>
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>

using namespace geode::prelude;

namespace mpedit {

    namespace {
        // Captures the set of object pointers currently in the editor, so that
        // after a bulk createObjectsFromString() call we can identify exactly
        // which objects are new — regardless of whether GD appended them to the
        // front or the back of m_objects (the insertion position is not a stable
        // contract and varies across GD versions).
        std::set<GameObject*> snapshotExistingObjects(LevelEditorLayer* editor) {
            std::set<GameObject*> existing;
            if (editor && editor->m_objects) {
                for (auto* obj : CCArrayExt<GameObject*>(editor->m_objects)) {
                    if (obj) existing.insert(obj);
                }
            }
            return existing;
        }

        std::vector<GameObject*> createObjectsFromSaveStringRobust(LevelEditorLayer* editor, std::string const& saveStr) {
            std::vector<GameObject*> newObjects;
            if (!editor || saveStr.empty()) return newObjects;

            // Snapshot existing objects BEFORE creating, so we can tell new from old
            // without depending on m_objects insertion order.
            auto existing = snapshotExistingObjects(editor);

            editor->createObjectsFromString(saveStr, true, true);

            // Return the freshly created objects in m_objects iteration order.
            // This is the same relative order the host serialized them in
            // (getSaveString per object, joined by ';'), so it aligns 1:1 with
            // the host-supplied uuids[] array.
            if (editor->m_objects) {
                for (auto* obj : CCArrayExt<GameObject*>(editor->m_objects)) {
                    if (obj && !existing.count(obj)) {
                        newObjects.push_back(obj);
                    }
                }
            }
            return newObjects;
        }

        // Applies rotation/scale/flip to an object in a way that is IDENTITY on a
        // healthy object and self-correcting on a corrupted one.
        //
        // Why this exists: in GD, "flip" is encoded BOTH as the m_isFlipX/Y bool
        // AND as the SIGN of scaleX/scaleY (e.g. flipX=true  <=>  scaleX<0). GD's
        // GameObject::setFlipX(bool) only negates the scale when the flip state
        // *changes*, and our naive sequence
        // Fix: Geometry Dash treats flipX/Y and scaleX/Y signs as orthogonal properties
        // depending on the object type. Solid blocks mirror using negative scaleX without
        // changing flipX, while decorations use flipX without changing scaleX.
        // We must apply both exactly as received to prevent breaking the mirror state.
        void applyTransformSafe(GameObject* obj, float rotation, float scaleX, float scaleY, bool flipX, bool flipY) {
            if (!obj) return;
            obj->setRotation(rotation);
            obj->setFlipX(flipX);
            obj->setFlipY(flipY);
            obj->setScaleX(scaleX);
            obj->setScaleY(scaleY);
        }
    }

    RemoteActionHandler& RemoteActionHandler::get() {
        static RemoteActionHandler instance;
        return instance;
    }

    void RemoteActionHandler::setupHandlers() {
        MusicDownloadManager::sharedState()->addMusicDownloadDelegate(this);

        auto& net = P2PManager::get();

        net.on(proto::Opcode::PlaceObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializePlaceObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing PlaceObjects");
                return;
            }
            handleRemotePlaceObjects(playerId, msg.objects);
        });

        net.on(proto::Opcode::DeleteObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeDeleteObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing DeleteObjects");
                return;
            }
            handleRemoteDeleteObjects(playerId, msg.uuids);
        });

        net.on(proto::Opcode::MoveObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeMoveObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing MoveObjects");
                return;
            }
            handleRemoteMoveObjects(playerId, msg.moves);
        });

        net.on(proto::Opcode::MoveBatch, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeMoveBatch(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing MoveBatch");
                return;
            }
            handleRemoteMoveObjects(playerId, msg.moves);
        });

        net.on(proto::Opcode::TransformObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeTransformObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing TransformObjects");
                return;
            }
            handleRemoteTransformObjects(playerId, msg.transforms);
        });

        net.on(proto::Opcode::ReconcileObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeReconcileObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing ReconcileObjects");
                return;
            }
            handleRemoteReconcileObjects(playerId, msg.reconciles);
        });

        net.on(proto::Opcode::UpdateObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeUpdateObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing UpdateObjects");
                return;
            }
            handleRemoteUpdateObjects(playerId, msg.objects);
        });

        net.on(proto::Opcode::LockObjects, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeLockObjects(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing LockObjects");
                return;
            }
            handleRemoteLockObjects(playerId, msg.uuids, msg.locked);
        });

        net.on(proto::Opcode::UpdateSettings, [this](int playerId, proto::Reader& reader) {
            if (playerId == P2PManager::get().getLocalPlayerId()) return;
            auto msg = proto::deserializeUpdateSettings(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing UpdateSettings");
                return;
            }
            handleRemoteUpdateSettings(playerId, msg.settings);
        });

        net.on(proto::Opcode::SyncLevelStart, [this](int playerId, proto::Reader& reader) {
            auto msg = proto::deserializeSyncLevelStart(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing SyncLevelStart");
                return;
            }
            m_chunkedSync.hostPlayerId = playerId;
            m_chunkedSync.totalChunks = msg.totalChunks;
            m_chunkedSync.totalObjects = msg.totalObjects;
            m_chunkedSync.settings = msg.settings;
            m_chunkedSync.chunks.clear();
            m_chunkedSync.chunks.resize(msg.totalChunks);
            m_chunkedSync.uuidChunks.clear();
            m_chunkedSync.uuidChunks.resize(msg.totalChunks);
            m_chunkedSync.active = true;
            log::info("RemoteActionHandler: SyncLevelStart received ({} chunks, {} objects)",
                msg.totalChunks, msg.totalObjects);
        });

        net.on(proto::Opcode::SyncLevelChunk, [this](int playerId, proto::Reader& reader) {
            auto msg = proto::deserializeSyncLevelChunk(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing SyncLevelChunk");
                return;
            }
            if (!m_chunkedSync.active || playerId != m_chunkedSync.hostPlayerId) return;
            if (msg.chunkIndex < m_chunkedSync.totalChunks) {
                m_chunkedSync.chunks[msg.chunkIndex] = std::string(msg.data.begin(), msg.data.end());
                m_chunkedSync.uuidChunks[msg.chunkIndex] = msg.uuids;
                log::info("RemoteActionHandler: SyncLevelChunk received: {}/{}",
                    msg.chunkIndex + 1, m_chunkedSync.totalChunks);
            }
        });

        net.on(proto::Opcode::SyncLevelEnd, [this](int playerId, proto::Reader& reader) {
            auto msg = proto::deserializeSyncLevelEnd(reader);
            if (reader.hasError()) {
                log::error("RemoteActionHandler: Error deserializing SyncLevelEnd");
                return;
            }
            if (!m_chunkedSync.active || playerId != m_chunkedSync.hostPlayerId) return;

            // Reconstruct objectsString
            std::string objectsString = "";
            for (auto const& chunk : m_chunkedSync.chunks) {
                objectsString += chunk;
            }
            
            // Reconstruct uuids
            std::vector<std::string> uuids;
            uuids.reserve(m_chunkedSync.totalObjects);
            for (auto const& uuidChunk : m_chunkedSync.uuidChunks) {
                uuids.insert(uuids.end(), uuidChunk.begin(), uuidChunk.end());
            }

            log::info("RemoteActionHandler: SyncLevelEnd received, processing full sync");
            handleRemoteSyncLevel(playerId, objectsString, uuids, m_chunkedSync.settings, msg.locks);

            m_chunkedSync.active = false;
            m_chunkedSync.chunks.clear();
            m_chunkedSync.uuidChunks.clear();
        });
    }

    void RemoteActionHandler::clearHandlers() {
        MusicDownloadManager::sharedState()->removeMusicDownloadDelegate(this);
        clearMappings();
        m_expectedUuids.clear();
        m_objectLocks.clear();
        m_pendingSync.reset();
        m_initialSyncCompleted = false;
        m_chunkedSync.active = false;
        m_chunkedSync.chunks.clear();
        m_chunkedSync.uuidChunks.clear();
        P2PManager::get().clearHandlers();
    }

    static LevelEditorLayer* findEditorLayer(CCNode* parent) {
        if (!parent) return nullptr;
        if (auto* editor = typeinfo_cast<LevelEditorLayer*>(parent)) {
            return editor;
        }
        if (parent->getChildren()) {
            for (auto* child : CCArrayExt<CCNode*>(parent->getChildren())) {
                if (auto* editor = findEditorLayer(child)) {
                    return editor;
                }
            }
        }
        return nullptr;
    }

    // Returns true only when the editor has finished init() — m_editorUI is the
    // last field GD wires up during LevelEditorLayer::init(), so its presence is
    // a reliable "ready to receive mutations" signal. Using a half-initialized
    // editor (e.g. one discovered via getNextScene() mid-transition) risks
    // spawning objects into a broken editor state.
    bool isEditorReady(LevelEditorLayer* editor) {
        return editor && editor->m_editorUI;
    }

    LevelEditorLayer* RemoteActionHandler::getEditorLayer() const {
        // Init-bridge: when applyPendingSync() runs from inside init(), the
        // editor isn't in the scene graph yet, so the searches below would miss
        // it and re-enter the "no editor" branch (recursion). Honor an explicit
        // override set by the init() code path.
        if (m_editorForInit) {
            return m_editorForInit;
        }

        // Preferred path: GD's own lookup, which only returns the *currently
        // running* editor (so it can't hand back a tearing-down instance).
        if (auto* editor = LevelEditorLayer::get()) {
            if (isEditorReady(editor)) {
                return editor;
            }
            // GD's static returned something but init() isn't done yet — fall
            // through to the scene walk so callers can decide via the pending
            // sync path rather than mutating a half-built editor.
            log::debug("RemoteActionHandler: LevelEditorLayer::get() returned an unready editor, falling through");
        }

        auto* dir = CCDirector::sharedDirector();
        if (auto* scene = dir->getRunningScene()) {
            if (auto* editor = findEditorLayer(scene)) {
                if (isEditorReady(editor)) {
                    return editor;
                }
            }
        }
        // Next-scene fallback: during pushScene() the editor exists in the
        // upcoming scene but isn't the running one yet. We intentionally return
        // it here (still possibly mid-init) because the pending-sync flow needs
        // to detect its existence and defer spawning until init() completes.
        if (auto* nextScene = dir->getNextScene()) {
            if (auto* editor = findEditorLayer(nextScene)) {
                log::debug("RemoteActionHandler: editor resolved via getNextScene() (ready={})",
                    isEditorReady(editor));
                return editor;
            }
        }
        return nullptr;
    }

    void RemoteActionHandler::applyPendingSync() {
        if (!m_pendingSync) {
            log::debug("RemoteActionHandler: applyPendingSync called but no pending sync");
            return;
        }
        auto sync = m_pendingSync.value();
        m_pendingSync.reset();
        log::info("RemoteActionHandler: Applying pending sync (objectsStringLen={}, uuids={}, settingsLen={}, locks={})",
            sync.objectsString.size(), sync.uuids.size(), sync.settings.saveString.size(), sync.locks.size());
        // If we were given an editor override (init-bridge), make sure it's
        // cleared afterwards so we don't pin a possibly-destroyed editor.
        LevelEditorLayer* override = m_editorForInit;
        handleRemoteSyncLevel(sync.playerId, sync.objectsString, sync.uuids, sync.settings, sync.locks, true);
        m_editorForInit = nullptr;
        (void)override;
    }

    void RemoteActionHandler::handleRemotePlaceObjects(
        int playerId, 
        std::vector<ActionSerializer::ObjectData> const& objects
    ) {
        auto* editor = getEditorLayer();
        if (!editor) {
            log::warn("RemoteActionHandler: No editor layer found");
            return;
        }

        if (editor->m_playbackMode != PlaybackMode::Not) {
            QueuedAction qa;
            qa.type = QueuedAction::Type::Place;
            qa.playerId = playerId;
            qa.placeObjects = objects;
            m_playtestQueue.push_back(std::move(qa));
            return;
        }

        m_processingRemote = true;

        std::unordered_set<std::string> processedUUIDs;

        for (size_t i = 0; i < objects.size(); i++) {
            auto const& objData = objects[i];
            if (processedUUIDs.count(objData.uuid)) continue;
            processedUUIDs.insert(objData.uuid);
            
            // Recreate object using saveString if available to load all properties (e.g. text contents, custom colors, groups)
            if (!objData.saveString.empty()) {
                auto newObjs = createObjectsFromSaveStringRobust(editor, objData.saveString);
                if (!newObjs.empty()) {
                    GameObject* obj = nullptr;
                    for (auto* createdObj : newObjs) {
                        if (createdObj->m_objectID == objData.objectID) {
                            if (objData.objectID == 747) {
                                if (auto* tp = typeinfo_cast<TeleportPortalObject*>(createdObj)) {
                                    if (!tp->m_isYellowPortal) {
                                        obj = createdObj;
                                        break;
                                    }
                                }
                            } else {
                                obj = createdObj;
                                break;
                            }
                        }
                    }
                    if (!obj) obj = newObjs.front();
                    // Re-apply the authoritative transform read by the sender via
                    // the same getter APIs. Use applyTransformSafe so the flip
                    // flag and the scale sign stay consistent (see its comment) —
                    // a naive setScaleX+setFlipX here double-applies the flip and
                    // lands the remote at the OPPOSITE state from the host.
                    applyTransformSafe(obj, objData.rotation, objData.scaleX, objData.scaleY, objData.flipX, objData.flipY);
                    registerObject(objData.uuid, obj);
                    log::debug("RemoteActionHandler: Placed object {} via saveString (uuid={})", objData.objectID, objData.uuid);
                    
                    if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(obj)) {
                        if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                            for (size_t j = 0; j < objects.size(); j++) {
                                auto const& orangeData = objects[j];
                                if (orangeData.objectID == 749 && !processedUUIDs.count(orangeData.uuid)) {
                                    auto* orange = tpPortal->m_orangePortal;
                                    orange->setPositionOverride({orangeData.x, orangeData.y});
                                    applyTransformSafe(orange, orangeData.rotation, orangeData.scaleX,
                                                       orangeData.scaleY, orangeData.flipX, orangeData.flipY);
                                    registerObject(orangeData.uuid, orange);
                                    processedUUIDs.insert(orangeData.uuid);
                                    break;
                                }
                            }
                        }
                    }
                    continue;
                }
            }

            // Fallback to basic creation if saveString is empty or failed
            auto* obj = editor->createObject(objData.objectID, {objData.x, objData.y}, true);
            if (!obj) {
                log::warn("RemoteActionHandler: Failed to create object ID {}", objData.objectID);
                continue;
            }

            applyTransformSafe(obj, objData.rotation, objData.scaleX, objData.scaleY, objData.flipX, objData.flipY);
            obj->m_editorLayer = objData.editorLayer;
            obj->m_editorLayer2 = objData.editorLayer2;

            registerObject(objData.uuid, obj);
            log::debug("RemoteActionHandler: Placed object {} (uuid={})", objData.objectID, objData.uuid);
            
            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(obj)) {
                if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                    for (size_t j = 0; j < objects.size(); j++) {
                        auto const& orangeData = objects[j];
                        if (orangeData.objectID == 749 && !processedUUIDs.count(orangeData.uuid)) {
                            auto* orange = tpPortal->m_orangePortal;
                            orange->setPositionOverride({orangeData.x, orangeData.y});
                            applyTransformSafe(orange, orangeData.rotation, orangeData.scaleX,
                                               orangeData.scaleY, orangeData.flipX, orangeData.flipY);
                            registerObject(orangeData.uuid, orange);
                            processedUUIDs.insert(orangeData.uuid);
                            break;
                        }
                    }
                }
            }
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteDeleteObjects(
        int playerId, 
        std::vector<std::string> const& uuids
    ) {
        auto* editor = getEditorLayer();
        if (!editor) return;

        if (editor->m_playbackMode != PlaybackMode::Not) {
            QueuedAction qa;
            qa.type = QueuedAction::Type::Delete;
            qa.playerId = playerId;
            qa.deleteUuids = uuids;
            m_playtestQueue.push_back(std::move(qa));
            return;
        }

        m_processingRemote = true;

        for (auto& uuid : uuids) {
            auto* obj = getObjectByUUID(uuid);
            if (!obj) {
                log::warn("RemoteActionHandler: Object with uuid '{}' not found for deletion", uuid);
                continue;
            }

            // Deselect first to prevent dangling pointer crashes in EditorUI
            if (auto* editorUI = editor->m_editorUI) {
                if (editorUI->m_selectedObject == obj || (editorUI->m_selectedObjects && editorUI->m_selectedObjects->containsObject(obj))) {
                    editorUI->deselectObject(obj);
                    if (editorUI->m_selectedObject == obj) {
                        editorUI->m_selectedObject = nullptr;
                    }
                    if (editorUI->m_selectedObjects && editorUI->m_selectedObjects->containsObject(obj)) {
                        editorUI->m_selectedObjects->removeObject(obj);
                    }
                }
            }

            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(obj)) {
                if (tpPortal->m_isYellowPortal) {
                    // Do not explicitly delete orange portals. The engine will delete them
                    // when the primary blue portal is deleted. Deleting it explicitly
                    // leaves a dangling pointer in the blue portal!
                    continue;
                }
            }

            pruneObjectFromHistory(editor, obj);
            
            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(obj)) {
                if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                    auto orangeUuid = getUUIDForObject(tpPortal->m_orangePortal);
                    auto* orange = tpPortal->m_orangePortal;
                    tpPortal->m_orangePortal = nullptr; // Prevent double-free in engine
                    editor->removeObject(orange, true);
                    if (!orangeUuid.empty()) unregisterObject(orangeUuid);
                }
            }

            editor->removeObject(obj, true);
            unregisterObject(uuid);
            log::debug("RemoteActionHandler: Deleted object (uuid={})", uuid);
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteMoveObjects(
        int playerId, 
        std::vector<ActionSerializer::MoveData> const& moves
    ) {
        auto* editor = getEditorLayer();
        if (!editor) return;

        if (editor->m_playbackMode != PlaybackMode::Not) {
            QueuedAction qa;
            qa.type = QueuedAction::Type::Move;
            qa.playerId = playerId;
            qa.moveData = moves;
            m_playtestQueue.push_back(std::move(qa));
            return;
        }

        m_processingRemote = true;

        for (auto& move : moves) {
            auto* obj = getObjectByUUID(move.uuid);
            if (!obj) {
                log::warn("RemoteActionHandler: Object with uuid '{}' not found for move", move.uuid);
                continue;
            }

            auto pos = obj->getPosition();
            obj->setPosition({pos.x + move.dx, pos.y + move.dy});
            editor->updateObjectSection(obj);
            
            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(obj)) {
                if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                    editor->updateObjectSection(tpPortal->m_orangePortal);
                }
            }

            // Move makes any pending position in locked state stale
            // (Removed lockedSaveStrings handling)
            log::debug("RemoteActionHandler: Moved object (uuid={}) by ({}, {})", move.uuid, move.dx, move.dy);
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteTransformObjects(
        int playerId,
        std::vector<ActionSerializer::TransformData> const& transforms
    ) {
        auto* editor = getEditorLayer();
        if (!editor) return;

        if (editor->m_playbackMode != PlaybackMode::Not) {
            QueuedAction qa;
            qa.type = QueuedAction::Type::Transform;
            qa.playerId = playerId;
            qa.transformData = transforms;
            m_playtestQueue.push_back(std::move(qa));
            return;
        }

        log::debug("RemoteActionHandler: applying remote transform (playerId={}, n={})", playerId, transforms.size());
        m_processingRemote = true;

        for (auto& t : transforms) {
            auto* obj = getObjectByUUID(t.uuid);
            if (!obj) {
                log::warn("RemoteActionHandler: Object with uuid '{}' not found for transform", t.uuid);
                continue;
            }

            applyTransformSafe(obj, t.rotation, t.scaleX, t.scaleY, t.flipX, t.flipY);

            // Transform makes any pending transform in locked state stale
            // (Removed lockedSaveStrings handling)
            log::debug("RemoteActionHandler: transformed object (uuid={}..., rot={:.1f}, flipX={}, flipY={})",
                t.uuid.substr(0, 8), t.rotation, t.flipX, t.flipY);
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteReconcileObjects(
        int playerId,
        std::vector<ActionSerializer::ReconcileData> const& reconciles
    ) {
        auto* editor = getEditorLayer();
        if (!editor) return;

        if (editor->m_playbackMode != PlaybackMode::Not) {
            QueuedAction qa;
            qa.type = QueuedAction::Type::Reconcile;
            qa.playerId = playerId;
            qa.reconcileData = reconciles;
            m_playtestQueue.push_back(std::move(qa));
            return;
        }

        log::debug("RemoteActionHandler: applying remote reconcile (playerId={}, n={})", playerId, reconciles.size());
        m_processingRemote = true;

        for (auto& r : reconciles) {
            auto* obj = getObjectByUUID(r.uuid);
            if (!obj) {
                log::warn("RemoteActionHandler: Object with uuid '{}' not found for reconcile", r.uuid);
                continue;
            }

            // Set absolute position
            obj->setPosition(cocos2d::CCPoint{r.x, r.y});
            
            // Set absolute transform
            applyTransformSafe(obj, r.rotation, r.scaleX, r.scaleY, r.flipX, r.flipY);
            editor->updateObjectSection(obj);

            // Reconcile makes any pending transforms/moves in locked state stale
            // (Removed lockedSaveStrings handling)

            log::debug("RemoteActionHandler: reconciled object (uuid={}..., pos=({}, {}), rot={:.1f})",
                r.uuid.substr(0, 8), r.x, r.y, r.rotation);
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteUpdateObjects(
        int playerId, 
        std::vector<ActionSerializer::ObjectData> const& objects
    ) {
        auto* editor = getEditorLayer();
        if (!editor) return;

        if (editor->m_playbackMode != PlaybackMode::Not) {
            QueuedAction qa;
            qa.type = QueuedAction::Type::Update;
            qa.playerId = playerId;
            qa.updateObjects = objects;
            m_playtestQueue.push_back(std::move(qa));
            return;
        }

        m_processingRemote = true;
        std::unordered_set<std::string> processedUUIDs;

        for (size_t i = 0; i < objects.size(); i++) {
            auto const& objData = objects[i];
            auto* oldObj = getObjectByUUID(objData.uuid);
            if (!oldObj) {
                log::warn("RemoteActionHandler: Object to update not found (uuid={})", objData.uuid);
                continue;
            }

            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(oldObj)) {
                if (tpPortal->m_isYellowPortal) {
                    // Never delete and recreate an orange portal; its lifecycle is bound to the blue portal.
                    // Doing so breaks the internal link and causes crashes when the engine accesses it.
                    tpPortal->setPositionOverride({objData.x, objData.y});
                    applyTransformSafe(oldObj, objData.rotation, objData.scaleX, objData.scaleY, objData.flipX, objData.flipY);
                    log::debug("RemoteActionHandler: Updated orange portal directly without recreation");
                    continue;
                }
            }

            // Removed deferred update for locked objects; UpdateObjects now only happens on deep property changes, so recreating immediately is safe and fixes delayed sync.

            // Get selection state
            auto* editorUI = editor->m_editorUI;
            bool wasSelected = false;
            if (editorUI && (editorUI->m_selectedObject == oldObj || (editorUI->m_selectedObjects && editorUI->m_selectedObjects->containsObject(oldObj)))) {
                wasSelected = true;
                editorUI->deselectObject(oldObj);
                if (editorUI->m_selectedObject == oldObj) {
                    editorUI->m_selectedObject = nullptr;
                }
                if (editorUI->m_selectedObjects && editorUI->m_selectedObjects->containsObject(oldObj)) {
                    editorUI->m_selectedObjects->removeObject(oldObj);
                }
            }

            pruneObjectFromHistory(editor, oldObj);
            
            std::string orangeOldUuid;
            ActionSerializer::ObjectData oldOrangeData;
            bool hadOldOrange = false;
            
            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(oldObj)) {
                if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                    orangeOldUuid = getUUIDForObject(tpPortal->m_orangePortal);
                    oldOrangeData = ActionSerializer::extractObjectData(tpPortal->m_orangePortal, orangeOldUuid);
                    hadOldOrange = true;
                    
                    auto* orange = tpPortal->m_orangePortal;
                    tpPortal->m_orangePortal = nullptr; // Prevent double-free in engine
                    editor->removeObject(orange, true);
                    if (!orangeOldUuid.empty()) unregisterObject(orangeOldUuid);
                }
            }

            // Remove old object
            editor->removeObject(oldObj, true);
            unregisterObject(objData.uuid);

            // Recreate object using saveString to ensure ALL properties are loaded
            auto newObjs = createObjectsFromSaveStringRobust(editor, objData.saveString);
            if (!newObjs.empty()) {
                GameObject* newObj = nullptr;
                for (auto* createdObj : newObjs) {
                    if (createdObj->m_objectID == objData.objectID) {
                        if (objData.objectID == 747) {
                            if (auto* tp = typeinfo_cast<TeleportPortalObject*>(createdObj)) {
                                if (!tp->m_isYellowPortal) {
                                    newObj = createdObj;
                                    break;
                                }
                            }
                        } else {
                            newObj = createdObj;
                            break;
                        }
                    }
                }
                if (!newObj) newObj = newObjs.front();
                // Re-apply the authoritative transform via applyTransformSafe so
                // the flip flag and scale sign stay consistent (a naive
                // setScaleX+setFlipX here double-applies the flip). See the
                // helper's comment for the full rationale.
                applyTransformSafe(newObj, objData.rotation, objData.scaleX, objData.scaleY, objData.flipX, objData.flipY);
                registerObject(objData.uuid, newObj);
                log::debug("RemoteActionHandler: Updated object {} via saveString", objData.uuid);
                
                if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(newObj)) {
                    if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                        bool foundInData = false;
                        for (size_t j = 0; j < objects.size(); j++) {
                            auto const& orangeData = objects[j];
                            if (orangeData.objectID == 749 && !processedUUIDs.count(orangeData.uuid)) {
                                auto* orange = tpPortal->m_orangePortal;
                                orange->setPositionOverride({orangeData.x, orangeData.y});
                                applyTransformSafe(orange, orangeData.rotation, orangeData.scaleX,
                                                   orangeData.scaleY, orangeData.flipX, orangeData.flipY);
                                registerObject(orangeData.uuid, orange);
                                processedUUIDs.insert(orangeData.uuid);
                                foundInData = true;
                                break;
                            }
                        }
                        if (!foundInData && hadOldOrange) {
                            // Recover its old state if not in the update batch
                            auto* orange = tpPortal->m_orangePortal;
                            orange->setPositionOverride({oldOrangeData.x, oldOrangeData.y});
                            applyTransformSafe(orange, oldOrangeData.rotation, oldOrangeData.scaleX,
                                               oldOrangeData.scaleY, oldOrangeData.flipX, oldOrangeData.flipY);
                            if (!orangeOldUuid.empty()) {
                                registerObject(orangeOldUuid, orange);
                            }
                        }
                    }
                }

                if (wasSelected && editorUI) {
                    editorUI->selectObject(newObj, true);
                }
            } else {
                // Fallback: recreate with basic properties if saveString parsing failed
                auto* fallbackObj = editor->createObject(objData.objectID, {objData.x, objData.y}, true);
                if (fallbackObj) {
                    applyTransformSafe(fallbackObj, objData.rotation, objData.scaleX, objData.scaleY, objData.flipX, objData.flipY);
                    fallbackObj->m_editorLayer = objData.editorLayer;
                    fallbackObj->m_editorLayer2 = objData.editorLayer2;
                    registerObject(objData.uuid, fallbackObj);
                    log::warn("RemoteActionHandler: Updated object {} via fallback createObject", objData.uuid);
                    
                    if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(fallbackObj)) {
                        if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                            bool foundInData = false;
                            for (size_t j = 0; j < objects.size(); j++) {
                                auto const& orangeData = objects[j];
                                if (orangeData.objectID == 749 && !processedUUIDs.count(orangeData.uuid)) {
                                    auto* orange = tpPortal->m_orangePortal;
                                    orange->setPositionOverride({orangeData.x, orangeData.y});
                                    applyTransformSafe(orange, orangeData.rotation, orangeData.scaleX,
                                                       orangeData.scaleY, orangeData.flipX, orangeData.flipY);
                                    registerObject(orangeData.uuid, orange);
                                    processedUUIDs.insert(orangeData.uuid);
                                    foundInData = true;
                                    break;
                                }
                            }
                            if (!foundInData && hadOldOrange) {
                                auto* orange = tpPortal->m_orangePortal;
                                orange->setPositionOverride({oldOrangeData.x, oldOrangeData.y});
                                applyTransformSafe(orange, oldOrangeData.rotation, oldOrangeData.scaleX,
                                                   oldOrangeData.scaleY, oldOrangeData.flipX, oldOrangeData.flipY);
                                if (!orangeOldUuid.empty()) {
                                    registerObject(orangeOldUuid, orange);
                                }
                            }
                        }
                    }

                    if (wasSelected && editorUI) {
                        editorUI->selectObject(fallbackObj, true);
                    }
                } else {
                    log::error("RemoteActionHandler: Failed to create updated object from saveString AND fallback");
                }
            }
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteLockObjects(
        int playerId, 
        std::vector<std::string> const& uuids, 
        bool locked
    ) {
        m_processingRemote = true;

        if (locked) {
            auto* editor = getEditorLayer();
            auto* editorUI = editor ? editor->m_editorUI : nullptr;
            for (auto& uuid : uuids) {
                // Set lock timeout to 3 seconds. It will be refreshed by cursor_update or explicitly released
                m_objectLocks[uuid] = LockInfo { playerId, 3.0f }; 
                
                // Deselect locked objects if we have them selected
                if (editorUI) {
                    auto* obj = getObjectByUUID(uuid);
                    if (obj) {
                        if (editorUI->m_selectedObject == obj || (editorUI->m_selectedObjects && editorUI->m_selectedObjects->containsObject(obj))) {
                            editorUI->deselectObject(obj);
                            if (editorUI->m_selectedObject == obj) {
                                editorUI->m_selectedObject = nullptr;
                            }
                            if (editorUI->m_selectedObjects && editorUI->m_selectedObjects->containsObject(obj)) {
                                editorUI->m_selectedObjects->removeObject(obj);
                            }
                        }
                    }
                }
            }
        } else {
            for (auto& uuid : uuids) {
                auto it = m_objectLocks.find(uuid);
                if (it != m_objectLocks.end() && it->second.playerId == playerId) {
                    m_objectLocks.erase(it);
                    // Object recreation on unlock removed since UpdateObjects are now applied immediately.
                }
            }
        }

        m_processingRemote = false;
    }

    void RemoteActionHandler::handleRemoteSyncLevel(
        int playerId,
        std::string const& objectsString,
        std::vector<std::string> const& uuids,
        ActionSerializer::LevelSettingsData const& settings,
        std::vector<ActionSerializer::LockData> const& locks,
        bool isPendingSync
    ) {
        log::info("RemoteActionHandler: sync_level received (playerId={}, objectsStringLen={}, uuids={}, settingsLen={}, locks={}, pending={})",
            playerId, objectsString.size(), uuids.size(), settings.saveString.size(), locks.size(), isPendingSync);

        auto* editor = getEditorLayer();
        if (!editor) {
            log::info("RemoteActionHandler: Editor not ready yet, opening editor with settings-only level string");

            // settings.saveString is the LevelSettingsObject string (colors,
            // start mode, song, etc.) — it becomes the level's settings.
            // Objects come in separately via objectsString and are spawned by
            // applyPendingSync() once the editor's init() has run.
            std::string levelString = settings.saveString;
            m_expectedUuids = uuids;

            // IMPORTANT: queue the pending sync BEFORE scene(). LevelEditorLayer::
            // scene() runs init() *synchronously*, and init() calls
            // applyPendingSync(). If we queue after scene() (as we used to), init()
            // sees no pending sync, never spawns the objects, and the level loads
            // empty (settings-only). This was the root cause of the empty-level bug.
            m_pendingSync = PendingSync {
                playerId,
                objectsString,
                uuids,
                settings,
                locks
            };

            // Create game level
            auto* level = GJGameLevel::create();
            level->m_levelName = "Multiplayer Session";
            level->m_levelType = GJLevelType::Editor;
            level->m_levelString = levelString;
            level->m_audioTrack = settings.audioTrack;
            level->m_songID = settings.songID;
            level->m_levelLength = settings.levelLength;

            // Open the editor scene (this synchronously runs the editor's init(),
            // which will detect the pending sync above and spawn the objects).
            auto* scene = LevelEditorLayer::scene(level, false);
            if (!scene) {
                log::error("RemoteActionHandler: LevelEditorLayer::scene returned null — cannot open editor for sync!");
                m_pendingSync.reset();
                return;
            }
            cocos2d::CCDirector::sharedDirector()->pushScene(scene);

            // Close MultiplayerPopup if open
            if (MultiplayerPopup::s_instance) {
                MultiplayerPopup::s_instance->forceClose();
            }

            log::info("RemoteActionHandler: Pushed editor scene; pending sync will apply in init() (hasPending={})",
                m_pendingSync.has_value());
            return;
        }

        // Guard against a half-initialized editor: if init() hasn't finished
        // wiring up m_editorUI / m_objects / m_objectLayer, defer to
        // applyPendingSync() so we don't spawn objects into a broken editor.
        if (!editor->m_editorUI || !editor->m_objectLayer) {
            log::warn("RemoteActionHandler: Editor found but not fully initialized (editorUI={} objectLayer={}) — deferring as pending sync",
                static_cast<void*>(editor->m_editorUI), static_cast<void*>(editor->m_objectLayer));
            m_pendingSync = PendingSync { playerId, objectsString, uuids, settings, locks };
            return;
        }

        log::info("RemoteActionHandler: Editor ready (m_objects count before sync = {})",
            editor->m_objects ? editor->m_objects->count() : 0);

        m_pendingSync.reset();
        m_processingRemote = true;

        // Deselect all selected objects first to prevent dangling pointers in EditorUI!
        if (auto* editorUI = editor->m_editorUI) {
            editorUI->deselectAll();
        }

        // isPendingSync: editor was just pushed by us above; the level string
        // was settings-only, so m_objects is empty. We must spawn the objects
        // now (not skip them) so the world actually populates and UUIDs map.
        if (!isPendingSync) {
            // Resync path: clear out any existing objects/undo history first.
            if (editor->m_objects) {
                auto copy = cocos2d::CCArray::create();
                copy->addObjectsFromArray(editor->m_objects);
                for (auto* obj : CCArrayExt<GameObject*>(copy)) {
                    editor->removeObject(obj, true);
                }
            }
            if (editor->m_undoObjects) editor->m_undoObjects->removeAllObjects();
            if (editor->m_redoObjects) editor->m_redoObjects->removeAllObjects();
        }
        clearMappings();
        m_expectedUuids.clear();

        log::info("RemoteActionHandler: Syncing level state ({} objects) from player {} (pending={})",
            uuids.size(), playerId, isPendingSync);

        // Apply level settings (colors, start mode, song, etc.). This replaces
        // the previous manual m_effectManager swap that broke object colors.
        applyLevelSettings(editor, settings);

        // Spawn all objects from the per-object save string and register the
        // host-supplied UUIDs in spawn order.
        if (!objectsString.empty()) {
            auto newObjs = createObjectsFromSaveStringRobust(editor, objectsString);
            log::info("RemoteActionHandler: Spawned {} objects from objectsString (len={})",
                newObjs.size(), objectsString.size());
            int index = 0;
            for (auto* obj : newObjs) {
                if (index < static_cast<int>(uuids.size())) {
                    registerObject(uuids[index], obj);
                    index++;
                } else {
                    registerObject(generateUUID(), obj);
                }
            }
            if (index != static_cast<int>(uuids.size())) {
                log::warn("RemoteActionHandler: object/uuid count mismatch on sync "
                          "(spawned={}, uuids={})", index, uuids.size());
            }
        } else if (!uuids.empty()) {
            log::warn("RemoteActionHandler: sync_level had {} uuids but empty objectsString — "
                      "objects cannot be spawned (host sent no object data)", uuids.size());
        }

        // Apply locks
        m_objectLocks.clear();
        for (auto const& lock : locks) {
            m_objectLocks[lock.uuid] = LockInfo { lock.playerId, lock.timeLeft };
        }

        // Force UI options update (background, ground, colors, etc.)
        editor->levelSettingsUpdated();

        geode::Notification::create("Level Synced!", geode::NotificationIcon::Success)->show();

        m_initialSyncCompleted = true;
        m_processingRemote = false;
        log::info("RemoteActionHandler: sync_level complete (final m_objects count = {})",
            editor->m_objects ? editor->m_objects->count() : 0);
    }

    void RemoteActionHandler::updateLocks(float dt) {
        for (auto it = m_objectLocks.begin(); it != m_objectLocks.end(); ) {
            it->second.timeLeft -= dt;
            if (it->second.timeLeft <= 0.f) {
                it = m_objectLocks.erase(it);
            } else {
                ++it;
            }
        }
    }

    void RemoteActionHandler::registerObject(std::string const& uuid, GameObject* obj) {
        // Clean up any existing mapping for this object (prevent orphaned UUID→object entries)
        auto existingUuidIt = m_objectToUuid.find(obj);
        if (existingUuidIt != m_objectToUuid.end()) {
            m_uuidToObject.erase(existingUuidIt->second);
        }
        // Clean up any existing mapping for this UUID (prevent orphaned object→UUID entries)
        auto existingObjIt = m_uuidToObject.find(uuid);
        if (existingObjIt != m_uuidToObject.end()) {
            m_objectToUuid.erase(existingObjIt->second);
        }
        m_uuidToObject[uuid] = obj;
        m_objectToUuid[obj] = uuid;
    }

    void RemoteActionHandler::unregisterObject(std::string const& uuid) {
        auto it = m_uuidToObject.find(uuid);
        if (it != m_uuidToObject.end()) {
            GameObject* obj = it->second;
            m_objectToUuid.erase(obj);
            m_uuidToObject.erase(it);
        }
    }

    void RemoteActionHandler::pruneObjectFromHistory(LevelEditorLayer* editor, GameObject* obj) {
        if (!editor || !obj) return;

        auto pruneList = [](cocos2d::CCArray* list, GameObject* target) {
            if (!list) return;
            std::vector<cocos2d::CCObject*> toRemove;
            for (auto* itemObj : geode::cocos::CCArrayExt<cocos2d::CCObject*>(list)) {
                if (!itemObj) continue;
                auto* item = static_cast<UndoObject*>(itemObj);
                
                // Check m_objects array
                if (item->m_objects) {
                    if (item->m_objects->containsObject(target)) {
                        item->m_objects->removeObject(target);
                    }
                    if (item->m_objects->count() == 0) {
                        toRemove.push_back(item);
                        continue;
                    }
                }
                
                // Check m_objectCopy safely (m_objectCopy is already GameObjectCopy* in bindings)
                if (item->m_objectCopy && item->m_objectCopy->m_object && item->m_objectCopy->m_object == target) {
                    toRemove.push_back(item);
                }
            }
            for (auto* item : toRemove) {
                list->removeObject(item);
            }
        };

        pruneList(editor->m_undoObjects, obj);
        pruneList(editor->m_redoObjects, obj);
    }

    GameObject* RemoteActionHandler::getObjectByUUID(std::string const& uuid) const {
        auto it = m_uuidToObject.find(uuid);
        if (it != m_uuidToObject.end()) {
            auto* obj = it->second;
            if (auto* editor = LevelEditorLayer::get()) {
                if (editor->m_objects && editor->m_objects->containsObject(obj)) {
                    return obj;
                }
            }
        }
        return nullptr;
    }

    std::string RemoteActionHandler::getUUIDForObject(GameObject* obj) const {
        if (!obj) return "";
        auto it = m_objectToUuid.find(obj);
        return it != m_objectToUuid.end() ? it->second : "";
    }

    std::string RemoteActionHandler::getOrCreateUUID(GameObject* obj) {
        if (!obj) return "";
        auto it = m_objectToUuid.find(obj);
        if (it != m_objectToUuid.end()) {
            return it->second;
        }
        auto uuid = generateUUID();
        registerObject(uuid, obj);
        return uuid;
    }

    std::string RemoteActionHandler::generateUUID() {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_int_distribution<int> dist(0, 0xFFFF);

        int playerId = SessionManager::get().getLocalPlayerId();
        int counter = s_uuidCounter++;
        int random = dist(rng);

        std::ostringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(4) << playerId << "-"
           << std::setw(8) << counter << "-"
           << std::setw(4) << random;

        return ss.str();
    }

    void RemoteActionHandler::clearMappings() {
        m_uuidToObject.clear();
        m_objectToUuid.clear();
        m_objectLocks.clear();
        m_preSelectSaveStrings.clear();
        m_pendingPlacements.clear();
        m_playtestQueue.clear();
        m_initialSyncCompleted = false;
        // NOTE: s_uuidCounter is intentionally NOT reset here. Resetting it on
        // every editor open made UUIDs collide across reconnects/sessions
        // (playerId+counter pairs could repeat). It is process-wide and must
        // keep growing for the lifetime of the game.
    }

    void RemoteActionHandler::queueObjectForPlacement(std::string const& uuid, GameObject* obj) {
        if (!obj || uuid.empty()) return;
        m_pendingPlacements.push_back(PendingPlacement { uuid, geode::Ref<GameObject>(obj) });
    }

    bool RemoteActionHandler::isObjectPendingPlacement(GameObject* obj) const {
        if (!obj) return false;
        for (auto const& p : m_pendingPlacements) {
            if (p.obj == obj) return true;
        }
        return false;
    }

    void RemoteActionHandler::flushPlaytestQueue() {
        if (m_playtestQueue.empty()) return;
        
        log::info("RemoteActionHandler: Flushing {} queued playtest actions", m_playtestQueue.size());
        
        // Take ownership of the queue so we can re-enter handlers safely
        auto queueCopy = std::move(m_playtestQueue);
        m_playtestQueue.clear();
        
        for (auto& qa : queueCopy) {
            switch (qa.type) {
                case QueuedAction::Type::Place:     handleRemotePlaceObjects(qa.playerId, qa.placeObjects); break;
                case QueuedAction::Type::Delete:    handleRemoteDeleteObjects(qa.playerId, qa.deleteUuids); break;
                case QueuedAction::Type::Move:      handleRemoteMoveObjects(qa.playerId, qa.moveData); break;
                case QueuedAction::Type::Transform: handleRemoteTransformObjects(qa.playerId, qa.transformData); break;
                case QueuedAction::Type::Reconcile: handleRemoteReconcileObjects(qa.playerId, qa.reconcileData); break;
                case QueuedAction::Type::Update:    handleRemoteUpdateObjects(qa.playerId, qa.updateObjects); break;
            }
        }
    }

    void RemoteActionHandler::flushPendingPlacements() {
        if (m_pendingPlacements.empty()) return;

        auto* editor = getEditorLayer();
        if (!editor || !editor->m_objects) {
            m_pendingPlacements.clear();
            return;
        }

        std::vector<ActionSerializer::ObjectData> objects;
        objects.reserve(m_pendingPlacements.size());
        for (auto& p : m_pendingPlacements) {
            // The object may have been deleted between queue and flush.
            if (!p.obj || !editor->m_objects->containsObject(p.obj)) continue;
            objects.push_back(ActionSerializer::extractObjectData(p.obj, p.uuid));
            
            // Clear any relative deltas queued into the MessageBatcher during
            // this same frame (e.g. from the 'Copy + Paste' button's auto-offset).
            // Since extractObjectData captures the absolute POST-offset position,
            // sending a MoveBatch delta later would cause a double-move.
            MessageBatcher::get().removePending(p.uuid);
            
            // If this is a blue teleport portal, also send its paired orange portal
            if (auto* tpPortal = typeinfo_cast<TeleportPortalObject*>(static_cast<GameObject*>(p.obj))) {
                if (!tpPortal->m_isYellowPortal && tpPortal->m_orangePortal) {
                    auto* orange = tpPortal->m_orangePortal;
                    auto orangeUuid = getOrCreateUUID(orange);
                    objects.push_back(ActionSerializer::extractObjectData(orange, orangeUuid));
                    MessageBatcher::get().removePending(orangeUuid);
                }
            }
        }
        m_pendingPlacements.clear();

        if (!objects.empty() && !m_processingRemote) {
            auto data = proto::serializePlaceObjects(objects);
            P2PManager::get().send(data, ChannelType::Reliable);
            log::debug("RemoteActionHandler: Flushed batched placement of {} objects", objects.size());
        }
    }

    bool RemoteActionHandler::isInitialSyncCompleted() const {
        if (SessionManager::get().getRole() == SessionManager::Role::Host) {
            return true;
        }
        return m_initialSyncCompleted;
    }

    void RemoteActionHandler::downloadSongFinished(int id) {
        auto* editor = getEditorLayer();
        if (editor && editor->m_level && editor->m_level->m_songID == id) {
            GameManager::get()->fadeInMusic(editor->m_level->getAudioFileName());
            geode::Notification::create("Song downloaded! Playing now.", geode::NotificationIcon::Success)->show();
        }
    }

    void RemoteActionHandler::downloadSongFailed(int id, GJSongError error) {
        geode::Notification::create("Failed to download custom song", geode::NotificationIcon::Error)->show();
    }

    void RemoteActionHandler::applyLevelSettings(LevelEditorLayer* editor, ActionSerializer::LevelSettingsData const& settings) {
        if (!editor) return;

        // Parse the incoming LevelSettingsObject string and copy its fields
        // onto the editor's current settings. We previously swapped
        // m_effectManager by hand here, but that left already-spawned objects
        // referencing a stale/empty color manager, producing the
        // "color not seeable" / glitched-color bug. Instead we copy every
        // settings field and let GD's updateOptions()/levelSettingsUpdated()
        // rebuild the color state correctly.
        if (!settings.saveString.empty() && editor->m_levelSettings) {
            auto* newSettings = LevelSettingsObject::objectFromString(settings.saveString);
            if (newSettings) {
                editor->m_levelSettings->m_startMode = newSettings->m_startMode;
                editor->m_levelSettings->m_startSpeed = newSettings->m_startSpeed;
                editor->m_levelSettings->m_startMini = newSettings->m_startMini;
                editor->m_levelSettings->m_startDual = newSettings->m_startDual;
                editor->m_levelSettings->m_twoPlayerMode = newSettings->m_twoPlayerMode;
                editor->m_levelSettings->m_isFlipped = newSettings->m_isFlipped;
                editor->m_levelSettings->m_songOffset = newSettings->m_songOffset;

                // Deep-copy colors without replacing EffectManager or ColorActions
                if (auto* newEffectMgr = newSettings->m_effectManager) {
                    if (auto* oldEffectMgr = editor->m_levelSettings->m_effectManager) {
                        if (auto* newDict = newEffectMgr->m_colorActionDict) {
                            if (auto* oldDict = oldEffectMgr->m_colorActionDict) {
                                auto copyColor = [](ColorAction* oldAction, ColorAction* newAction) {
                                    if (oldAction && newAction) {
                                        oldAction->m_color = newAction->m_color;
                                        oldAction->m_fromColor = newAction->m_fromColor;
                                        oldAction->m_toColor = newAction->m_toColor;
                                        oldAction->m_duration = newAction->m_duration;
                                        oldAction->m_blending = newAction->m_blending;
                                        oldAction->m_playerColor = newAction->m_playerColor;
                                        oldAction->m_fromOpacity = newAction->m_fromOpacity;
                                        oldAction->m_toOpacity = newAction->m_toOpacity;
                                        oldAction->m_copyHSV = newAction->m_copyHSV;
                                        oldAction->m_copyID = newAction->m_copyID;
                                        oldAction->m_copyOpacity = newAction->m_copyOpacity;
                                        oldAction->m_copyColorCalculated = newAction->m_copyColorCalculated;
                                        oldAction->m_colorID = newAction->m_colorID;
                                        oldAction->m_copyColorLoop = newAction->m_copyColorLoop;
                                        oldAction->m_legacyHSV = newAction->m_legacyHSV;
                                    }
                                };
                                
                                // Copy the standard colors array
                                for (size_t i = 0; i < newEffectMgr->m_colorActionVector.size(); i++) {
                                    if (i < oldEffectMgr->m_colorActionVector.size()) {
                                        auto* newAction = newEffectMgr->m_colorActionVector[i];
                                        auto* oldAction = oldEffectMgr->m_colorActionVector[i];
                                        copyColor(oldAction, newAction);
                                        if (oldAction) {
                                            oldEffectMgr->updateColorAction(oldAction);
                                            oldEffectMgr->colorActionChanged(oldAction);
                                        }
                                    }
                                }

                                log::info("applyLevelSettings: newDict count={}, oldDict count={}", newDict->count(), oldDict->count());
                                auto* keys = newDict->allKeys();
                                if (keys) {
                                    for (int i = 0; i < keys->count(); i++) {
                                        auto* keyObj = keys->objectAtIndex(i);
                                        intptr_t k = 0;
                                        if (auto* strKey = typeinfo_cast<cocos2d::CCString*>(keyObj)) {
                                            k = std::stoi(strKey->getCString());
                                            log::info("Processing key {} (from string: {})", k, strKey->getCString());
                                        } else if (auto* intKey = typeinfo_cast<cocos2d::CCInteger*>(keyObj)) {
                                            k = intKey->getValue();
                                            log::info("Processing key {} (from int)", k);
                                        }
                                        
                                        auto* newAction = static_cast<ColorAction*>(newDict->objectForKey(k));
                                        if (!newAction && keyObj) {
                                            if (auto* strKey = typeinfo_cast<cocos2d::CCString*>(keyObj)) {
                                                newAction = static_cast<ColorAction*>(newDict->objectForKey(strKey->getCString()));
                                            }
                                        }
                                        
                                        auto* oldAction = static_cast<ColorAction*>(oldDict->objectForKey(k));
                                        
                                        if (!oldAction) {
                                            oldAction = ColorAction::create();
                                            oldDict->setObject(oldAction, k);
                                        }
                                        if (newAction && oldAction) {
                                            copyColor(oldAction, newAction);
                                            
                                            // Tell effect manager about the change
                                            oldEffectMgr->updateColorAction(oldAction);
                                            oldEffectMgr->colorActionChanged(oldAction);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                editor->m_updateColorSprites = true;
                editor->updateOptions();
            }
        }

        if (editor->m_level) {
            bool songChanged = (editor->m_level->m_songID != settings.songID
                || editor->m_level->m_audioTrack != settings.audioTrack);
            editor->m_level->m_audioTrack = settings.audioTrack;
            editor->m_level->m_songID = settings.songID;
            editor->m_level->m_levelLength = settings.levelLength;

            if (songChanged) {
                if (settings.songID > 0) {
                    if (MusicDownloadManager::sharedState()->isSongDownloaded(settings.songID)) {
                        GameManager::get()->fadeInMusic(editor->m_level->getAudioFileName());
                    } else {
                        geode::Notification::create("Custom song not downloaded locally.", geode::NotificationIcon::Info)->show();
                    }
                } else {
                    GameManager::get()->fadeInMusic(editor->m_level->getAudioFileName());
                }
            }
        }
    }

    void RemoteActionHandler::handleRemoteUpdateSettings(int playerId, ActionSerializer::LevelSettingsData const& settings) {
        auto* editor = getEditorLayer();
        if (!editor) return;

        m_processingRemote = true;

        log::info("RemoteActionHandler: Updating level settings from player {}", playerId);

        applyLevelSettings(editor, settings);

        // Force UI options update (e.g. background, ground, colors)
        editor->levelSettingsUpdated();

        m_processingRemote = false;
    }

} // namespace mpedit
