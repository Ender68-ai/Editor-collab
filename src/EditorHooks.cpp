#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/TeleportPortalObject.hpp>

#include "ui/ui.hpp"
#include "SessionManager.hpp"
#include "P2PManager.hpp"
#include "BinaryProtocol.hpp"
#include "MessageBatcher.hpp"
#include "ActionSerializer.hpp"
#include "RemoteActionHandler.hpp"
#include "ui/MultiplayerPopup.hpp"
#include "ui/SessionStatusNode.hpp"
#include "ui/CursorNode.hpp"
#include "ui/UpdateHelperNode.hpp"

using namespace geode::prelude;
using namespace mpedit;

namespace {
    int s_selectedObjectID = 1;
    bool s_inTransformSync = false;
    cocos2d::CCPoint s_lastTouchPos = {0.f, 0.f};
    bool s_isTouching = false;
}

// ============================================================
// EditorPauseLayer — Add "Multiplayer" button to pause menu
// ============================================================

class $modify(MPEditorPauseLayer, EditorPauseLayer) {
    bool init(LevelEditorLayer* editor) {
        if (!EditorPauseLayer::init(editor)) return false;

        // Create the multiplayer button
        auto* btnSprite = ButtonSprite::create(
            "Multiplayer Edit", 90, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.45f
        );
        auto* btn = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(MPEditorPauseLayer::onMultiplayer)
        );
        btn->setID("multiplayer-button"_spr);

        // Find the center button menu
        CCMenu* targetMenu = typeinfo_cast<CCMenu*>(this->getChildByID("center-button-menu"));
        
        if (!targetMenu) {
            // Fallback: look through all menus to find one with the most buttons (likely the center one)
            for (CCNode* child : this->getChildrenExt()) {
                if (auto* menu = typeinfo_cast<CCMenu*>(child)) {
                    if (menu->getChildrenCount() >= 4) {
                        targetMenu = menu;
                        break;
                    }
                }
            }
        }

        if (targetMenu) {
            targetMenu->addChild(btn);
            targetMenu->updateLayout();
        } else {
            // Fallback: create our own menu
            auto* fallbackMenu = CCMenu::create();
            fallbackMenu->setID("multiplayer-menu"_spr);
            fallbackMenu->setPosition({0, 0});
            
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            btn->setPosition({winSize.width / 2.f, 40.f}); // Bottom center
            fallbackMenu->addChild(btn);
            this->addChild(fallbackMenu, 10);
        }

        auto& session = SessionManager::get();
        if (session.isInSession()) {
            auto disableBtn = [this](const char* id) {
                if (auto* btn = typeinfo_cast<CCMenuItemSpriteExtra*>(this->getChildByIDRecursive(id))) {
                    btn->setEnabled(false);
                    if (auto* sprite = typeinfo_cast<cocos2d::CCSprite*>(btn->getNormalImage())) {
                        sprite->setColor({100, 100, 100});
                    }
                }
            };

            auto disableBtnByText = [this](bool isSavePlay, bool isSaveExit) {
                std::function<CCMenuItemSpriteExtra*(CCNode*)> findBtn = [&](CCNode* node) -> CCMenuItemSpriteExtra* {
                    if (!node) return nullptr;
                    if (auto* item = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
                        if (auto* normal = item->getNormalImage()) {
                            std::function<CCLabelBMFont*(CCNode*)> findLabel = [&](CCNode* n) -> CCLabelBMFont* {
                                if (auto* lbl = typeinfo_cast<CCLabelBMFont*>(n)) return lbl;
                                if (n->getChildren()) {
                                    for (auto* c : CCArrayExt<CCNode*>(n->getChildren())) {
                                        if (auto* l = findLabel(c)) return l;
                                    }
                                }
                                return nullptr;
                            };
                            if (auto* label = findLabel(normal)) {
                                std::string s = label->getString();
                                bool hasSave = s.find("Save") != std::string::npos;
                                bool hasPlay = s.find("Play") != std::string::npos;
                                bool hasExit = s.find("Exit") != std::string::npos;
                                
                                if (isSavePlay && hasSave && hasPlay) return item;
                                if (isSaveExit && hasSave && hasExit) return item;
                                if (!isSavePlay && !isSaveExit && hasSave && !hasPlay && !hasExit) return item;
                            }
                        }
                    }
                    if (node->getChildren()) {
                        for (auto* c : CCArrayExt<CCNode*>(node->getChildren())) {
                            if (auto* b = findBtn(c)) return b;
                        }
                    }
                    return nullptr;
                };

                if (auto* btn = findBtn(this)) {
                    btn->setEnabled(false);
                    std::function<void(CCNode*)> grayOut = [&](CCNode* n) {
                        if (auto* rgba = typeinfo_cast<cocos2d::CCNodeRGBA*>(n)) {
                            rgba->setColor({100, 100, 100});
                        }
                        if (n->getChildren()) {
                            for (auto* c : CCArrayExt<CCNode*>(n->getChildren())) {
                                grayOut(c);
                            }
                        }
                    };
                    grayOut(btn->getNormalImage());
                }
            };

            if (session.getRole() == SessionManager::Role::Host) {
                disableBtn("save-and-play-button");
                disableBtnByText(true, false);
            } else if (session.getRole() == SessionManager::Role::Client) {
                disableBtn("save-button");
                disableBtn("save-and-play-button");
                disableBtn("save-and-exit-button");
                
                disableBtnByText(false, false);
                disableBtnByText(true, false);
                disableBtnByText(false, true);
            }
        }

        return true;
    }

    void onMultiplayer(CCObject*) {
        MultiplayerPopup::create()->show();
    }

    void onSave(CCObject* sender) {
        if (SessionManager::get().isInSession() && SessionManager::get().getRole() == SessionManager::Role::Client) {
            Notification::create("Guests cannot save levels", NotificationIcon::Warning)->show();
            return;
        }
        EditorPauseLayer::onSave(sender);
    }

    void onSaveAndPlay(CCObject* sender) {
        if (SessionManager::get().isInSession()) {
            Notification::create("Cannot Save & Play in multiplayer", NotificationIcon::Warning)->show();
            return;
        }
        EditorPauseLayer::onSaveAndPlay(sender);
    }

    void onSaveAndExit(CCObject* sender) {
        auto& session = SessionManager::get();
        if (session.isInSession()) {
            if (session.getRole() == SessionManager::Role::Client) {
                Notification::create("Guests cannot save levels", NotificationIcon::Warning)->show();
                return;
            }
            session.leaveSession();
        }
        EditorPauseLayer::onSaveAndExit(sender);
    }

    void onExitEditor(CCObject* sender) {
        auto& session = SessionManager::get();
        if (session.isInSession()) {
            session.leaveSession();
        }
        EditorPauseLayer::onExitEditor(sender);
    }
};

// ============================================================
// LevelBrowserLayer — Add "Multiplayer" button to My Levels page
// ============================================================

class $modify(MPLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* object) {
        if (!LevelBrowserLayer::init(object)) return false;

        if (object->m_searchType != SearchType::MyLevels) return true;

        auto* btnSprite = ButtonSprite::create(
            "Multiplayer Edit", 90, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.45f
        );
        auto* btn = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(MPLevelBrowserLayer::onMultiplayer)
        );
        btn->setID("multiplayer-button"_spr);

        // Create a menu at the bottom center, underneath the level list
        auto* centerMenu = CCMenu::create();
        centerMenu->setID("multiplayer-menu"_spr);
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        // Place it horizontally centered and near the bottom edge
        centerMenu->setPosition({winSize.width / 2.f, 35.f});
        
        btn->setPosition({0, 0});
        centerMenu->addChild(btn);
        this->addChild(centerMenu, 10);

        return true;
    }

    void onMultiplayer(CCObject*) {
        MultiplayerPopup::create()->show();
    }
};

