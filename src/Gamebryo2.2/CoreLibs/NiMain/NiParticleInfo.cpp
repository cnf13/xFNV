#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

// The constructor copies the zero velocity and clears the age, life span, last update,
// generation and code fields.
NiParticleInfo::NiParticleInfo() {
    m_kVelocity = NiPoint3::ZERO;
    m_fAge = 0.0f;
    m_fLifeSpan = 0.0f;
    m_fLastUpdate = 0.0f;
    m_usGeneration = 0;
    m_usCode = 0;
}

void NiParticleInfo::LoadBinary(NiStream& kStream) {
    uint32_t v15;
    NiStream* v2 = &kStream;
    m_kVelocity.LoadBinary(kStream);
    if (v2->GetFileVersion() < 0xA030005) {
        NiPoint3 kRotationAxis;
        kRotationAxis.LoadBinary(*v2);
    }
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v15 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_fAge, 4, &v15, 1);
    NiBinaryStream* v6 = v2->Istr();
    uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
    v15 = 4;
    v6->m_uiAbsoluteCurrentPos += v7(v6, &m_fLifeSpan, 4, &v15, 1);
    NiBinaryStream* v8 = v2->Istr();
    uint32_t (*v9)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v8->m_pfnRead;
    v15 = 4;
    v8->m_uiAbsoluteCurrentPos += v9(v8, &m_fLastUpdate, 4, &v15, 1);
    NiBinaryStream* v10 = v2->Istr();
    uint32_t (*v11)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v10->m_pfnRead;
    v15 = 2;
    v10->m_uiAbsoluteCurrentPos += v11(v10, &m_usGeneration, 2, &v15, 1);
    NiBinaryStream* v12 = v2->Istr();
    uint32_t (*v13)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v12->m_pfnRead;
    v15 = 2;
    v12->m_uiAbsoluteCurrentPos += v13(v12, &m_usCode, 2, &v15, 1);
}

bool NiParticleInfo::IsEqual(const NiParticleInfo& kData) const {
    return kData.m_kVelocity.x == m_kVelocity.x &&
        kData.m_kVelocity.y == m_kVelocity.y &&
        kData.m_kVelocity.z == m_kVelocity.z &&
        kData.m_fAge == m_fAge &&
        kData.m_fLifeSpan == m_fLifeSpan &&
        kData.m_fLastUpdate == m_fLastUpdate &&
        m_usGeneration == kData.m_usGeneration &&
        m_usCode == kData.m_usCode;
}
