#include "Gamebryo2.2/CoreLibs/NiMain/NiSingleShaderMaterial.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"

NiRTTI NiSingleShaderMaterial::ms_RTTI("NiSingleShaderMaterial", &NiMaterial::ms_RTTI);

NiSingleShaderMaterial::NiSingleShaderMaterial(const NiFixedString& kName, const char* pcName, bool bPreloadShader)
  : NiMaterial(kName), m_kShaderName(pcName), m_spCachedShader(nullptr), m_bManualShaderManagement(false) {
    if (bPreloadShader) {
        // The SDK's preload: the factory loads the shader; the temporary reference is dropped at once.
        NiPointer<NiShader> spShader =
            NiShaderFactory::GetInstance()->RetrieveShader(pcName, NiShader::DEFAULT_IMPLEMENTATION, false);
    }
}

NiSingleShaderMaterial::~NiSingleShaderMaterial() {}

const NiRTTI* NiSingleShaderMaterial::GetRTTI() const {
    return &NiSingleShaderMaterial::ms_RTTI;
}

bool NiSingleShaderMaterial::IsShaderCurrent(
    NiShader* a1,
    const NiGeometry* a2,
    const NiSkinInstance* a3,
    const NiPropertyState* a4,
    const NiDynamicEffectState* a5,
    uint32_t a6
) {
    return a1 && (&a1->m_kName)->m_kHandle == m_kShaderName.GetHandle();
}

NiShader* NiSingleShaderMaterial::GetCurrentShader(
    const NiGeometry* a1,
    const NiSkinInstance* a2,
    const NiPropertyState* a3,
    const NiDynamicEffectState* a4,
    uint32_t a5
) {
    NiShaderFactory* pkShaderFactory = NiShaderFactory::GetInstance();
    NiShader* m_pObject = m_spCachedShader.m_pObject;
    if (m_pObject) {
        if (m_pObject->m_uiImplementation == a5) {
            return m_spCachedShader.m_pObject;
        }
        if (a5 == -1) {
            if (!pkShaderFactory) {
                return nullptr;
            }
            if (pkShaderFactory->IsDefaultImplementation(m_spCachedShader.m_pObject)) {
                return m_spCachedShader.m_pObject;
            }
        }
    }
    if (pkShaderFactory) {
        NiShader* pkShader = pkShaderFactory->RetrieveShader(m_kShaderName.GetHandle(), a5, true);
        if ((pkShader) && (pkShader->IsInitialized() || pkShader->Initialize())) {
            return pkShader;
        }
    }
    return nullptr;
}

void NiSingleShaderMaterial::UnloadShaders() {
    NiShader* m_pObject;
    if (!m_bManualShaderManagement) {
        m_pObject = m_spCachedShader.m_pObject;
        if (m_pObject) {
            m_pObject->DecRefCount();
            m_spCachedShader.m_pObject = nullptr;
        }
    }
}

NiSingleShaderMaterial* NiSingleShaderMaterial::Create(const char* materialname, int32_t a2) {
    NiSingleShaderMaterial* pkMaterial = static_cast<NiSingleShaderMaterial*>(NiMaterial::GetMaterial(materialname));
    if (pkMaterial) {
        return pkMaterial;
    }
    // The constructor reads only the low byte of the caller's value.
    return new NiSingleShaderMaterial(materialname, materialname, static_cast<uint8_t>(a2));
}
