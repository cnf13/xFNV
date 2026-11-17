#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

// The texture type names' literals: each old handle is compared with its literal's address before interning it.
using FogMapLiteral_t = const char[7];
using EnvironmentMapLiteral_t = const char[15];
using ProjectedShadowLiteral_t = const char[16];
using ProjectedLightLiteral_t = const char[15];
static FogMapLiteral_t FogMapLiteral{"FogMap"};
static EnvironmentMapLiteral_t EnvironmentMapLiteral{"EnvironmentMap"};
static ProjectedShadowLiteral_t ProjectedShadowLiteral{"ProjectedShadow"};
static ProjectedLightLiteral_t ProjectedLightLiteral{"ProjectedLight"};
NiRTTI NiTextureEffect::ms_RTTI("NiTextureEffect", &NiDynamicEffect::ms_RTTI);
NiTextureEffect::TextureTypeStringArray NiTextureEffect::ms_akTextureTypeStrings;

NiTextureEffect::NiTextureEffect()
  : m_kModelProjMat(NiMatrix3_IDENTITY), m_kModelProjTrans(0.0f, 0.0f, 0.0f), m_kWorldProjMat(NiMatrix3_IDENTITY),
    m_kWorldProjTrans(0.0f, 0.0f, 0.0f), m_spTexture(nullptr), m_eFilter(NiTexturingProperty::FILTER_NEAREST),
    m_eClamp(NiTexturingProperty::WRAP_S_WRAP_T), m_eTextureMode(PROJECTED_LIGHT), m_eCoordMode(WORLD_PARALLEL),
    m_bPlaneEnable(false), m_kModelPlane(NiPoint3_UNIT_X, 0.0f), m_kWorldPlane(NiPoint3_UNIT_X, 0.0f) {
    m_ucEffectType = TEXTURE_EFFECT;
}

// The renderer drops what it holds for this effect; the texture is released by its pointer's destructor.
NiTextureEffect::~NiTextureEffect() {
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeEffect(this);
    }
}

const NiRTTI* NiTextureEffect::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiTextureEffect::CreateClone(NiCloningProcess& kCloning) {
    NiTextureEffect* v4;
    void* v3 = NiMemObject::operator new(sizeof(NiTextureEffect));
    if (v3) {
        v4 = ::new (v3) NiTextureEffect;
        CopyMembers(v4, kCloning);
        return v4;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// Files before 10.3.0.4 carry two more 16-bit values and files before 4.1.0.16 a third; all are skipped.
void NiTextureEffect::LoadBinary(NiStream& kStream) {
    NiDynamicEffect::LoadBinary(kStream);
    m_kModelProjMat.LoadBinary(kStream);
    m_kModelProjTrans.LoadBinary(kStream);

    uint32_t uiValue;
    NiStreamLoadBinary(kStream, uiValue);
    m_eFilter = static_cast<NiTexturingProperty::FilterMode>(uiValue);
    NiStreamLoadBinary(kStream, uiValue);
    m_eClamp = static_cast<NiTexturingProperty::ClampMode>(uiValue);
    NiStreamLoadBinary(kStream, uiValue);
    m_eTextureMode = static_cast<TextureType>(uiValue);
    NiStreamLoadBinary(kStream, uiValue);
    m_eCoordMode = static_cast<CoordGenType>(uiValue);

    // The texture.
    kStream.ReadLinkID();

    uint8_t ucPlaneEnable;
    NiStreamLoadBinary(kStream, ucPlaneEnable);
    m_bPlaneEnable = ucPlaneEnable != 0;
    m_kModelPlane.LoadBinary(kStream);

    uint16_t usUnused;
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 3, 0, 4)) {
        NiStreamLoadBinary(kStream, usUnused);
        NiStreamLoadBinary(kStream, usUnused);
    }
    if (kStream.GetFileVersion() < NiStream::GetVersion(4, 1, 0, 16)) {
        NiStreamLoadBinary(kStream, usUnused);
    }
}

void NiTextureEffect::LinkObject(NiStream& kStream) {
    NiDynamicEffect::LinkObject(kStream);
    m_spTexture = static_cast<NiTexture*>(kStream.GetObjectFromLinkID());
}

bool NiTextureEffect::RegisterStreamables(NiStream& kStream) {
    if (!NiDynamicEffect::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spTexture) {
        m_spTexture->RegisterStreamables(kStream);
    }
    return true;
}

