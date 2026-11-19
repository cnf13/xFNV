#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

NiPropertyState::NiPropertyState() {
    Reset();
}

NiPropertyState::NiPropertyState(const NiPropertyState& kState) {
    for (uint32_t i = 0; i < 7; ++i) {
        m_aspProps[i] = kState.m_aspProps[i];
    }
}

// Releases and clears each slot; the member NiPointers' destructors then find them empty.
NiPropertyState::~NiPropertyState() {
    for (NiPointer<NiProperty>& spProperty : m_aspProps) {
        spProperty = nullptr;
    }
}

// Seven slots, each assigned through NiPointer's copy assignment: the asm releases the old property through
// InterlockedDecrement and its DeleteThis slot, stores the new pointer, then takes its reference.
NiPropertyState& NiPropertyState::operator=(const NiPropertyState& kState) {
    for (uint32_t i = 0; i < 7; ++i) {
        m_aspProps[i] = kState.m_aspProps[i];
    }
    return *this;
}

void NiPropertyState::Reset() {
    NiProperty* m_pObject = m_aspProps[0].m_pObject;
    NiAlphaProperty* v3 = NiAlphaProperty::ms_spDefault.m_pObject;
    if (m_aspProps[0].m_pObject != NiAlphaProperty::ms_spDefault.m_pObject) {
        if (m_pObject) {
            NiDecRefCount(m_pObject);
        }
        m_aspProps[0].m_pObject = v3;
        if (v3) {
            NiIncRefCount(v3);
        }
    }
    NiCullingProperty* v4 = NiCullingProperty::ms_spDefault.m_pObject;
    NiCullingProperty* v5 = static_cast<NiCullingProperty*>(m_aspProps[1].m_pObject);
    if (v5 != NiCullingProperty::ms_spDefault.m_pObject) {
        if (v5) {
            NiDecRefCount(v5);
        }
        m_aspProps[1].m_pObject = v4;
        if (v4) {
            NiIncRefCount(v4);
        }
    }
    NiMaterialProperty* v6 = NiMaterialProperty::ms_spDefault.m_pObject;
    NiMaterialProperty* v7 = static_cast<NiMaterialProperty*>(m_aspProps[2].m_pObject);
    if (v7 != NiMaterialProperty::ms_spDefault.m_pObject) {
        if (v7) {
            NiDecRefCount(v7);
        }
        m_aspProps[2].m_pObject = v6;
        if (v6) {
            NiIncRefCount(v6);
        }
    }
    NiShadeProperty* v8 = NiShadeProperty::ms_spDefault.m_pObject;
    NiShadeProperty* v9 = static_cast<NiShadeProperty*>(m_aspProps[3].m_pObject);
    if (v9 != NiShadeProperty::ms_spDefault.m_pObject) {
        if (v9) {
            NiDecRefCount(v9);
        }
        m_aspProps[3].m_pObject = v8;
        if (v8) {
            NiIncRefCount(v8);
        }
    }
    NiStencilProperty* v10 = NiStencilProperty::ms_spDefault.m_pObject;
    NiStencilProperty* v11 = static_cast<NiStencilProperty*>(m_aspProps[4].m_pObject);
    if (v11 != NiStencilProperty::ms_spDefault.m_pObject) {
        if (v11) {
            NiDecRefCount(v11);
        }
        m_aspProps[4].m_pObject = v10;
        if (v10) {
            NiIncRefCount(v10);
        }
    }
    NiTexturingProperty* v12 = NiTexturingProperty::ms_spDefault.m_pObject;
    NiTexturingProperty* v13 = static_cast<NiTexturingProperty*>(m_aspProps[5].m_pObject);
    if (v13 != NiTexturingProperty::ms_spDefault.m_pObject) {
        if (v13) {
            NiDecRefCount(v13);
        }
        m_aspProps[5].m_pObject = v12;
        if (v12) {
            NiIncRefCount(v12);
        }
    }
}

NiAlphaProperty* NiPropertyState::GetAlpha() {
    return static_cast<NiAlphaProperty*>(m_aspProps[0].m_pObject);
}
