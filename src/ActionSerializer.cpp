#include "ActionSerializer.hpp"
#include <Geode/Geode.hpp>
#include <sstream>
#include <unordered_map>

using namespace geode::prelude;

namespace mpedit::ActionSerializer {



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
            std::string s = obj->getSaveString(editor);
            size_t pos = s.find(';');
            if (pos != std::string::npos) {
                s = s.substr(0, pos);
            }
            data.saveString = s;
        }

        if (obj->m_groups && obj->m_groupCount > 0) {
            int count = std::min(static_cast<int>(obj->m_groupCount), 10);
            for (int i = 0; i < count; i++) {
                data.groups.push_back((*obj->m_groups)[i]);
            }
        }

        return data;
    }

    std::unordered_map<std::string, std::string> parseSaveString(std::string const& str) {
        std::unordered_map<std::string, std::string> out;
        
        std::string s = str;
        size_t pos = s.find(';');
        if (pos != std::string::npos) {
            s = s.substr(0, pos);
        }

        std::stringstream ss(s);
        std::string key, val;
        while (std::getline(ss, key, ',') && std::getline(ss, val, ',')) {
            out[key] = val;
        }
        return out;
    }

    std::string buildSaveString(std::unordered_map<std::string, std::string> const& map) {
        std::string result = "";
        for (auto const& [k, v] : map) {
            result += k + "," + v + ",";
        }
        return result;
    }

    std::vector<std::pair<std::string, std::string>> parseSaveStringOrdered(std::string const& str) {
        std::vector<std::pair<std::string, std::string>> out;
        std::string s = str;
        size_t pos = s.find(';');
        if (pos != std::string::npos) s = s.substr(0, pos);
        std::stringstream ss(s);
        std::string key, val;
        while (std::getline(ss, key, ',') && std::getline(ss, val, ',')) {
            out.push_back({key, val});
        }
        return out;
    }

    std::string buildSaveStringOrdered(std::vector<std::pair<std::string, std::string>> const& vec) {
        std::string result = "";
        for (auto const& p : vec) {
            result += p.first + "," + p.second + ",";
        }
        return result;
    }

    void injectLocalStartPosState(ObjectData& remoteData, GameObject* localObj) {
        if (!localObj || remoteData.objectID != 31 || remoteData.saveString.empty()) return;
        
        if (auto* editor = LevelEditorLayer::get()) {
            auto localMap = parseSaveString(localObj->getSaveString(editor));
            auto remoteVec = parseSaveStringOrdered(remoteData.saveString);
            
            std::vector<std::pair<std::string, std::string>> newRemoteVec;
            for (auto const& p : remoteVec) {
                if (p.first == "kA21" || p.first == "kA9" || p.first == "93") continue;
                newRemoteVec.push_back(p);
            }
            
            if (localMap.count("kA21")) newRemoteVec.push_back({"kA21", localMap.at("kA21")});
            if (localMap.count("kA9")) newRemoteVec.push_back({"kA9", localMap.at("kA9")});
            if (localMap.count("93")) newRemoteVec.push_back({"93", localMap.at("93")});
            
            remoteData.saveString = buildSaveStringOrdered(newRemoteVec);
        }
    }

    bool hasDeepPropertyChanges(GameObject* obj, std::string const& oldSave, std::string const& newSave) {
        if (oldSave == newSave) return false;
        
        auto oldMap = parseSaveString(oldSave);
        auto newMap = parseSaveString(newSave);

        for (auto const& k : {"2", "3"}) {
            oldMap.erase(k);
            newMap.erase(k);
        }

        if (obj && obj->m_objectID == 31) {
            oldMap.erase("kA21");
            newMap.erase("kA21");
            oldMap.erase("kA9");
            newMap.erase("kA9");
            oldMap.erase("93");
            newMap.erase("93");
        }

        auto getValue = [](std::unordered_map<std::string, std::string> const& m, std::string const& k, std::string const& def) {
            auto it = m.find(k);
            return it == m.end() ? def : it->second;
        };

        std::vector<std::pair<std::string, std::string>> transformKeys = {
            {"4", "0"}, {"5", "0"}, {"6", "0"}, {"11", "0"}, 
            {"32", "1"}, {"128", "1"}, {"129", "1"}
        };

        for (auto const& [k, def] : transformKeys) {
            if (getValue(oldMap, k, def) != getValue(newMap, k, def)) {
                return true;
            }
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

}