// ============================================================
// LevelEditorLayer — Hook editor lifecycle for session management
// ============================================================

namespace {
    void sendChunkedSync(LevelEditorLayer* editor, int targetPlayerId) {
        auto& handler = RemoteActionHandler::get();

        constexpr size_t BATCH_SIZE = 200;
        struct ChunkData {
            std::string objectsString;
            std::vector<std::string> uuids;
        };
        std::vector<ChunkData> chunks;
        ChunkData currentChunk;

        if (editor->m_objects) {
            for (auto* obj : CCArrayExt<GameObject*>(editor->m_objects)) {
                if (!obj) continue;
                if (obj->m_objectID == 31) continue; // Skip start pos
                auto uuid = handler.getUUIDForObject(obj);
                if (uuid.empty()) {
                    uuid = RemoteActionHandler::generateUUID();
                    handler.registerObject(uuid, obj);
                }
                currentChunk.uuids.push_back(uuid);
                currentChunk.objectsString += std::string(obj->getSaveString(editor)) + ";";
                
                if (currentChunk.uuids.size() >= BATCH_SIZE) {
                    chunks.push_back(std::move(currentChunk));
                    currentChunk = ChunkData();
                }
            }
        }
        if (!currentChunk.uuids.empty() || chunks.empty()) {
            if (chunks.empty() && currentChunk.uuids.empty()) {
                chunks.push_back(ChunkData()); // Ensure at least 1 chunk for empty level
            } else {
                chunks.push_back(std::move(currentChunk));
            }
        }

        // Settings only: LevelSettingsObject::getSaveString() uses the ','
        // separator and carries colors (EffectManager), start mode, song, etc.
        ActionSerializer::LevelSettingsData settings;
        if (editor->m_levelSettings) {
            settings.saveString = editor->m_levelSettings->getSaveString();
        }
        if (editor->m_level) {
            settings.audioTrack = editor->m_level->m_audioTrack;
            settings.songID = editor->m_level->m_songID;
            settings.levelLength = editor->m_level->m_levelLength;
        }

        uint32_t totalChunks = static_cast<uint32_t>(chunks.size());
        uint32_t totalObjects = 0;
        for (auto const& chunk : chunks) {
            totalObjects += chunk.uuids.size();
        }

        // 4. Send SyncLevelStart
        auto startMsg = proto::serializeSyncLevelStart(totalChunks, totalObjects, settings);
        P2PManager::get().sendTo(targetPlayerId, startMsg, ChannelType::Reliable);

        // 5. Send chunks sequentially
        for (uint32_t i = 0; i < totalChunks; ++i) {
            auto chunkMsg = proto::serializeSyncLevelChunk(
                i, 
                reinterpret_cast<const uint8_t*>(chunks[i].objectsString.data()), 
                chunks[i].objectsString.size(),
                chunks[i].uuids
            );
            P2PManager::get().sendTo(targetPlayerId, chunkMsg, ChannelType::Reliable);
        }

        // 6. Gather locks
        std::vector<ActionSerializer::LockData> locks;
        for (auto const& [uuid, lockInfo] : handler.getObjectLocks()) {
            locks.push_back({uuid, lockInfo.playerId, lockInfo.timeLeft});
        }

        // 7. Send SyncLevelEnd
        auto endMsg = proto::serializeSyncLevelEnd(locks);
        P2PManager::get().sendTo(targetPlayerId, endMsg, ChannelType::Reliable);
    }

    // Registers UUIDs onto the editor's currently-spawned objects, aligned by
    // index with the provided uuids list. Missing/extra objects get fresh
    // generated UUIDs. Returns once registration is consistent.
    void registerObjectsWithUuids(LevelEditorLayer* editor,
                                  std::vector<std::string> const& uuids) {
        if (!editor || !editor->m_objects) return;
        auto& handler = RemoteActionHandler::get();
        int index = 0;
        for (auto* obj : CCArrayExt<GameObject*>(editor->m_objects)) {
            if (!obj) continue;
            if (index < static_cast<int>(uuids.size()) && !uuids[index].empty()) {
                handler.registerObject(uuids[index], obj);
            } else {
                // Object count mismatch with host (rare): assign a fresh UUID.
                if (handler.getUUIDForObject(obj).empty()) {
                    handler.registerObject(RemoteActionHandler::generateUUID(), obj);
                }
            }
            index++;
        }
        if (index != static_cast<int>(uuids.size())) {
            log::warn("EditorHooks: object/uuid count mismatch on sync "
                      "(objects={}, uuids={})", index, uuids.size());
        }
    }
}