void NiTextureEffect::SaveBinary(NiStream& kStream) {
    NiDynamicEffect::SaveBinary(kStream);
    m_kModelProjMat.SaveBinary(kStream);
    m_kModelProjTrans.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_eFilter);
    NiStreamSaveBinary(kStream, m_eClamp);
    NiStreamSaveBinary(kStream, m_eTextureMode);
    NiStreamSaveBinary(kStream, m_eCoordMode);
    kStream.SaveLinkID(m_spTexture);
    // Copies the bool to a byte and writes that.
    uint8_t ucPlaneEnable = m_bPlaneEnable;
    NiStreamSaveBinary(kStream, ucPlaneEnable);
    m_kModelPlane.SaveBinary(kStream);
}

bool NiTextureEffect::IsEqual(NiObject* pObject) {
    if (!NiDynamicEffect::IsEqual(pObject)) {
        return false;
    }

    NiTextureEffect* pkEffect = static_cast<NiTextureEffect*>(pObject);
    if (!(pkEffect->m_kModelProjMat == m_kModelProjMat)) {
        return false;
    }
    if (pkEffect->m_kModelProjTrans != m_kModelProjTrans) {
        return false;
    }
    if (pkEffect->m_eFilter != m_eFilter || pkEffect->m_eClamp != m_eClamp) {
        return false;
    }
    if (pkEffect->m_eTextureMode != m_eTextureMode || pkEffect->m_eCoordMode != m_eCoordMode) {
        return false;
    }
    if (pkEffect->m_bPlaneEnable != m_bPlaneEnable) {
        return false;
    }
    if (pkEffect->m_kModelPlane != m_kModelPlane) {
        return false;
    }
    return true;
}

// The texture mode is listed twice.
void NiTextureEffect::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiDynamicEffect::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(m_kModelProjMat.GetViewerString("m_kModelProjMat"));
    pStrings->Add(m_kModelProjTrans.GetViewerString("m_kModelProjTrans"));
    pStrings->Add(GetViewerString("m_eTextureMode", m_eTextureMode));
    pStrings->Add(NiTexturingProperty::GetViewerString("m_eFilter", m_eFilter));
    pStrings->Add(NiTexturingProperty::GetViewerString("m_eClamp", m_eClamp));
    pStrings->Add(GetViewerString("m_eTextureMode", m_eTextureMode));
    pStrings->Add(GetViewerString("m_eCoordMode", m_eCoordMode));
    pStrings->Add(NiGetViewerString("m_bPlaneEnable", m_bPlaneEnable));
    pStrings->Add(m_kModelPlane.GetViewerString("m_kModelPlane"));
}

void NiTextureEffect::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    UpdateProjection();
}

