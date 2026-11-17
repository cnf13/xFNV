#include "BSShader/BSFogProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFogProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <bit>

NiRTTI BSFogProperty::ms_RTTI("BSFogProperty", &NiFogProperty::ms_RTTI);

// The fog starts at the depth's share of the 163840 maximum and ends there; flat plane, black water, power 1.
BSFogProperty::BSFogProperty()
  : fStartDistance(static_cast<float>((1.0 - m_fDepth * 0.8299999833106995) * 163840.0)), fEndDistance(163840.0f),
    fPower(1.0f) {
    kWaterColor.r = 0.0f;
    kWaterColor.g = 0.0f;
    kWaterColor.b = 0.0f;
    kPlane.m_kNormal.x = 0.0f;
    kPlane.m_kNormal.y = 0.0f;
    kPlane.m_kNormal.z = 0.0f;
    kPlane.m_fConstant = 0.0f;
}

// Constructs an empty fog property, then copies the members into it.
NiObject* BSFogProperty::CreateClone(NiCloningProcess& arCloneData) {
    BSFogProperty* pClone = static_cast<BSFogProperty*>(NiMemObject::operator new(sizeof(BSFogProperty)));
    BSFogProperty* pResult = nullptr;
    if (pClone) {
        pResult = ::new (pClone) BSFogProperty;
    }
    CopyMembers(pResult, arCloneData);
    return pResult;
}

const NiRTTI* BSFogProperty::GetRTTI() const {
    return &BSFogProperty::ms_RTTI;
}

void BSFogProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiFogProperty::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(BSFogProperty::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v13 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v13);
    char* ViewerString_7 = NiGetViewerString("fog start distance", fStartDistance);
    uint32_t v8 = apStrings->m_usSize;
    uint32_t v9 = apStrings->m_usMaxSize;
    v13 = ViewerString_7;
    if (v8 >= v9) {
        apStrings->SetSize(v8 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v8, v13);
    char* v10 = NiGetViewerString("fog end distance", fEndDistance);
    uint32_t v11 = apStrings->m_usSize;
    uint32_t v12 = apStrings->m_usMaxSize;
    v13 = v10;
    if (v11 >= v12) {
        apStrings->SetSize(v11 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v11, v13);
}

void BSFogProperty::SetDistance(float afStart, float afEnd) {
    fStartDistance = afStart;
    fEndDistance = afEnd;
}

// Sets the start distance from afDepth (0 to 1) along the 163840 maximum, (1 - afDepth * 0.83) * 163840, and the end
// distance to the maximum. Computes in double and stores floats.
void BSFogProperty::SetDepth(float afDepth) {
    fStartDistance = (float)((1.0 - afDepth * 0.8299999833106995) * 163840.0);
    fEndDistance = 163840.0f;
}

void BSFogProperty::SetFogHeight(float afHeight) {
    fHeight = afHeight;
}

void BSFogProperty::SetWaterColor(const NiColor& aColor) {
    kWaterColor = aColor;
}

void BSFogProperty::CopyMembers(BSFogProperty* pDest, NiCloningProcess& arCloneData) {
    NiProperty::CopyMembers(pDest, arCloneData);
    pDest->m_uFlags = m_uFlags;
    pDest->m_fDepth = m_fDepth;
    pDest->m_kColor = m_kColor;
}

void BSFogProperty::SetWaterDistance(float afStart, float afEnd) {
    fStartWaterDistance = afStart;
    fEndWaterDistance = afEnd;
}

void BSFogProperty::SetPower(float afPower) {
    fPower = afPower;
}

int32_t BSFogProperty::SetColor(float* a1) {
    NiColor* p_m_kColor = &m_kColor;
    p_m_kColor->r = *a1;
    p_m_kColor->g = a1[1];
    int32_t result = *(reinterpret_cast<uint32_t*>(a1) + 2);
    p_m_kColor->b = std::bit_cast<float>(static_cast<uint32_t>(static_cast<uint32_t>(result)));
    return result;
}