class $modify(MPLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        float m_cursorSendTimer = 0.f;
        bool m_sessionActive = false;
        bool m_inUndoRedo = false;
        cocos2d::CCPoint m_lastSentLevelPos = {0.f, 0.f};

        ~Fields() {
            auto& session = SessionManager::get();
            if (session.isInSession()) {
                session.leaveSession();
                log::info("EditorHooks: Left session automatically on editor destructor (Fields)");
            }
            session.clearCallbacks();
        }
    };

    void levelSettingsUpdated() {
        LevelEditorLayer::levelSettingsUpdated();

        auto& handler = RemoteActionHandler::get();
        if (handler.isProcessingRemote() || !handler.isInitialSyncCompleted()) return;

        auto& session = SessionManager::get();
        if (session.isInSession()) {
            ActionSerializer::LevelSettingsData settings;
            if (this->m_levelSettings) {
                settings.saveString = this->m_levelSettings->getSaveString();
            }
            if (this->m_level) {
                settings.audioTrack = this->m_level->m_audioTrack;
                settings.songID = this->m_level->m_songID;
                settings.levelLength = this->m_level->m_levelLength;
            }
            auto data = proto::serializeUpdateSettings(settings);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
            log::info("EditorHooks: Broadcasted update_settings");
        }
    }

    bool init(GJGameLevel* level, bool unk) {
        if (!LevelEditorLayer::init(level, unk)) return false;

        // Force construction of Fields immediately so its destructor runs reliably
        m_fields->m_sessionActive = SessionManager::get().isInSession();

        // Set up the remote action handler for this editor session
        auto& handler = RemoteActionHandler::get();
        handler.clearMappings();

        // Register callback to assign UUIDs to all existing objects as soon as host session starts
        SessionManager::get().onSessionStarted([this]() {
            auto& session = SessionManager::get();
            if (session.getRole() == SessionManager::Role::Host) {
                if (this->m_objects) {
                    auto& handler = RemoteActionHandler::get();
                    for (auto* obj : CCArrayExt<GameObject*>(this->m_objects)) {
                        if (handler.getUUIDForObject(obj).empty()) {
                            auto uuid = RemoteActionHandler::generateUUID();
                            handler.registerObject(uuid, obj);
                        }
                    }
                }
            }
        });

        auto& session = SessionManager::get();
        if (session.isInSession()) {
            // If a sync_level arrived before the editor existed, we queued it as
            // a pending sync (and stored its uuids in m_expectedUuids). The
            // level string we pushed was settings-only, so m_objects is EMPTY
            // right now — the objects live in objectsString and will be spawned
            // by applyPendingSync() below. So do NOT try to register expected
            // UUIDs onto m_objects here (it's empty; that just logged a
            // spurious "count mismatch"). Let the pending sync own spawning.
            bool hasPending = handler.hasPendingSync();

            if (!hasPending) {
                auto const& expected = handler.getExpectedUuids();
                if (!expected.empty()) {
                    if (this->m_objects) {
                        registerObjectsWithUuids(this, expected);
                    }
                    handler.clearExpectedUuids();
                } else if (this->m_objects) {
                    // No expected UUIDs (e.g. host just started): just ensure every
                    // object has a UUID so future edits can be tracked.
                    for (auto* obj : CCArrayExt<GameObject*>(this->m_objects)) {
                        if (obj && handler.getUUIDForObject(obj).empty()) {
                            handler.registerObject(RemoteActionHandler::generateUUID(), obj);
                        }
                    }
                }
            } else {
                // Pending sync will create objects from objectsString and
                // register host-supplied UUIDs in spawn order. Clear the
                // expected-uuid staging list; it's redundant now.
                handler.clearExpectedUuids();
            }

            // Unconditionally mark initial sync as completed for the client
            handler.setInitialSyncCompleted(true);

            // Host: immediately broadcast the level state to every already
            // connected player (e.g. client joined while host editor was open).
            if (session.getRole() == SessionManager::Role::Host) {
                for (auto const& player : session.getPlayers()) {
                    if (player.id != session.getLocalPlayerId()) {
                        sendChunkedSync(this, player.id);
                        log::info("EditorHooks: Sent chunked sync_level to existing player {}", player.id);
                    }
                }
            }
        }

        // Host: broadcast the level state to any NEW player that joins later.
        // Registered here (inside init) so it captures `this` editor instance.
        SessionManager::get().onPlayerJoined([this](PlayerInfo const& info) {
            auto& session = SessionManager::get();
            if (session.getRole() == SessionManager::Role::Host && info.id != session.getLocalPlayerId()) {
                sendChunkedSync(this, info.id);
                log::info("EditorHooks: Sent chunked sync_level to new player {}", info.id);
            }
        });

        // Apply any pending sync_level packet that arrived early (client path):
        // the editor was pushed by handleRemoteSyncLevel, then init() ran and
        // registered UUIDs above; now finish applying settings + objects.
        //
        // NOTE: scene() runs init() synchronously, so by the time we reach here
        // the editor is constructed but NOT yet added to the scene graph. That
        // means LevelEditorLayer::get() / scene-walk lookups would miss it and
        // send handleRemoteSyncLevel back into the "no editor" branch (infinite
        // recursion). We hand it the editor explicitly via the init-bridge so
        // it mutates `this` directly.
        if (handler.hasPendingSync()) {
            handler.setEditorForInit(this);
            handler.applyPendingSync();
            // applyPendingSync clears the override on return; assert defensively.
            handler.setEditorForInit(nullptr);
        }

        // Add a helper node to handle network updates safely without member function pointer layout mismatch
        auto* helper = UpdateHelperNode::create([this](float dt) {
            this->networkUpdate(dt);
        }, 0.05f);
        if (helper) {
            helper->setID("network-update-helper"_spr);
            this->addChild(helper);
        }

        // Add session status indicator
        auto* status = SessionStatusNode::create();
        status->setID("session-status"_spr);
        this->addChild(status, 1000);

        // Add cursor node to the object layer so it scales/pans correctly
        auto* cursorNode = CursorNode::create();
        cursorNode->setID("cursor-node"_spr);
        this->m_objectLayer->addChild(cursorNode, 999);

        return true;
    }

    void onExit() {
        LevelEditorLayer::onExit();
        
        auto& session = SessionManager::get();
        if (session.isInSession()) {
            session.leaveSession();
            log::info("EditorHooks: Left session automatically on editor exit");
        }
        session.clearCallbacks();
    }


    // Intercept object creation — UUID assignment and sync is handled by addToSection hook
    GameObject* createObject(int objectID, cocos2d::CCPoint position, bool noUndo) {
        auto* obj = LevelEditorLayer::createObject(objectID, position, noUndo);
        // addToSection is called internally by LevelEditorLayer::createObject,
        // which handles UUID assignment and placement sync.
        // We do NOT sync here to avoid sending duplicate placement messages.
        return obj;
    }

    // Intercept object removal to sync deletion
    void removeObject(GameObject* obj, bool undo) {
        if (!obj) {
            LevelEditorLayer::removeObject(obj, undo);
            return;
        }

        // Prevent premature deallocation during cleanup — the object may only be kept alive
        // by CCArrays (e.g., m_selectedObjects, m_touchingRings) that we're removing from.
        obj->retain();

        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        {
            // Clean up teleport portal pairs to prevent Use-After-Free crashes from dangling pointers
            if (auto* portal = typeinfo_cast<TeleportPortalObject*>(obj)) {
                if (portal->m_orangePortal) {
                    if (portal->m_orangePortal->m_orangePortal == portal) {
                        portal->m_orangePortal->m_orangePortal = nullptr;
                    }
                    portal->m_orangePortal = nullptr;
                }
            }

            // Clean up game state last activated portal references to prevent Use-After-Free crashes during playtesting/editing
            if (m_gameState.m_lastActivatedPortal1 == obj) {
                m_gameState.m_lastActivatedPortal1 = nullptr;
            }
            if (m_gameState.m_lastActivatedPortal2 == obj) {
                m_gameState.m_lastActivatedPortal2 = nullptr;
            }
            if (this->m_player1) {
                if (this->m_player1->m_lastActivatedPortal == obj) {
                    this->m_player1->m_lastActivatedPortal = nullptr;
                }
                if (this->m_player1->m_touchingRings && this->m_player1->m_touchingRings->containsObject(obj)) {
                    this->m_player1->m_touchingRings->removeObject(obj);
                }
            }
            if (this->m_player2) {
                if (this->m_player2->m_lastActivatedPortal == obj) {
                    this->m_player2->m_lastActivatedPortal = nullptr;
                }
                if (this->m_player2->m_touchingRings && this->m_player2->m_touchingRings->containsObject(obj)) {
                    this->m_player2->m_touchingRings->removeObject(obj);
                }
            }
            if (this->m_endPortal == obj) {
                this->m_endPortal = nullptr;
            }
            if (this->m_player1CollisionBlock == obj) {
                this->m_player1CollisionBlock = nullptr;
            }
            if (this->m_player2CollisionBlock == obj) {
                this->m_player2CollisionBlock = nullptr;
            }
            if (this->m_startPosObject == obj) {
                this->m_startPosObject = nullptr;
            }
            if (this->m_copyStateObject == obj) {
                this->m_copyStateObject = nullptr;
            }
            if (this->m_editorUI) {
                if (this->m_editorUI->m_selectedObject == obj) {
                    this->m_editorUI->m_selectedObject = nullptr;
                }
                if (this->m_editorUI->m_snapObject == obj) {
                    this->m_editorUI->m_snapObject = nullptr;
                }
                if (this->m_editorUI->m_selectedObjects && this->m_editorUI->m_selectedObjects->containsObject(obj)) {
                    this->m_editorUI->m_selectedObjects->removeObject(obj);
                }
            }
        }

        // During undo/redo, handleAction already handles sync — skip here to avoid double-sync
        bool inUndoRedo = m_fields->m_inUndoRedo;
        bool shouldBroadcastDelete = session.isInSession()
            && !handler.isProcessingRemote() && !inUndoRedo && obj;

        if (shouldBroadcastDelete) {
            auto uuid = handler.getUUIDForObject(obj);
            if (!uuid.empty()) {
                // Block deletion of objects locked by another player.
                auto const& locks = handler.getObjectLocks();
                auto it = locks.find(uuid);
                if (it != locks.end() && it->second.playerId != session.getLocalPlayerId()) {
                    log::info("EditorHooks: Blocked removal of locked object (uuid={})", uuid);
                    obj->release();
                    return;
                }
                // Broadcast the deletion and unregister in one place. Callers
                // like onDeleteSelected unregister first, so this is a no-op for
                // them; for other deletion paths this is the single broadcast.
                auto data = proto::serializeDeleteObjects({uuid});
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
                handler.unregisterObject(uuid);
                log::debug("EditorHooks: Deleted object (uuid={})", uuid);
            }
        }

        // Always clean up dangling references in RemoteActionHandler maps so a
        // later remote action can never resolve a freed GameObject pointer.
        if (obj) {
            auto uuid = handler.getUUIDForObject(obj);
            if (!uuid.empty()) {
                handler.unregisterObject(uuid);
            }
            handler.getTrackedSelections().erase(obj);
        }

        LevelEditorLayer::removeObject(obj, undo);

        obj->release();
    }

    // Hook handleAction to block locked undo/redo actions and synchronize local history updates
    void handleAction(bool undo, cocos2d::CCArray* undoObjects) {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        if (!session.isInSession() || handler.isProcessingRemote() || !undoObjects || undoObjects->count() == 0) {
            LevelEditorLayer::handleAction(undo, undoObjects);
            return;
        }

        // 1. Gather ONLY objects affected by this specific action to prevent O(N) lag spikes
        std::unordered_set<GameObject*> affectedObjects;
        auto* lastItem = static_cast<UndoObject*>(undoObjects->lastObject());
        if (lastItem) {
            if (lastItem->m_objects) {
                for (auto* gObj : CCArrayExt<GameObject*>(lastItem->m_objects)) {
                    affectedObjects.insert(gObj);
                }
            }
            if (lastItem->m_objectCopy && lastItem->m_objectCopy->m_object) {
                affectedObjects.insert(lastItem->m_objectCopy->m_object);
            }
        }

        // 2. Verify locks for affected objects
        for (auto* gObj : affectedObjects) {
            if (!gObj) continue;
            auto uuid = handler.getUUIDForObject(gObj);
            if (!uuid.empty()) {
                auto const& locks = handler.getObjectLocks();
                auto it = locks.find(uuid);
                if (it != locks.end() && it->second.playerId != session.getLocalPlayerId()) {
                    log::info("EditorHooks: Blocked undo/redo of locked object");
                    return;
                }
            }
        }

        // 3. Record baseline state for ONLY the affected objects
        std::unordered_map<GameObject*, cocos2d::CCPoint> positionsBefore;
        std::unordered_map<GameObject*, std::string> saveStringsBefore;
        std::unordered_set<GameObject*> existedBefore;

        for (auto* obj : affectedObjects) {
            if (!obj) continue;
            if (this->m_objects && this->m_objects->containsObject(obj)) {
                existedBefore.insert(obj);
                positionsBefore[obj] = obj->getPosition();
                saveStringsBefore[obj] = obj->getSaveString(this);
            }
        }

        // 4. Execute base handleAction
        m_fields->m_inUndoRedo = true;
        LevelEditorLayer::handleAction(undo, undoObjects);

        // 5. Detect changes using existedBefore vs existedAfter matrix
        std::vector<ActionSerializer::ObjectData> placedObjects;
        std::vector<std::string> deletedUuids;
        std::vector<ActionSerializer::MoveData> movedObjects;
        std::vector<ActionSerializer::ObjectData> updatedObjects;

        for (auto* obj : affectedObjects) {
            if (!obj) continue;
            
            bool existed_before = existedBefore.find(obj) != existedBefore.end();
            bool existed_after = this->m_objects && this->m_objects->containsObject(obj);
            
            if (existed_before && !existed_after) {
                // Object was DELETED by Undo
                std::string uuid = handler.getUUIDForObject(obj);
                if (!uuid.empty()) {
                    deletedUuids.push_back(uuid);
                    handler.unregisterObject(uuid);
                }
            } 
            else if (!existed_before && existed_after) {
                // Object was PLACED by Redo
                std::string uuid = handler.getUUIDForObject(obj);
                if (uuid.empty()) {
                    uuid = RemoteActionHandler::generateUUID();
                    handler.registerObject(uuid, obj);
                }
                placedObjects.push_back(ActionSerializer::extractObjectData(obj, uuid));
            } 
            else if (existed_before && existed_after) {
                // Object was MODIFIED by Undo/Redo
                std::string uuid = handler.getUUIDForObject(obj);
                if (uuid.empty()) {
                    uuid = RemoteActionHandler::generateUUID();
                    handler.registerObject(uuid, obj);
                }
                
                std::string currentSave = obj->getSaveString(this);
                if (saveStringsBefore[obj] != currentSave) {
                    if (obj->m_objectID != 31) {
                        updatedObjects.push_back(ActionSerializer::extractObjectData(obj, uuid));
                    }
                } else {
                    cocos2d::CCPoint oldPos = positionsBefore[obj];
                    float dx = obj->getPositionX() - oldPos.x;
                    float dy = obj->getPositionY() - oldPos.y;
                    if (dx != 0.f || dy != 0.f) {
                        ActionSerializer::MoveData md;
                        md.uuid = uuid;
                        md.dx = dx;
                        md.dy = dy;
                        movedObjects.push_back(md);
                    }
                }
            }
        }

        // 6. Send updates
        if (!placedObjects.empty()) {
            auto data = proto::serializePlaceObjects(placedObjects);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
            log::info("EditorHooks: Synced redo placement of {} objects", placedObjects.size());
        }
        if (!deletedUuids.empty()) {
            auto data = proto::serializeDeleteObjects(deletedUuids);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
            log::info("EditorHooks: Synced undo deletion of {} objects", deletedUuids.size());
        }
        if (!movedObjects.empty()) {
            auto data = proto::serializeMoveObjects(movedObjects);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }
        if (!updatedObjects.empty()) {
            auto data = proto::serializeUpdateObjects(updatedObjects);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }
        
        m_fields->m_inUndoRedo = false;
    }

    void networkUpdate(float dt) {
        auto& session = SessionManager::get();
        if (!session.isInSession()) return;

        // Dispatch queued network messages
        P2PManager::get().dispatchMessages();

        auto& handler = RemoteActionHandler::get();
        handler.updateLocks(dt);
        MessageBatcher::get().update(dt);

        // Flush any batched placements (copy/paste/duplicate) as a single message.
        handler.flushPendingPlacements();

        // Send cursor position periodically
        m_fields->m_cursorSendTimer += dt;
        if (m_fields->m_cursorSendTimer >= 0.1f) {  // 10 Hz cursor updates
            m_fields->m_cursorSendTimer = 0.f;
            
            if (this->m_objectLayer) {
                cocos2d::CCPoint levelPos;
                std::string statusStr = "";

                if (this->m_playbackMode != PlaybackMode::Not && this->m_player1) {
                    levelPos = this->m_player1->getPosition();
                    
                    auto* gm = GameManager::get();
                    int iconType = 0; // Cube
                    if (this->m_player1->m_isShip) {
                        iconType = this->m_player1->m_isPlatformer ? 8 : 1; // 8 = Jetpack, 1 = Ship
                    } else if (this->m_player1->m_isBall) {
                        iconType = 2;
                    } else if (this->m_player1->m_isBird) {
                        iconType = 3;
                    } else if (this->m_player1->m_isDart) {
                        iconType = 4;
                    } else if (this->m_player1->m_isRobot) {
                        iconType = 5;
                    } else if (this->m_player1->m_isSpider) {
                        iconType = 6;
                    } else if (this->m_player1->m_isSwing) {
                        iconType = 7;
                    }

                    auto col1 = gm->colorForIdx(gm->getPlayerColor());
                    auto col2 = gm->colorForIdx(gm->getPlayerColor2());
                    bool glowEnabled = gm->getPlayerGlow();
                    auto colGlow = gm->colorForIdx(gm->getPlayerGlowColor());

                    std::stringstream ss;
                    ss << "pt:1:" 
                       << iconType << ":" 
                       << this->m_player1->getRotation() << ":" 
                       << (this->m_player1->m_isUpsideDown ? 1 : 0) << ":"
                       << gm->getPlayerFrame() << ":"
                       << gm->getPlayerShip() << ":"
                       << gm->getPlayerBall() << ":"
                       << gm->getPlayerBird() << ":"
                       << gm->getPlayerDart() << ":"
                       << gm->getPlayerRobot() << ":"
                       << gm->getPlayerSpider() << ":"
                       << gm->getPlayerSwing() << ":"
                       << static_cast<int>(col1.r) << ":" << static_cast<int>(col1.g) << ":" << static_cast<int>(col1.b) << ":"
                       << static_cast<int>(col2.r) << ":" << static_cast<int>(col2.g) << ":" << static_cast<int>(col2.b) << ":"
                       << (glowEnabled ? 1 : 0) << ":"
                       << static_cast<int>(colGlow.r) << ":" << static_cast<int>(colGlow.g) << ":" << static_cast<int>(colGlow.b);
                    statusStr = ss.str();
                } else {
#ifdef GEODE_IS_MOBILE
                    if (s_isTouching) {
                        levelPos = this->m_objectLayer->convertToNodeSpace(s_lastTouchPos);
                        m_fields->m_lastSentLevelPos = levelPos;
                    } else {
                        levelPos = m_fields->m_lastSentLevelPos;
                    }
#else
                    auto mousePos = geode::cocos::getMousePos();
                    levelPos = this->m_objectLayer->convertToNodeSpace(mousePos);
#endif
                    
                    if (auto* ui = this->m_editorUI) {
                        int mode = ui->m_selectedMode;
                        int swipe = ui->m_swipeEnabled ? 1 : 0;
                        int objectId = 0;
                        if (mode == 2) { // Build mode
                            objectId = s_selectedObjectID;
                        } else if (mode == 3) { // Edit mode
                            if (ui->m_selectedObject) {
                                objectId = ui->m_selectedObject->m_objectID;
                            } else if (ui->m_selectedObjects && ui->m_selectedObjects->count() > 0) {
                                if (auto* first = typeinfo_cast<GameObject*>(ui->m_selectedObjects->objectAtIndex(0))) {
                                    objectId = first->m_objectID;
                                }
                            }
                        }
                        statusStr = std::to_string(mode) + ":" + std::to_string(swipe) + ":" + std::to_string(objectId);
                    }
                }
                
                auto data = proto::serializeCursorUpdate(levelPos.x, levelPos.y, statusStr);
                P2PManager::get().send(std::move(data), ChannelType::Unreliable);
            }
        }
    }
};

