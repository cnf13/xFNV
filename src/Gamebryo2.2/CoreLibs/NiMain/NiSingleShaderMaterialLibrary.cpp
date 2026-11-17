#include "Gamebryo2.2/CoreLibs/NiMain/NiSingleShaderMaterialLibrary.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibraryDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSingleShaderMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <cstdint>

NiSingleShaderMaterialLibrary::NiSingleShaderMaterialLibrary()
  : NiMaterialLibrary(NiFixedString("NiSingleShaderMaterialLibrary")), m_pkLastLibrary(nullptr), m_uiLastDesc(0) {}

NiSingleShaderMaterialLibrary::~NiSingleShaderMaterialLibrary() {
    m_pkLastLibrary = nullptr;
    m_uiLastDesc = 0;
}

NiMaterial* NiSingleShaderMaterialLibrary::GetMaterial(const NiFixedString& kName) {
    return NiSingleShaderMaterial::Create(kName.GetHandle(), 1);
}

const char* NiSingleShaderMaterialLibrary::GetMaterialName(uint32_t uiMaterial) {
    uint32_t uiCurrentIdx = 0;
    NiShaderFactory* pkFactory = NiShaderFactory::GetInstance();
    for (NiShaderLibrary* pkLib = pkFactory->GetFirstLibrary(); pkLib; pkLib = pkFactory->GetNextLibrary()) {
        NiShaderLibraryDesc* pkDesc = pkLib->GetShaderLibraryDesc();
        if (pkDesc) {
            if (uiCurrentIdx + pkDesc->GetShaderCount() > uiMaterial) {
                NiShaderDesc* pkShaderDesc = pkDesc->GetShaderDesc(uiMaterial - uiCurrentIdx);
                return pkShaderDesc->GetName();
            }
            uiCurrentIdx += pkDesc->GetShaderCount();
        }
    }
    return nullptr;
}

uint32_t NiSingleShaderMaterialLibrary::GetMaterialCount() const {
    uint32_t uiCount = 0;
    NiShaderFactory* pkFactory = NiShaderFactory::GetInstance();
    if (!pkFactory) {
        return 0;
    }
    for (NiShaderLibrary* pkLib = pkFactory->GetFirstLibrary(); pkLib; pkLib = pkFactory->GetNextLibrary()) {
        NiShaderLibraryDesc* pkDesc = pkLib->GetShaderLibraryDesc();
        if (pkDesc) {
            uiCount += pkDesc->GetShaderCount();
        }
    }
    return uiCount;
}

NiShaderDesc* NiSingleShaderMaterialLibrary::GetFirstMaterialDesc() {
    NiShaderFactory* pkFactory = NiShaderFactory::GetInstance();
    NiShaderLibrary* pkLib = pkFactory->GetFirstLibrary();
    m_pkLastLibrary = pkLib;
    m_uiLastDesc = 0;
    if (!pkLib) {
        return nullptr;
    }
    // As in the SDK, the loop tests neither the next library nor its description for null.
    NiShaderLibraryDesc* pkDesc = pkLib->GetShaderLibraryDesc();
    while (pkDesc->GetShaderCount() == 0) {
        pkLib = pkFactory->GetNextLibrary();
        m_pkLastLibrary = pkLib;
        m_uiLastDesc = 0;
        pkDesc = pkLib->GetShaderLibraryDesc();
    }
    if (pkDesc && pkDesc->GetShaderCount()) {
        return pkDesc->GetShaderDesc(m_uiLastDesc);
    }
    return nullptr;
}

NiShaderDesc* NiSingleShaderMaterialLibrary::GetNextMaterialDesc() {
    NiShaderLibraryDesc* pkDesc;
    NiShaderFactory* pkFactory = NiShaderFactory::GetInstance();
    NiShaderLibrary* pkLib;
    for (pkLib = pkFactory->GetFirstLibrary(); pkLib != m_pkLastLibrary; pkLib = pkFactory->GetNextLibrary()) {}
    ++m_uiLastDesc;
    if (!pkLib) {
        return nullptr;
    }
    do {
        pkDesc = pkLib->GetShaderLibraryDesc();
        if (pkDesc && pkDesc->GetShaderCount() > m_uiLastDesc) {
            break;
        }
        pkLib = pkFactory->GetNextLibrary();
        m_pkLastLibrary = pkLib;
        m_uiLastDesc = 0;
    }
    while (pkLib);
    if (m_pkLastLibrary && pkDesc) {
        // Loads the index once, before the count, and passes that copy on.
        uint32_t uiDesc = m_uiLastDesc;
        if (pkDesc->GetShaderCount() > uiDesc) {
            return pkDesc->GetShaderDesc(uiDesc);
        }
    }
    return nullptr;
}

void NiSingleShaderMaterialLibrary::_SDMInit() {
    void* v0 = NiMemObject::operator new(sizeof(NiSingleShaderMaterialLibrary));
    if (v0) {
        NiSingleShaderMaterialLibrary::ms_pkThis = ::new (v0) NiSingleShaderMaterialLibrary;
    }
    else {
        NiSingleShaderMaterialLibrary::ms_pkThis = nullptr;
    }
}

void NiSingleShaderMaterialLibrary::_SDMShutdown() {
    if (NiSingleShaderMaterialLibrary::ms_pkThis) {
        delete NiSingleShaderMaterialLibrary::ms_pkThis;
    }
    NiSingleShaderMaterialLibrary::ms_pkThis = nullptr;
}
