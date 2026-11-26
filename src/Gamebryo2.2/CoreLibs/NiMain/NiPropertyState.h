#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"

class NiAlphaProperty;
class NiProperty;
class NiShadeProperty;

// The properties in effect for a geometry, one per property type, gathered from the scene graph above it.
class NiPropertyState {
public:
    NiPropertyState();
    NiPropertyState(const NiPropertyState& kState);
    ~NiPropertyState();

    // Stores pkProp in the slot of its type.
    void SetProperty(NiProperty* pkProp);
    // Copies the seven property pointers, releasing each old one and holding each new one.
    NiPropertyState& operator=(const NiPropertyState& kState);
    void Reset();
    NiShadeProperty* GetShade();
    // The alpha property's slot, m_aspProps[0].
    NiAlphaProperty* GetAlpha();

    // the PC state holds seven property slots (sizeof 0x1C)
    NiPointer<NiProperty> m_aspProps[7];
};

inline NiShadeProperty* NiPropertyState::GetShade() {
    return static_cast<NiShadeProperty*>(m_aspProps[NiProperty::SHADE].m_pObject);
}

// Stores pkProp in the slot of its type. The type is compared signed and 7 is accepted, one past the PC's seven slots.
inline void NiPropertyState::SetProperty(NiProperty* pkProp) {
    if (pkProp && pkProp->Type() <= 7) {
        m_aspProps[pkProp->Type()] = pkProp;
    }
}