// ============================================================
// EditorUI — Hook object movement/transform to sync
// ============================================================

namespace {
    // Captures the pre-edit state of a set of objects, runs a mutating lambda
    // (the real GD call), then broadcasts transform + move deltas for any
    // objects that changed. Shared by transformObjectCall, rotateObjects,
    // scaleObjects, flipObjectsX and flipObjectsY so they all sync identically.
    //
    // This is the single source of truth for transform syncing, including
    // flipX/flipY (mirror) which previously slipped through because GD's
    // mirror buttons call flipObjectsX/Y rather than transformObjectCall.
    //
    // NOTE: we deliberately do NOT also hook the umbrella EditorUI::transformObjects().
    // Hooking that caused a stale-cache clobber on deselect: it ran the sync
    // from a layer that committed the transform before syncDeselections had a
    // chance to update its tracked-selection saveString baseline, so the
    // subsequent deselect diff computed a spurious delta and re-broadcast a
    // stale/empty transform. The property-diff in syncDeselections is already
    // the universal fallback that syncs any transform (Q/E, buttons, mirror),
    // so these per-command hooks are sufficient on their own.
    void syncTransformedObjects(cocos2d::CCArray* objects,
                                std::function<void()> applyBase) {
        if (s_inTransformSync) {
            applyBase();
            return;
        }

        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        // Pre-state: uuid + old position for every object we can track.
        struct ObjState {
            std::string uuid;
            GameObject* obj;
            cocos2d::CCPoint oldPos;
        };
        std::vector<ObjState> selected;

        if (session.isInSession() && !handler.isProcessingRemote() && objects) {
            for (auto* obj : CCArrayExt<GameObject*>(objects)) {
                if (!obj) continue;
                // Skip objects still awaiting their initial PlaceObjects flush.
                // The remote doesn't know this UUID yet, so any transform/move
                // message would be silently dropped. The pending PlaceObjects
                // flush will capture the final state (including any transforms
                // applied between creation and flush).
                if (handler.isObjectPendingPlacement(obj)) continue;
                auto uuid = handler.getUUIDForObject(obj);
                if (!uuid.empty()) {
                    selected.push_back({uuid, obj, obj->getPosition()});
                }
            }
        }

        s_inTransformSync = true;
        applyBase();
        s_inTransformSync = false;

        if (selected.empty()) return;

        std::vector<ActionSerializer::TransformData> transforms;
        std::vector<ActionSerializer::MoveData> moves;

        for (auto& state : selected) {
            ActionSerializer::TransformData td;
            td.uuid = state.uuid;
            td.rotation = state.obj->getRotation();
            td.scaleX = state.obj->getScaleX();
            td.scaleY = state.obj->getScaleY();
            td.flipX = state.obj->isFlipX();
            td.flipY = state.obj->isFlipY();
            transforms.push_back(td);

            cocos2d::CCPoint newPos = state.obj->getPosition();
            float dx = newPos.x - state.oldPos.x;
            float dy = newPos.y - state.oldPos.y;
            if (dx != 0.f || dy != 0.f) {
                ActionSerializer::MoveData md;
                md.uuid = state.uuid;
                md.dx = dx;
                md.dy = dy;
                moves.push_back(md);
            }
        }

        if (!transforms.empty()) {
            auto data = proto::serializeTransformObjects(transforms);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }
        if (!moves.empty()) {
            auto data = proto::serializeMoveObjects(moves);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }

        // Update tracked saveString baselines so syncDeselections does NOT
        // see a stale diff for these changes and send a redundant
        // UpdateObjects (which would cause double-move / double-transform
        // on the remote side).
        auto& tracked = handler.getTrackedSelections();
        auto* editor = LevelEditorLayer::get();
        if (editor) {
            for (auto& state : selected) {
                auto tIt = tracked.find(state.obj);
                if (tIt != tracked.end()) {
                    tIt->second = state.obj->getSaveString(editor);
                }
            }
        }
    }
}

