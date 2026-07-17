#pragma once

#include <string>
#include <vector>


class GameObject;

namespace mpedit {

    /**
     * Data structures for editor actions and state, along with extraction helpers.
     * All network wire encoding/decoding is handled by mpedit::proto (BinaryProtocol).
     */
    namespace ActionSerializer {

        /**
         * Object data structure for network transmission.
         * Contains all properties needed to reconstruct an object.
         */
        struct ObjectData {
            std::string uuid;       // Mod-assigned unique identifier
            std::string saveString; // GD native save string (full object state)
            int objectID = 0;       // GD object type ID
            float x = 0.f;
            float y = 0.f;
            float rotation = 0.f;
            float scaleX = 1.f;
            float scaleY = 1.f;
            bool flipX = false;
            bool flipY = false;
            int zOrder = 0;
            int editorLayer = 0;
            int editorLayer2 = 0;

            std::vector<int> groups;

            int mainColorChannel = -1;
            int secondColorChannel = -1;
        };

        struct LevelSettingsData {
            std::string saveString;
            int audioTrack = 0;
            int songID = 0;
            float levelLength = 0;
        };

        struct MoveData {
            std::string uuid;
            float dx = 0.f;
            float dy = 0.f;
        };

        struct TransformData {
            std::string uuid;
            float rotation = 0.f;
            float scaleX = 1.f;
            float scaleY = 1.f;
            bool flipX = false;
            bool flipY = false;
        };

        struct ReconcileData {
            std::string uuid;
            float x = 0.f;
            float y = 0.f;
            float rotation = 0.f;
            float scaleX = 1.f;
            float scaleY = 1.f;
            bool flipX = false;
            bool flipY = false;
        };

        struct LockData {
            std::string uuid;
            int playerId = 0;
            float timeLeft = 3.0f;
        };



        ObjectData extractObjectData(GameObject* obj, std::string const& uuid);
        

        bool hasDeepPropertyChanges(std::string const& oldSave, std::string const& newSave);

    } // namespace ActionSerializer

} // namespace mpedit
