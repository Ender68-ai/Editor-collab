#include "ActionSerializer.hpp"
#include <Geode/Geode.hpp>
#include <sstream>
#include <unordered_map>

using namespace geode::prelude;

namespace mpedit::ActionSerializer {

    // ============================================================
    // GameObject Helpers
    // ============================================================

    ObjectData extractObjectData(GameObject* obj, std::string const& uuid) {
        ObjectData data;
        data.uuid = uuid;

        if (!obj) return data;

        data.objectID = obj->m_objectID;
        data.x = obj->getPositionX();
        data.y = obj->getPositionY();
        data.rotation = obj->getRotation();
        data.scaleX = obj->getScaleX();
        data.scaleY = obj->getScaleY();
        data.flipX = obj->isFlipX();
        data.flipY = obj->isFlipY();
        data.zOrder = obj->getZOrder();
        data.editorLayer = obj->m_editorLayer;
        data.editorLayer2 = obj->m_editorLayer2;
        data.saveString = "";
        if (auto* editor = LevelEditorLayer::get()) {
            data.saveString = obj->getSaveString(editor);
        }

        // Extract groups safely up to a maximum of 10 to avoid out-of-bounds/std::out_of_range crashes
        if (obj->m_groups && obj->m_groupCount > 0) {
            int count = std::min(static_cast<int>(obj->m_groupCount), 10);
            for (int i = 0; i < count; i++) {
                data.groups.push_back((*obj->m_groups)[i]);
            }
        }

        return data;
    }

    std::unordered_map<int, std::string> parseSaveString(std::string const& str) {
        std::unordered_map<int, std::string> out;
        std::stringstream ss(str);
        std::string key, val;
        while (std::getline(ss, key, ',') && std::getline(ss, val, ',')) {
            auto parsed = geode::utils::numFromString<int>(key);
            if (parsed.isOk()) {
                out[parsed.unwrap()] = val;
            }
        }
        return out;
    }

    bool hasDeepPropertyChanges(std::string const& oldSave, std::string const& newSave) {
        if (oldSave == newSave) return false;
        
        auto oldMap = parseSaveString(oldSave);
        auto newMap = parseSaveString(newSave);

        // Keys to ignore for purely positional/transform reconciles:
        // 2: X, 3: Y, 4: FlipX, 5: FlipY, 11: Rot, 32: Scale, 128: ScaleX, 129: ScaleY
        for (int k : {2, 3, 4, 5, 11, 32, 128, 129}) {
            oldMap.erase(k);
            newMap.erase(k);
        }

        if (oldMap.size() != newMap.size()) return true;
        for (auto const& [k, v] : oldMap) {
            auto it = newMap.find(k);
            if (it == newMap.end() || it->second != v) return true;
        }
        return false;
    }

} // namespace mpedit::ActionSerializer