void NiTextureEffect::UpdateProjection() {
    int32_t m_eCoordMode;
    NiMatrix3* v3;
    NiMatrix3* v4;
    NiPoint3* v5;
    double v6;
    NiMatrix3* v7;
    double z;
    float v10;
    float v11;
    float v12;
    float v13;
    float v16;
    float v18;
    float v20;
    NiPoint3 v22;
    NiMatrix3 v23;
    NiMatrix3 v24;
    NiMatrix3 v25;
    m_eCoordMode = (int32_t)this->m_eCoordMode;
    if (m_eCoordMode == 2) {
        m_kWorldProjTrans.y = 0x1p-1f;
        m_kWorldProjTrans.x = 0x1p-1f;
        m_kWorldProjTrans.z = (float)0.0;
        v10 = m_kWorld.m_Rotate.m_pEntry[0][1];
        m_kWorldProjMat.m_pEntry[0][0] = (float)(m_kWorld.m_Rotate.m_pEntry[0][2] * 0.5);
        m_kWorldProjMat.m_pEntry[1][0] = (float)(v10 * (-0.5));
        m_kWorldProjMat.m_pEntry[2][0] = (float)0.0;
        v11 = m_kWorld.m_Rotate.m_pEntry[1][1];
        m_kWorldProjMat.m_pEntry[0][1] = (float)(m_kWorld.m_Rotate.m_pEntry[1][2] * 0.5);
        m_kWorldProjMat.m_pEntry[1][1] = (float)(v11 * (-0.5));
        m_kWorldProjMat.m_pEntry[2][1] = (float)0.0;
        v12 = m_kWorld.m_Rotate.m_pEntry[2][1];
        m_kWorldProjMat.m_pEntry[0][2] = (float)(0.5 * m_kWorld.m_Rotate.m_pEntry[2][2]);
        m_kWorldProjMat.m_pEntry[1][2] = (float)((-0.5) * v12);
        m_kWorldProjMat.m_pEntry[2][2] = (float)0.0;
    }
    else {
        if ((m_eCoordMode == 3) || (m_eCoordMode == 4)) {
            m_kWorldProjTrans.z = (float)0.0;
            m_kWorldProjTrans.y = (float)0.0;
            m_kWorldProjTrans.x = (float)0.0;
            v7 = &(v24 = (&m_kWorld.m_Rotate)->Transpose());
            __builtin_memcpy(&m_kWorldProjMat, &(v23 = (&m_kModelProjMat)->operator*(*v7)), sizeof(m_kWorldProjMat));
        }
        else {
            v13 = (float)(1.0 / m_kWorld.m_fScale);
            v3 = &(v23 = (&m_kWorld.m_Rotate)->Transpose());
            v4 = &(v24 = (&m_kModelProjMat)->operator*(*v3));
            __builtin_memcpy(&m_kWorldProjMat, &(v25 = v4->operator*(v13)), sizeof(m_kWorldProjMat));
            v5 = &(v22 = (&m_kWorldProjMat)->operator*(m_kWorld.m_Translate));
            v16 = m_kModelProjTrans.x - v5->x;
            v18 = m_kModelProjTrans.y - v5->y;
            v6 = (double)(m_kModelProjTrans.z - v5->z);
            m_kWorldProjTrans.x = v16;
            m_kWorldProjTrans.y = v18;
            v20 = (float)v6;
            m_kWorldProjTrans.z = v20;
        }
    }
    float v17 = ((m_kWorld.m_Rotate.m_pEntry[0][1] * m_kModelPlane.m_kNormal.y) +
                 (m_kWorld.m_Rotate.m_pEntry[0][0] * m_kModelPlane.m_kNormal.x)) +
        (m_kWorld.m_Rotate.m_pEntry[0][2] * m_kModelPlane.m_kNormal.z);
    float v19 = ((m_kWorld.m_Rotate.m_pEntry[1][0] * m_kModelPlane.m_kNormal.x) +
                 (m_kWorld.m_Rotate.m_pEntry[1][1] * m_kModelPlane.m_kNormal.y)) +
        (m_kWorld.m_Rotate.m_pEntry[1][2] * m_kModelPlane.m_kNormal.z);
    float v21 = ((m_kWorld.m_Rotate.m_pEntry[2][0] * m_kModelPlane.m_kNormal.x) +
                 (m_kWorld.m_Rotate.m_pEntry[2][1] * m_kModelPlane.m_kNormal.y)) +
        (m_kWorld.m_Rotate.m_pEntry[2][2] * m_kModelPlane.m_kNormal.z);
    double v8 = (double)((m_kWorld.m_Translate.y * v19) + (m_kWorld.m_Translate.x * v17));
    z = (double)m_kWorld.m_Translate.z;
    ++m_uiRevID;
    m_kWorldPlane.m_kNormal.x = v17;
    m_kWorldPlane.m_kNormal.y = v19;
    m_kWorldPlane.m_kNormal.z = v21;
    float v14 = (float)(v8 + (z * v21));
    float v15 = v14 + (m_kModelPlane.m_fConstant * m_kWorld.m_fScale);
    m_kWorldPlane.m_fConstant = v15;
}

void NiTextureEffect::CopyMembers(NiTextureEffect* pDest, NiCloningProcess& kCloning) {
    NiDynamicEffect::CopyMembers(pDest, kCloning);
    pDest->m_spTexture = m_spTexture;
    pDest->m_kModelProjMat = m_kModelProjMat;
    pDest->m_kModelProjTrans = m_kModelProjTrans;
    pDest->m_eFilter = m_eFilter;
    pDest->m_eClamp = m_eClamp;
    pDest->m_eTextureMode = m_eTextureMode;
    pDest->m_eCoordMode = m_eCoordMode;
    pDest->m_bPlaneEnable = m_bPlaneEnable;
    pDest->m_kModelPlane = m_kModelPlane;
    pDest->m_kWorldPlane = m_kWorldPlane;
}

