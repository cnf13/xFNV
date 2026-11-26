#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

#include <cstdint>

class NiGeometryData;
class NiGeometryGroup;
class NiGeometryGroupManager;
class NiScreenTexture;
class NiSkinInstance;

// Creates geometry groups and moves geometry between them.
class NiGeometryGroupManager : public NiMemObject {
public:
    enum RepackingBehavior : uint32_t {
        STATIC,
        UNSHARED,
        DYNAMIC,
    };

    NiGeometryGroupManager();
    virtual ~NiGeometryGroupManager();

    virtual NiGeometryGroup* CreateGroup(NiGeometryGroupManager::RepackingBehavior eBehavior) {
        return nullptr;
    }
    virtual bool DestroyGroup(NiGeometryGroup* pkGroup) {
        return false;
    }
    virtual bool IsGroupEmpty(NiGeometryGroup* pkGroup) const {
        return true;
    }

    virtual bool AddObjectToGroup(NiGeometryGroup* pkGroup, NiGeometryData* pkData, NiSkinInstance* pkSkinInstance) {
        return false;
    }

    virtual bool AddObjectToGroup(NiGeometryGroup* pkGroup, NiScreenTexture* pkScreenTexture) {
        return false;
    }
    virtual bool RemoveObjectFromGroup(NiGeometryData* pkData, NiSkinInstance* pkSkinInstance) {
        return false;
    }
    virtual bool RemoveObjectFromGroup(NiScreenTexture* pkScreenTexture) {
        return false;
    }

    static inline NiGeometryGroupManager* ms_pkManager;
};
