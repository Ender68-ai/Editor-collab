#include "ActionSerializer.hpp"
#include <Geode/Geode.hpp>

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

} // namespace mpedit::ActionSerializer
