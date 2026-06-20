#pragma once

#include "ActionSerializer.hpp"
#include <string>
#include <unordered_map>
#include <Geode/binding/MusicDownloadDelegate.hpp>

#include <optional>
#include <vector>

class GameObject;
class LevelEditorLayer;

namespace mpedit {

    struct LockInfo {
        int playerId;
        float timeLeft;
    };

    /**
     * Handles incoming remote actions and applies them to the local editor.
     * Maintains a UUID-to-GameObject mapping for tracking remote objects.
     */
    class RemoteActionHandler : public MusicDownloadDelegate {
    public:
        static RemoteActionHandler& get();

        // Register network handlers for remote actions
        void setupHandlers();
        void clearHandlers();

        // Apply remote actions to the local editor
        void handleRemotePlaceObjects(int playerId, std::vector<ActionSerializer::ObjectData> const& objects);
        void handleRemoteDeleteObjects(int playerId, std::vector<std::string> const& uuids);
        void handleRemoteMoveObjects(int playerId, std::vector<ActionSerializer::MoveData> const& moves);
        void handleRemoteTransformObjects(int playerId, std::vector<ActionSerializer::TransformData> const& transforms);
        void handleRemoteUpdateObjects(int playerId, std::vector<ActionSerializer::ObjectData> const& objects);
        void handleRemoteLockObjects(int playerId, std::vector<std::string> const& uuids, bool locked);
        void handleRemoteSyncLevel(int playerId, std::string const& objectsString, std::vector<std::string> const& uuids, ActionSerializer::LevelSettingsData const& settings, std::vector<ActionSerializer::LockData> const& locks, bool isPendingSync = false);
        void handleRemoteUpdateSettings(int playerId, ActionSerializer::LevelSettingsData const& settings);

        std::unordered_map<std::string, LockInfo> const& getObjectLocks() const { return m_objectLocks; }
        
        // Call this every frame to decay lock timers
        void updateLocks(float dt);

        // History pruning helper
        void pruneObjectFromHistory(LevelEditorLayer* editor, GameObject* obj);

        // UUID management
        void registerObject(std::string const& uuid, GameObject* obj);
        void unregisterObject(std::string const& uuid);
        GameObject* getObjectByUUID(std::string const& uuid) const;
        std::string getUUIDForObject(GameObject* obj) const;
        std::string getOrCreateUUID(GameObject* obj);

        // Generate a new UUID
        static std::string generateUUID();

        // Clear all mappings (called when leaving editor)
        void clearMappings();

        // --- Batched placement sync ---
        // Copy/paste/duplicate can spawn dozens of objects in a single frame.
        // Instead of sending one place_objects message per object (one WS send
        // + one getSaveString each), queue them here and flush as a single
        // message via flushPendingPlacements() on the next network tick.
        void queueObjectForPlacement(std::string const& uuid, GameObject* obj);
        void flushPendingPlacements();

        // Flag to suppress outgoing messages when processing remote actions
        bool isProcessingRemote() const { return m_processingRemote; }

        bool isInitialSyncCompleted() const;
        void setInitialSyncCompleted(bool completed) { m_initialSyncCompleted = completed; }

        void applyPendingSync();
        bool hasPendingSync() const { return m_pendingSync.has_value(); }

        // Init-bridge: lets applyPendingSync() resolve the editor while it runs
        // from inside LevelEditorLayer::init() — at that point the editor is not
        // yet in the scene graph, so getEditorLayer() (which walks the running
        // scene) cannot find it. Setting this temporarily makes getEditorLayer()
        // return the editor passed in, so handleRemoteSyncLevel() mutates the
        // right one instead of re-entering the "no editor" branch (recursion).
        void setEditorForInit(LevelEditorLayer* editor) { m_editorForInit = editor; }
        LevelEditorLayer* getEditorForInit() const { return m_editorForInit; }

        std::vector<std::string> const& getExpectedUuids() const { return m_expectedUuids; }
        void setExpectedUuids(std::vector<std::string> const& uuids) { m_expectedUuids = uuids; }
        void clearExpectedUuids() { m_expectedUuids.clear(); }

        void downloadSongFinished(int id) override;
        void downloadSongFailed(int id, GJSongError error) override;
        void downloadSongStarted(int id) override {}
        void loadSongInfoFinished(SongInfoObject* object) override {}
        void loadSongInfoFailed(int id, GJSongError errorType) override {}

        // Selected objects baseline saveStrings (for tracking property changes during selection)
        std::unordered_map<GameObject*, std::string>& getTrackedSelections() { return m_preSelectSaveStrings; }

    private:
        RemoteActionHandler() = default;
        ~RemoteActionHandler() = default;

        RemoteActionHandler(RemoteActionHandler const&) = delete;
        RemoteActionHandler& operator=(RemoteActionHandler const&) = delete;

        LevelEditorLayer* getEditorLayer() const;

        // Applies a LevelSettingsData packet (settings saveString + song) onto
        // the editor. Used by both sync_level and update_settings so the color
        // / portal / song application logic lives in exactly one place.
        void applyLevelSettings(LevelEditorLayer* editor, ActionSerializer::LevelSettingsData const& settings);

        // UUID ↔ GameObject bidirectional mapping
        std::unordered_map<std::string, GameObject*> m_uuidToObject;
        std::unordered_map<GameObject*, std::string> m_objectToUuid;

        // UUID ↔ Lock info
        std::unordered_map<std::string, LockInfo> m_objectLocks;
        // Pending final state for an object being edited by a remote player.
        // We update its transform in-place every tick while it's locked, and
        // store the latest saveString here so that on unlock we can recreate it
        // (to pick up non-transform properties). The authoritative transform is
        // carried alongside because GD's saveString flip round-trip
        // (createObjectsFromString) can land on the OPPOSITE runtime m_isFlipX
        // from what the sender observed — re-applying it after recreate prevents
        // the remote from showing an inverted flip state.
        struct LockedState {
            std::string saveString;
            float rotation = 0.f;
            float scaleX = 1.f;
            float scaleY = 1.f;
            bool flipX = false;
            bool flipY = false;
        };
        std::unordered_map<std::string, LockedState> m_lockedSaveStrings;
        std::unordered_map<GameObject*, std::string> m_preSelectSaveStrings;

        bool m_processingRemote = false;
        bool m_initialSyncCompleted = false;

        // When non-null, getEditorLayer() returns this instead of searching the
        // scene graph. Used to bridge applyPendingSync() → handleRemoteSyncLevel()
        // during LevelEditorLayer::init() (see setEditorForInit comment above).
        LevelEditorLayer* m_editorForInit = nullptr;

        struct PendingSync {
            int playerId;
            std::string objectsString;
            std::vector<std::string> uuids;
            ActionSerializer::LevelSettingsData settings;
            std::vector<ActionSerializer::LockData> locks;
        };
        std::optional<PendingSync> m_pendingSync;
        std::vector<std::string> m_expectedUuids;

        // Objects queued for a batched place_objects flush. Stored as UUID +
        // Ref<GameObject> (Ref keeps the object alive across the frame boundary
        // even if GD's arrays drop it).
        struct PendingPlacement {
            std::string uuid;
            geode::Ref<GameObject> obj;
        };
        std::vector<PendingPlacement> m_pendingPlacements;

        // Counter for UUID generation
        static inline int s_uuidCounter = 0;
    };

} // namespace mpedit