class $modify(MPEditorUI, EditorUI) {
    struct Fields {
        float m_lockRefreshTimer = 0.f;
    };

    void onCreateObject(int id) {
        EditorUI::onCreateObject(id);
        s_selectedObjectID = id;
    }

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
        bool res = EditorUI::ccTouchBegan(touch, event);
        if (touch) {
            s_lastTouchPos = touch->getLocation();
            s_isTouching = true;
        }
        return res;
    }

    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
        EditorUI::ccTouchMoved(touch, event);
        if (touch) {
            s_lastTouchPos = touch->getLocation();
            s_isTouching = true;
        }
    }

    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
        EditorUI::ccTouchEnded(touch, event);
        s_isTouching = false;
        if (SessionManager::get().isInSession()) {
            MessageBatcher::get().flush();
        }
    }

    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
        EditorUI::ccTouchCancelled(touch, event);
        s_isTouching = false;
        if (SessionManager::get().isInSession()) {
            MessageBatcher::get().flush();
        }
    }

    void selectObject(GameObject* obj, bool filter) {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        if (session.isInSession() && !handler.isProcessingRemote() && obj) {
            auto uuid = handler.getUUIDForObject(obj);
            if (!uuid.empty()) {
                auto const& locks = handler.getObjectLocks();
                auto it = locks.find(uuid);
                if (it != locks.end() && it->second.playerId != session.getLocalPlayerId()) {
                    // Locked by another player! Do not select.
                    return;
                }
            }
        }

        EditorUI::selectObject(obj, filter);

        if (session.isInSession() && obj) {
            auto uuid = handler.getOrCreateUUID(obj);
            auto& tracked = handler.getTrackedSelections();
            if (tracked.find(obj) == tracked.end()) {
                if (auto* editor = LevelEditorLayer::get()) {
                    tracked[obj] = obj->getSaveString(editor);
                }
                if (!handler.isProcessingRemote()) {
                    auto data = proto::serializeLockObjects({uuid}, true);
                    P2PManager::get().send(std::move(data), ChannelType::Reliable);

                    // Auto-sync: broadcast the object's full current state on
                    // select. This acts as a reconciliation point — if the
                    // object drifted due to fast actions or dropped deltas,
                    // selecting it snaps all remotes back to the truth.
                    // Skip objects still pending their initial PlaceObjects.
                    if (!handler.isObjectPendingPlacement(obj) && obj->m_objectID != 31) {
                        if (auto* editor = LevelEditorLayer::get()) {
                            auto objData = ActionSerializer::extractObjectData(obj, uuid);
                            auto syncData = proto::serializeUpdateObjects({objData});
                            P2PManager::get().send(std::move(syncData), ChannelType::Reliable);
                        }
                    }
                }
            }
        }
    }

    void deselectObject(GameObject* obj) {
        EditorUI::deselectObject(obj);
        
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();
        if (session.isInSession() && obj) {
            auto& tracked = handler.getTrackedSelections();
            if (!handler.isProcessingRemote()) {
                auto uuid = handler.getUUIDForObject(obj);
                if (!uuid.empty()) {
                    // Send a final UpdateObjects if the object changed since
                    // selection. This catches fast edits (e.g. rotate then
                    // immediately copy+paste) that slipped past the
                    // syncDeselections tick.
                    auto tIt = tracked.find(obj);
                    if (tIt != tracked.end() && obj->m_objectID != 31) {
                        if (auto* editor = LevelEditorLayer::get()) {
                            std::string currentSave = obj->getSaveString(editor);
                            if (tIt->second != currentSave) {
                                auto objData = ActionSerializer::extractObjectData(obj, uuid);
                                auto data = proto::serializeUpdateObjects({objData});
                                P2PManager::get().send(std::move(data), ChannelType::Reliable);
                            }
                        }
                    }

                    auto data = proto::serializeLockObjects({uuid}, false);
                    P2PManager::get().send(std::move(data), ChannelType::Reliable);
                }
            }
            tracked.erase(obj);
        }
    }

    void deselectAll() {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();
        if (session.isInSession()) {
            if (!handler.isProcessingRemote()) {
                auto* editor = LevelEditorLayer::get();
                std::vector<std::string> uuids;
                auto& tracked = handler.getTrackedSelections();
                for (auto const& [obj, _] : tracked) {
                    if (editor && editor->m_objects && editor->m_objects->containsObject(obj)) {
                        auto uuid = handler.getUUIDForObject(obj);
                        if (!uuid.empty()) {
                            uuids.push_back(uuid);
                        }
                    }
                }
                if (!uuids.empty()) {
                    auto data = proto::serializeLockObjects(uuids, false);
                    P2PManager::get().send(std::move(data), ChannelType::Reliable);
                }
            }
            handler.getTrackedSelections().clear();
        }
        EditorUI::deselectAll();
    }

    void onDeleteSelected(cocos2d::CCObject* sender) {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        if (session.isInSession() && !handler.isProcessingRemote()) {
            std::vector<std::string> uuids;
            
            if (m_selectedObjects && m_selectedObjects->count() > 0) {
                for (auto* obj : CCArrayExt<GameObject*>(m_selectedObjects)) {
                    auto uuid = handler.getUUIDForObject(obj);
                    if (!uuid.empty()) {
                        uuids.push_back(uuid);
                        handler.unregisterObject(uuid);
                    }
                }
            } else if (m_selectedObject) {
                auto uuid = handler.getUUIDForObject(m_selectedObject);
                if (!uuid.empty()) {
                    uuids.push_back(uuid);
                    handler.unregisterObject(uuid);
                }
            }

            if (!uuids.empty()) {
                auto data = proto::serializeDeleteObjects(uuids);
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }

        EditorUI::onDeleteSelected(sender);
    }

    bool shouldDeleteObject(GameObject* obj) {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        if (session.isInSession() && !handler.isProcessingRemote() && obj) {
            auto uuid = handler.getUUIDForObject(obj);
            if (!uuid.empty()) {
                auto const& locks = handler.getObjectLocks();
                auto it = locks.find(uuid);
                if (it != locks.end() && it->second.playerId != session.getLocalPlayerId()) {
                    // Locked by another player! Do not delete.
                    return false;
                }
            }
        }
        return EditorUI::shouldDeleteObject(obj);
    }

    void selectObjects(cocos2d::CCArray* objects, bool filter) {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        cocos2d::CCArray* filteredObjects = objects;
        if (session.isInSession() && !handler.isProcessingRemote() && objects) {
            auto const& locks = handler.getObjectLocks();
            int localId = session.getLocalPlayerId();
            
            bool hasLocked = false;
            for (auto* obj : CCArrayExt<GameObject*>(objects)) {
                auto uuid = handler.getUUIDForObject(obj);
                if (!uuid.empty()) {
                    auto it = locks.find(uuid);
                    if (it != locks.end() && it->second.playerId != localId) {
                        hasLocked = true;
                        break;
                    }
                }
            }

            if (hasLocked) {
                filteredObjects = cocos2d::CCArray::create();
                for (auto* obj : CCArrayExt<GameObject*>(objects)) {
                    auto uuid = handler.getUUIDForObject(obj);
                    bool isLockedByOther = false;
                    if (!uuid.empty()) {
                        auto it = locks.find(uuid);
                        if (it != locks.end() && it->second.playerId != localId) {
                            isLockedByOther = true;
                        }
                    }
                    if (!isLockedByOther) {
                        filteredObjects->addObject(obj);
                    }
                }
            }
        }

        EditorUI::selectObjects(filteredObjects, filter);

        if (session.isInSession() && filteredObjects) {
            std::vector<std::string> uuids;
            auto& tracked = handler.getTrackedSelections();
            auto* editor = LevelEditorLayer::get();
            for (auto* obj : CCArrayExt<GameObject*>(filteredObjects)) {
                auto uuid = handler.getOrCreateUUID(obj);
                if (tracked.find(obj) == tracked.end()) {
                    if (editor) {
                        tracked[obj] = obj->getSaveString(editor);
                    }
                    uuids.push_back(uuid);
                }
            }
            if (!uuids.empty() && !handler.isProcessingRemote()) {
                auto data = proto::serializeLockObjects(uuids, true);
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }
    }

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        // Add a helper node to handle syncDeselections updates safely without member function pointer layout mismatch
        auto* helper = UpdateHelperNode::create([this](float dt) {
            this->syncDeselections(dt);
        }, 0.1f);
        if (helper) {
            helper->setID("sync-deselect-helper"_spr);
            this->addChild(helper);
        }
        return true;
    }

    void syncDeselections(float dt) {
        auto* editor = LevelEditorLayer::get();
        if (!editor || !editor->m_objects) return;

        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();
        if (!session.isInSession() || handler.isProcessingRemote()) return;

        auto& tracked = handler.getTrackedSelections();
        auto const& locks = handler.getObjectLocks();
        int localId = session.getLocalPlayerId();

        // 1. Gather all currently selected objects
        std::vector<GameObject*> currentSelection;
        if (m_selectedObject) {
            currentSelection.push_back(m_selectedObject);
        }
        if (m_selectedObjects) {
            for (auto* obj : CCArrayExt<GameObject*>(m_selectedObjects)) {
                if (obj) currentSelection.push_back(obj);
            }
        }

        // 2. Identify objects to deselect (locked by others) or lock (newly selected by us)
        std::vector<GameObject*> toDeselect;
        std::vector<std::string> toLockUuids;

        for (auto* obj : currentSelection) {
            auto uuid = handler.getUUIDForObject(obj);
            // If the object has no UUID (e.g. just pasted), register it immediately
            if (uuid.empty()) {
                uuid = RemoteActionHandler::generateUUID();
                handler.registerObject(uuid, obj);
            }

            // Check if it's locked by another player
            auto it = locks.find(uuid);
            if (it != locks.end() && it->second.playerId != localId) {
                toDeselect.push_back(obj);
            } else {
                // If not tracked yet, track it and queue for locking
                if (tracked.find(obj) == tracked.end()) {
                    tracked[obj] = obj->getSaveString(editor);
                    toLockUuids.push_back(uuid);
                }
            }
        }

        // 3. Deselect locked objects
        for (auto* obj : toDeselect) {
            this->deselectObject(obj);
            if (m_selectedObject == obj) {
                m_selectedObject = nullptr;
            }
            if (m_selectedObjects && m_selectedObjects->containsObject(obj)) {
                m_selectedObjects->removeObject(obj);
            }
        }

        // 4. Send lock messages for newly selected objects
        if (!toLockUuids.empty()) {
            auto data = proto::serializeLockObjects(toLockUuids, true);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }

        // 5. selection lock refresh timer (for already tracked selections)
        m_fields->m_lockRefreshTimer += dt;
        if (m_fields->m_lockRefreshTimer >= 1.0f) {
            m_fields->m_lockRefreshTimer = 0.f;
            std::vector<std::string> refreshUuids;
            for (auto const& [obj, _] : tracked) {
                if (editor->m_objects->containsObject(obj)) {
                    auto uuid = handler.getUUIDForObject(obj);
                    if (!uuid.empty()) {
                        refreshUuids.push_back(uuid);
                    }
                }
            }
            if (!refreshUuids.empty()) {
                auto data = proto::serializeLockObjects(refreshUuids, true);
                P2PManager::get().send(std::move(data), ChannelType::Reliable);
            }
        }

        // 6. Handle deselections and update modified objects.
        //
        // We run the property-diff (getSaveString) on every tracked selected
        // object every tick. This is NOT optional: transforms that don't go
        // through a touch (Q/E rotate, the rotate/scale buttons, Mirror/Flip X/Y)
        // change object properties without setting s_isTouching, so gating the
        // diff on touch (as 0.3.0 did) silently dropped those changes. The diff
        // is the universal fallback that syncs any property change regardless of
        // how it was triggered. Performance is fine because the loop only covers
        // currently-selected objects, not the whole level.
        std::vector<std::string> unlockUuids;
        std::vector<ActionSerializer::ReconcileData> reconciles;
        std::vector<ActionSerializer::ObjectData> updates;

        for (auto it = tracked.begin(); it != tracked.end(); ) {
            GameObject* obj = it->first;

            // Check if object still exists in the editor to avoid dangling pointers
            if (!editor->m_objects->containsObject(obj)) {
                it = tracked.erase(it);
                continue;
            }

            // Check if object is still selected and not scheduled for deselection
            bool isSelected = (std::find(currentSelection.begin(), currentSelection.end(), obj) != currentSelection.end()) &&
                              (std::find(toDeselect.begin(), toDeselect.end(), obj) == toDeselect.end());

            auto uuid = handler.getUUIDForObject(obj);
            if (uuid.empty()) {
                it = tracked.erase(it);
                continue;
            }

            if (!isSelected) {
                unlockUuids.push_back(uuid);

                // Send a quick Reconcile on deselect to fix any drift!
                ActionSerializer::ReconcileData rec;
                rec.uuid = uuid;
                rec.x = obj->getPositionX();
                rec.y = obj->getPositionY();
                rec.rotation = obj->getRotation();
                rec.scaleX = obj->getScaleX();
                rec.scaleY = obj->getScaleY();
                rec.flipX = obj->isFlipX();
                rec.flipY = obj->isFlipY();
                reconciles.push_back(rec);

                // Only send UpdateObjects if there are deep property changes (color, groups, etc.)
                // This prevents massive lag spikes from deleting/recreating objects that were only moved.
                std::string currentSave = obj->getSaveString(editor);
                if (ActionSerializer::hasDeepPropertyChanges(it->second, currentSave)) {
                    if (obj->m_objectID != 31) {
                        updates.push_back(ActionSerializer::extractObjectData(obj, uuid));
                    }
                }

                it = tracked.erase(it);
            } else {
                // If still selected, diff every tick and broadcast any change.
                // This is the universal fallback that syncs ALL property edits
                // regardless of how they were triggered (drag, Q/E rotate,
                // rotate/scale/flip buttons, mirror) — those transforms don't go
                // through a touch and don't set s_isTouching, so gating on touch
                // (as 0.3.0 did) silently dropped them. Cost is bounded: the loop
                // only covers currently-selected objects.
                std::string currentSave = obj->getSaveString(editor);
                if (ActionSerializer::hasDeepPropertyChanges(it->second, currentSave)) {
                    if (obj->m_objectID != 31) {
                        updates.push_back(ActionSerializer::extractObjectData(obj, uuid));
                    }
                    it->second = currentSave;
                } else if (it->second != currentSave) {
                    // Update the cached save string anyway so we don't keep diffing position
                    it->second = currentSave;
                }
                ++it;
            }
        }

        if (!unlockUuids.empty()) {
            auto data = proto::serializeLockObjects(unlockUuids, false);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }
        if (!reconciles.empty()) {
            auto data = proto::serializeReconcileObjects(reconciles);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }
        if (!updates.empty()) {
            auto data = proto::serializeUpdateObjects(updates);
            P2PManager::get().send(std::move(data), ChannelType::Reliable);
        }
    }

    // Hook moveObject to detect object movement by the user
    void moveObject(GameObject* obj, CCPoint position) {
        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        if (session.isInSession() && !handler.isProcessingRemote() && obj) {
            auto uuid = handler.getUUIDForObject(obj);
            if (!uuid.empty()) {
                auto const& locks = handler.getObjectLocks();
                auto it = locks.find(uuid);
                if (it != locks.end() && it->second.playerId != session.getLocalPlayerId()) {
                    // Locked by another player! Do not move.
                    return;
                }
            }
        }

        CCPoint oldPos = obj->getPosition();

        EditorUI::moveObject(obj, position);

        if (session.isInSession() && !handler.isProcessingRemote()) {
            auto uuid = handler.getUUIDForObject(obj);
            if (!uuid.empty() && !handler.isObjectPendingPlacement(obj)) {
                CCPoint newPos = obj->getPosition();
                ActionSerializer::MoveData move;
                move.uuid = uuid;
                move.dx = newPos.x - oldPos.x;
                move.dy = newPos.y - oldPos.y;

                if ((move.dx != 0.f || move.dy != 0.f) && !s_inTransformSync) {
                    MessageBatcher::get().queueMove(uuid, move.dx, move.dy);

                    // Update the tracked saveString baseline so that
                    // syncDeselections does NOT see a stale diff for this
                    // position change and send a redundant UpdateObjects.
                    // Without this, both MoveBatch (relative delta) AND
                    // UpdateObjects (absolute pos via saveString rebuild)
                    // reach the remote — if UpdateObjects arrives first the
                    // delta gets applied on top → double-move.
                    auto& tracked = handler.getTrackedSelections();
                    auto tIt = tracked.find(obj);
                    if (tIt != tracked.end()) {
                        if (auto* editor = LevelEditorLayer::get()) {
                            tIt->second = obj->getSaveString(editor);
                        }
                    }
                }
            }
        }
    }

    void transformObjectCall(EditCommand command) {
        syncTransformedObjects(m_selectedObjects, [&]() {
            EditorUI::transformObjectCall(command);
        });
    }

    void rotateObjects(cocos2d::CCArray* objects, float rotation, cocos2d::CCPoint pivotPoint) {
        syncTransformedObjects(objects, [&]() {
            EditorUI::rotateObjects(objects, rotation, pivotPoint);
        });
    }

    void scaleObjects(cocos2d::CCArray* objects, float scaleX, float scaleY, cocos2d::CCPoint pivotPoint, ObjectScaleType type, bool lockMove) {
        syncTransformedObjects(objects, [&]() {
            EditorUI::scaleObjects(objects, scaleX, scaleY, pivotPoint, type, lockMove);
        });
    }

    // GD's "Mirror" buttons call flipObjectsX/flipObjectsY directly (NOT
    // transformObjectCall), so without these hooks mirror was never synced
    // to remote players. flipX/flipY are carried by the transform message.
    void flipObjectsX(cocos2d::CCArray* objects) {
        syncTransformedObjects(objects, [&]() {
            EditorUI::flipObjectsX(objects);
        });
    }

    void flipObjectsY(cocos2d::CCArray* objects) {
        syncTransformedObjects(objects, [&]() {
            EditorUI::flipObjectsY(objects);
        });
    }
};

