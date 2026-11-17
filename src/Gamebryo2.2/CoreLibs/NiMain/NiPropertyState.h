#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

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
