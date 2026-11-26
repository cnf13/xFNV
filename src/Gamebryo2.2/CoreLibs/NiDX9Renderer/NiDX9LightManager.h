#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"

class NiDX9RenderState;
class NiDynamicEffectState;
class NiLight;

// Sets the device's fixed-function lights from a geometry's dynamic effects. The layout is Gamebryo 2.2's
// NiDX9LightManager.h: the light map (hash size 37), the device light list, the device (AddRef'd), the render state, -1
// as the invalid state, the next free index and the 512-byte index bit set.
class NiDX9LightManager : public NiMemObject {
public:
    // One device light: the D3D9 light a NiLight produced, the NiLight revision it was built from, its device light
    // index and two flags. The constructor sets m_uiRevID to 0 and both flags to 0; Update rebuilds m_kD3DLight when
    // m_uiRevID differs from the NiLight's; UpdateLights reads m_iIndex and the flags. Public because the map
    // instances' DeletingDestruct specializations below are declared at namespace scope.
    class LightEntry : public NiMemObject {
    public:
        LightEntry(const NiLight& kLight, int32_t iIndex);

        // When kLight's revision differs from m_uiRevID, rebuilds m_kD3DLight from the light by its type and returns
        // true; else returns false.
        bool Update(const NiLight& kLight);

        D3DLIGHT9 m_kD3DLight;
        uint32_t m_uiRevID;
        int32_t m_iIndex;
        bool m_bNeedsRemoval;
        bool m_bActive;
    };

    NiDX9LightManager(NiDX9RenderState* pkRenderState, IDirect3DDevice9* pkD3DDevice9);
    ~NiDX9LightManager();

    // Sets the lighting and material-source render states for a texturing and vertex colour property and, when the
    // state or the ambient-diffuse mode changed, the device lights (UpdateLights).
    void SetState(
        const NiDynamicEffectState* pkState,
        const NiTexturingProperty* pkTex,
        const NiVertexColorProperty* pkColor
    );
    // Disables the device lights enabled during the last frame.
    void NextFrame();
    // Forgets apLight and frees its device light index.
    void RemoveLight(NiLight* apLight);
    // Disables and forgets every device light (before a device reset).
    void ReleaseLights();
    // Sets the device's lighting render states again.
    void InitRenderModes();
    // Takes the first clear bit of m_auiValidIndices from m_uiNextAvailableIndex on (wrapping at 4096), sets it and
    // returns its index, or -1.
    uint32_t GetNextAvailableIndex();
    void UpdateLights(const NiDynamicEffectState* pkState);

    NiTPointerMap<NiLight*, LightEntry*> m_kLights;
    NiTPointerList<NiLight*> m_kDeviceLights;
    IDirect3DDevice9* m_pkD3DDevice9;
    NiDX9RenderState* m_pkRenderState;
    // Gamebryo declares this const NiDynamicEffectState* const; Construct stores -1 in it.
    const NiDynamicEffectState* m_pkInvalidState;
    const NiDynamicEffectState* m_pkLastState;
    bool m_bLastAmbientDiffuse;
    bool m_bAmbientDiffuse;
    NiTexturingProperty::ApplyMode m_eApply;
    NiVertexColorProperty::SourceVertexMode m_eSourceMode;
    uint32_t m_uiNextAvailableIndex;
    // One bit per device light index (LIGHTINDEXARRAYSIZE 128 words).
    uint32_t m_auiValidIndices[128];

    static float ms_fMaxRange;
};