class $modify(MPBaseGameLayer, GJBaseGameLayer) {
    void addToSection(GameObject* obj) {
        GJBaseGameLayer::addToSection(obj);

        auto& handler = RemoteActionHandler::get();
        auto& session = SessionManager::get();

        if (!session.isInSession() || handler.isProcessingRemote() || !obj) {
            return;
        }

        if (!handler.isInitialSyncCompleted()) {
            return;
        }

        auto* editor = LevelEditorLayer::get();
        if (!editor || static_cast<GJBaseGameLayer*>(editor) != this) {
            return;
        }

        auto* mpEditor = modify_cast<MPLevelEditorLayer*>(editor);
        if (!mpEditor || !session.isInSession() || mpEditor->m_fields->m_inUndoRedo) {
            return;
        }

        // If the object already has a UUID, it's already registered (e.g., via createObject)
        if (!handler.getUUIDForObject(obj).empty()) {
            return;
        }

        // Assign a new UUID and queue it for a batched placement flush.
        // Copy/paste/duplicate can add dozens of objects in a single frame;
        // queueing them lets us send one place_objects message (via the network
        // tick) instead of one WebSocket send per object.
        auto uuid = RemoteActionHandler::generateUUID();
        handler.registerObject(uuid, obj);
        handler.queueObjectForPlacement(uuid, obj);
    }
};

