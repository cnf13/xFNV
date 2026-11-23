#include "BSShader/PrecipitationShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI PrecipitationShaderProperty::ms_RTTI("PrecipitationShaderProperty", &BSShaderProperty::ms_RTTI);

static_assert(MEM_CONTEXT_PRECIPITATION == 0x29);

PrecipitationShaderProperty::PrecipitationShaderProperty()
  : iParticleCount(0), fRotationSpeed(0.0f), spBaseTexture(nullptr), eFaceMode(NiBillboardNode::ROTATE_ABOUT_UP),
    ePrecipType(PRECIPTYPE_RAIN) {
    iShaderPropertyType = 0x12;
}

PrecipitationShaderProperty::PrecipitationShaderProperty(
    int32_t aiParticleCount,
    float afRotationSpeed,
    NiPoint3& akVelocity,
    NiPoint3& akMin,
    NiPoint3& akMax,
    PrecipitationShaderProperty::PrecipitationType aePrecipType,
    NiBillboardNode::FaceMode aeFaceMode
)
  : iParticleCount(aiParticleCount), fRotationSpeed(afRotationSpeed), kMin(akMin), kMax(akMax), kVelocity(akVelocity),
    spBaseTexture(nullptr), TextureClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T),
    eFaceMode(static_cast<NiBillboardNode::FaceMode>(aeFaceMode != NiBillboardNode::ALWAYS_FACE_CAMERA)),
    ePrecipType(aePrecipType) {}

PrecipitationShaderProperty::~PrecipitationShaderProperty() {
    iParticleCount = 0;
    spBaseTexture = nullptr;
}

const NiRTTI* PrecipitationShaderProperty::GetRTTI() const {
    return &PrecipitationShaderProperty::ms_RTTI;
}

// The clone is allocated in the precipitation heap context.
NiObject* PrecipitationShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_PRECIPITATION);
    PrecipitationShaderProperty* pClone = new PrecipitationShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void PrecipitationShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(PrecipitationShaderProperty::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v20 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v20);
    char* ViewerString_5 = NiGetViewerString("iParticleCount", iParticleCount);
    uint32_t v8 = apStrings->m_usSize;
    uint32_t v9 = apStrings->m_usMaxSize;
    v20 = ViewerString_5;
    if (v8 >= v9) {
        apStrings->SetSize(v8 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v8, v20);
    if (spBaseTexture.m_pObject) {
        char* ViewerString_9;
        const char* pname = BSUtilities::GetTextureName(spBaseTexture.m_pObject);
        if (pname) {
            ViewerString_9 = NiGetViewerString("base texture", pname);
        }
        else {
            ViewerString_9 = NiGetViewerString("base texture", static_cast<const void*>(spBaseTexture.m_pObject));
        }
        uint32_t v12 = apStrings->m_usMaxSize;
        uint32_t v13 = apStrings->m_usSize;
        v20 = ViewerString_9;
        if (v13 >= v12) {
            apStrings->SetSize(v13 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v13, v20);
    }
    if (GetClampNI() == NiTexturingProperty::CLAMP_S_CLAMP_T) {
        char* ViewerString_0 = NiGetViewerString("clamp", true);
        uint32_t v15 = apStrings->m_usSize;
        uint32_t v16 = apStrings->m_usMaxSize;
        v20 = ViewerString_0;
        if (v15 >= v16) {
            apStrings->SetSize(v15 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v15, v20);
    }
    char* v17 = NiBillboardNode::GetViewerString("billboard mode", eFaceMode);
    uint32_t v18 = apStrings->m_usSize;
    uint32_t v19 = apStrings->m_usMaxSize;
    v20 = v17;
    if (v18 >= v19) {
        apStrings->SetSize(v18 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v18, v20);
}

bool PrecipitationShaderProperty::SetupGeometry(NiGeometry* apGeometry) {
    NiTexture* m_pObject;
    NiTexturingProperty::ClampMode v8;
    // Tests the property state's address and, when it is null, updates the geometry's properties; the texturing
    // property is read through the address taken before the call.
    NiPropertyState* pkState = &apGeometry->m_PropertyState;
    if (!pkState) {
        apGeometry->UpdateProperties();
    }
    NiTexturingProperty* ptexprop = static_cast<NiTexturingProperty*>(pkState->m_aspProps[5].m_pObject);
    if (!ptexprop) {
        return false;
    }
    NiTexturingProperty::Map* v4 = *ptexprop->m_kMaps.m_pBase;
    if (v4) {
        m_pObject = v4->m_spTexture.m_pObject;
    }
    else {
        m_pObject = nullptr;
    }
    spBaseTexture = m_pObject;
    NiTexturingProperty::Map** m_pBase = ptexprop->m_kMaps.m_pBase;
    if (*m_pBase) {
        v8 = static_cast<NiTexturingProperty::ClampMode>(
            ((*m_pBase)->m_uFlags & NiTexturingProperty::Map::CLAMPMODE_MASK) >> NiTexturingProperty::Map::CLAMPMODE_POS
        );
    }
    else {
        v8 = NiTexturingProperty::WRAP_S_WRAP_T;
    }
    SetClampNI(v8);
    apGeometry->DetachProperty(ptexprop);
    return true;
}

BSShaderProperty::RenderPassArray* PrecipitationShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    AutoMemContext kMemContext(MEM_CONTEXT_PRECIPITATION);
    if (iLastRenderPassState != aiEnabledPasses) {
        CheckCreateRenderPassArray(1);
        pRenderPassList->iSize = 0;
        pRenderPassList
            ->Add(apGeom, BSShaderManager::BSSM_PRECIPITATION_RAIN, 1, 0, nullptr, nullptr, nullptr, nullptr);
        iLastRenderPassState = aiEnabledPasses | auiRenderMode << 8;
    }
    BSShaderProperty::RenderPassArray* result = pRenderPassList;
    return result;
}

D3DTEXTUREADDRESS PrecipitationShaderProperty::GetClampD3D() const {
    return D3DTADDRESS_WRAP;
}

void PrecipitationShaderProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    TextureClampMode = (aClampMode != D3DTADDRESS_CLAMP) ? NiTexturingProperty::WRAP_S_WRAP_T
                                                         : NiTexturingProperty::CLAMP_S_CLAMP_T;
}

NiTexturingProperty::ClampMode PrecipitationShaderProperty::GetClampNI() const {
    return TextureClampMode;
}

void PrecipitationShaderProperty::SetClampNI(NiTexturingProperty::ClampMode aClampMode) {
    TextureClampMode = aClampMode;
}

void PrecipitationShaderProperty::CopyMembers(PrecipitationShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderProperty::CopyMembers(pDest, arCloneData);
    pDest->iParticleCount = iParticleCount;
    pDest->kMin = kMin;
    pDest->kMax = kMax;
    pDest->kVelocity = kVelocity;
    NiTexture* m_pObject = pDest->spBaseTexture.m_pObject;
    if (m_pObject == spBaseTexture.m_pObject) {
        pDest->TextureClampMode = TextureClampMode;
    }
    else {
        if (m_pObject) {
            NiDecRefCount(m_pObject);
        }
        NiTexture* v5 = spBaseTexture.m_pObject;
        pDest->spBaseTexture.m_pObject = v5;
        if (v5) {
            NiIncRefCount(v5);
        }
        pDest->TextureClampMode = TextureClampMode;
    }
}
