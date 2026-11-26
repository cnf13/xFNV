#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

class NiMaterialProperty;

// The material the render state last sent to the device.
class NiDX9MaterialManager : public NiMemObject {
public:
    struct MaterialEntry : public NiMemObject {
        // Fills the D3D material from the property and returns whether the entry changed; it is called both for
        // construction (after m_uiRevID is cleared) and for Update.
        bool Update(const NiMaterialProperty* apMaterial);

        D3DMATERIAL9 m_kD3DMat;
        uint32_t m_uiRevID;
    };

    ~NiDX9MaterialManager();

    void PurgeMaterial(NiMaterialProperty* apMaterial);

    // Makes the entry of apMaterial (a new one when it has no renderer data) the current material and updates it.
    void SetCurrentMaterial(const NiMaterialProperty* apMaterial);

    void UpdateToDevice(IDirect3DDevice9* apDevice);

    // Forgets the current material, makes the default material current and marks it
    // changed.
    void Initialize();

    MaterialEntry* m_pkCurrentMaterial;
    bool m_bChanged;
};