#include <Geode/modify/GJColorSetupLayer.hpp>
class $modify(MPGJColorSetupLayer, GJColorSetupLayer) {
    void syncColors() {
        auto& handler = RemoteActionHandler::get();
        if (handler.isProcessingRemote() || !handler.isInitialSyncCompleted()) return;

        auto& session = SessionManager::get();
        if (!session.isInSession()) return;
        
        auto editor = LevelEditorLayer::get();
        if (editor && editor->m_levelSettings) {
            ActionSerializer::LevelSettingsData settings;
            settings.saveString = editor->m_levelSettings->getSaveString();
            settings.audioTrack = editor->m_level->m_audioTrack;
            settings.songID = editor->m_level->m_songID;
            settings.levelLength = editor->m_level->m_levelLength;
            
            auto packet = proto::serializeUpdateSettings(settings);
            P2PManager::get().send(std::move(packet), ChannelType::Reliable);
        }
    }

    void colorSelectClosed(cocos2d::CCNode* popup) {
        GJColorSetupLayer::colorSelectClosed(popup);
        
        auto* editor = LevelEditorLayer::get();
        if (editor && editor->m_levelSettings && editor->m_levelSettings->m_effectManager) {
            auto* dict = editor->m_levelSettings->m_effectManager->m_colorActionDict;
            if (dict) {
                log::info("m_colorActionDict contains {} items.", dict->count());
                auto* keys = dict->allKeys();
                if (keys) {
                    for (int i = 0; i < keys->count(); i++) {
                        auto* keyObj = keys->objectAtIndex(i);
                        if (auto* strKey = typeinfo_cast<cocos2d::CCString*>(keyObj)) {
                            log::info("Key type: String, val: {}", strKey->getCString());
                        } else if (auto* intKey = typeinfo_cast<cocos2d::CCInteger*>(keyObj)) {
                            log::info("Key type: Int, val: {}", intKey->getValue());
                        }
                    }
                }
            }
        }
        
        syncColors();
    }

