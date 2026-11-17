#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class NiDynamicEffect;
class NiTextureEffect;

// The lights and projected effects that affect one geometry, as singly linked lists.
class NiDynamicEffectState : public NiRefObject {
public:
    class ListItem {
    public:
        ListItem* m_pNext;
        NiDynamicEffect* m_pItem;
    };

    ~NiDynamicEffectState() override;

    bool IsMultiTexture() const {
        return m_bMultiTexture;
    }

    bool m_bMultiTexture;
    ListItem* m_pLightList;
    ListItem* m_pProjLightList;
    ListItem* m_pProjShadowList;
    NiTextureEffect* m_pEnvMap;
    NiTextureEffect* m_pFogMap;
};