bool NiTextureEffect::GetTypeIDFromName(const NiFixedString& kName, uint32_t& uiID) {
    for (uint32_t i = 0; i < 4; i++) {
        const char* pcType = ms_akTextureTypeStrings[i].GetHandle();
        const char* pcName = kName.GetHandle();
        if (pcType == pcName || (pcName && pcType && NiStricmp(pcType, pcName) == 0)) {
            uiID = i;
            return true;
        }
    }
    return false;
}

// Each entry is assigned as NiFixedString::operator=(const char*) does: it compares the handle with the literal,
// interns the literal through NiGlobalStringTable::AddString and drops the old handle's reference.
void NiTextureEffect::_SDMInit() {
    const char* pcOld = ms_akTextureTypeStrings[PROJECTED_LIGHT].GetHandle();
    if (pcOld != ProjectedLightLiteral) {
        ms_akTextureTypeStrings[PROJECTED_LIGHT].SetHandle(NiGlobalStringTable::AddString(ProjectedLightLiteral));
        if (pcOld != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
        }
    }
    pcOld = ms_akTextureTypeStrings[PROJECTED_SHADOW].GetHandle();
    if (pcOld != ProjectedShadowLiteral) {
        ms_akTextureTypeStrings[PROJECTED_SHADOW].SetHandle(NiGlobalStringTable::AddString(ProjectedShadowLiteral));
        if (pcOld != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
        }
    }
    pcOld = ms_akTextureTypeStrings[ENVIRONMENT_MAP].GetHandle();
    if (pcOld != EnvironmentMapLiteral) {
        ms_akTextureTypeStrings[ENVIRONMENT_MAP].SetHandle(NiGlobalStringTable::AddString(EnvironmentMapLiteral));
        if (pcOld != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
        }
    }
    pcOld = ms_akTextureTypeStrings[FOG_MAP].GetHandle();
    if (pcOld != FogMapLiteral) {
        ms_akTextureTypeStrings[FOG_MAP].SetHandle(NiGlobalStringTable::AddString(FogMapLiteral));
        if (pcOld != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
        }
    }
}

// Assigns a null string to each entry: an entry that already holds no handle is left alone; the others get the interned
// null string and drop the old handle's reference as in _SDMInit.
void NiTextureEffect::_SDMShutdown() {
    for (uint32_t i = PROJECTED_LIGHT; i <= FOG_MAP; i++) {
        const char* pcOld = ms_akTextureTypeStrings[i].GetHandle();
        if (pcOld) {
            ms_akTextureTypeStrings[i].SetHandle(NiGlobalStringTable::AddString(nullptr));
            if (pcOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
            }
        }
    }
}

char* NiTextureEffect::GetViewerString(const char* apPrefix, TextureType aeMode) {
    size_t stLength = strlen(apPrefix) + 25;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case PROJECTED_LIGHT: NiSprintf(pcString, stLength, "%s = PROJECTED_LIGHT", apPrefix); break;
        case PROJECTED_SHADOW: NiSprintf(pcString, stLength, "%s = PROJECTED_SHADOW", apPrefix); break;
        case ENVIRONMENT_MAP: NiSprintf(pcString, stLength, "%s = ENVIRONMENT_MAP", apPrefix); break;
        case FOG_MAP: NiSprintf(pcString, stLength, "%s = FOG_MAP", apPrefix); break;
        default: break;
    }
    return pcString;
}

char* NiTextureEffect::GetViewerString(const char* apPrefix, CoordGenType aeMode) {
    size_t stLength = strlen(apPrefix) + 25;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case WORLD_PARALLEL: NiSprintf(pcString, stLength, "%s = WORLD_PARALLEL", apPrefix); break;
        case WORLD_PERSPECTIVE: NiSprintf(pcString, stLength, "%s = WORLD_PERSPECTIVE", apPrefix); break;
        case SPHERE_MAP: NiSprintf(pcString, stLength, "%s = SPHERE_MAP", apPrefix); break;
        case SPECULAR_CUBE_MAP: NiSprintf(pcString, stLength, "%s = SPECULAR_CUBE_MAP", apPrefix); break;
        case DIFFUSE_CUBE_MAP: NiSprintf(pcString, stLength, "%s = DIFFUSE_CUBE_MAP", apPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apPrefix); break;
    }
    return pcString;
}

NiObject* NiTextureEffect::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiTextureEffect));
    if (v0) {
        return ::new (v0) NiTextureEffect;
    }

    return nullptr;
}