    void onClose(cocos2d::CCObject* sender) {
        GJColorSetupLayer::onClose(sender);
        syncColors();
    }
};

#include <Geode/modify/CustomizeObjectLayer.hpp>
class $modify(MPCustomizeObjectLayer, CustomizeObjectLayer) {
    void colorSelectClosed(cocos2d::CCNode* popup) {
        CustomizeObjectLayer::colorSelectClosed(popup);
        
        auto& handler = RemoteActionHandler::get();
        if (handler.isProcessingRemote() || !handler.isInitialSyncCompleted()) return;

        auto& session = SessionManager::get();
        if (!session.isInSession()) return;
        
        auto editor = LevelEditorLayer::get();
        if (editor && editor->m_levelSettings) {
            ActionSerializer::LevelSettingsData settings;
            settings.saveString = editor->m_levelSettings->getSaveString();
            settings.audioTrack = editor->m_level->m_audioTrack;
            settings.songID = editor->m_level->m_songID;
            settings.levelLength = editor->m_level->m_levelLength;
            
            auto packet = proto::serializeUpdateSettings(settings);
            P2PManager::get().send(std::move(packet), ChannelType::Reliable);
        }
    }
};

#include <Geode/modify/LevelSettingsLayer.hpp>
class $modify(MPLevelSettingsLayer, LevelSettingsLayer) {
    void onClose(cocos2d::CCObject* sender) {
        LevelSettingsLayer::onClose(sender);

        auto& handler = RemoteActionHandler::get();
        if (handler.isProcessingRemote() || !handler.isInitialSyncCompleted()) return;

        auto& session = SessionManager::get();
        if (!session.isInSession()) return;

        auto editor = LevelEditorLayer::get();
        if (editor && editor->m_levelSettings) {
            ActionSerializer::LevelSettingsData settings;
            settings.saveString = editor->m_levelSettings->getSaveString();
            settings.audioTrack = editor->m_level->m_audioTrack;
            settings.songID = editor->m_level->m_songID;
            settings.levelLength = editor->m_level->m_levelLength;
            
            auto packet = proto::serializeUpdateSettings(settings);
            P2PManager::get().send(std::move(packet), ChannelType::Reliable);
        }
    }
};

#include <Geode/modify/ColorSelectPopup.hpp>
class $modify(MPColorSelectPopup, ColorSelectPopup) {
    void closeColorSelect(cocos2d::CCObject* sender) {
        ColorSelectPopup::closeColorSelect(sender);

        auto& handler = RemoteActionHandler::get();
        if (handler.isProcessingRemote() || !handler.isInitialSyncCompleted()) return;

        auto& session = SessionManager::get();
        if (!session.isInSession()) return;

        auto editor = LevelEditorLayer::get();
        if (editor && editor->m_levelSettings) {
            ActionSerializer::LevelSettingsData settings;
            settings.saveString = editor->m_levelSettings->getSaveString();
            settings.audioTrack = editor->m_level->m_audioTrack;
            settings.songID = editor->m_level->m_songID;
            settings.levelLength = editor->m_level->m_levelLength;
            
            auto packet = proto::serializeUpdateSettings(settings);
            P2PManager::get().send(std::move(packet), ChannelType::Reliable);
        }
    }
};
