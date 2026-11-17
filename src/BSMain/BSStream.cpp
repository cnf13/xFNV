#include "BSMain/BSStream.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSBlastNode.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSDamageStage.h"
#include "BSMain/BSDebrisNode.h"
#include "BSMain/BSDecalPlacementVectorExtraData.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSMasterParticleSystem.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundCapsule.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundSphere.h"
#include "BSMain/BSPSysArrayEmitter.h"
#include "BSMain/BSPSysMultiTargetEmitterCtlr.h"
#include "BSMain/BSPSysSimpleColorModifier.h"
#include "BSMain/BSPSysStripUpdateModifier.h"
#include "BSMain/BSParentVelocityModifier.h"
#include "BSMain/BSReference.h"
#include "BSMain/BSValueNode.h"
#include "BSMain/BSWArray.h"
#include "BSMain/BSWindModifier.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/NiBSBoneLODController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"

BSStream::BSStream() : pObjectRefMap(nullptr), spNodeReferences(nullptr) {}

// Clears the stream; spNodeReferences releases itself.
BSStream::~BSStream() {
    KillObjectRefMap();
}

bool BSStream::Save(const char* apFileName) {
    NiFile* pfile = NiFile::GetFile(apFileName, NiFile::WRITE_ONLY, 0x2800u);
    bool bres = 0;
    if (pfile) {
        bres = NiStream::Save(pfile);
        delete pfile;
    }
    return bres;
}

bool BSStream::RegisterSaveObject(NiObject* apObject) {
    if (!pObjectRefMap) {
        return NiStream::RegisterSaveObject(apObject);
    }
    NiObject* pkObject = apObject;
    uint32_t uiLinkID;
    if (m_kRegisterMap.GetAt(apObject, uiLinkID)) {
        return 0;
    }
    NiPointer<BSReference> spRef(nullptr);
    NiObjectNET* pNET;
    if (apObject) {
        pNET = apObject->IsKindOfMember(&NiObjectNET::ms_RTTI) ? static_cast<NiObjectNET*>(apObject) : 0;
    }
    else {
        pNET = 0;
    }
    NiObjectNET* pFound = 0;
    if (pNET) {
        const char* pName = pNET->m_kName.GetHandle();
        if (pObjectRefMap && pName) {
            pObjectRefMap->GetAt(pName, pFound);
        }
    }
    if (pFound) {
        BSReference* pNewRef = static_cast<BSReference*>(NiMemObject::operator new(sizeof(BSReference)));
        if (pNewRef) {
            ::new (pNewRef) BSReference(pNET->m_kName.GetHandle());
        }
        spRef = pNewRef;
        pkObject = spRef.m_pObject;
    }
    else {
        static_cast<BSNodeReferences*>(spNodeReferences.m_pObject)->AddParentRef(*this, apObject);
    }
    uiLinkID = m_kObjects.m_uiSize;
    m_kRegisterMap.SetAt(apObject, uiLinkID);
    {
        // EXE-INLINED none: NiTLargeArray::Add; it grows by m_uiGrowBy unchecked, unlike src's SetAtGrow.
        NiPointer<NiObject> spObject(pkObject);
        uint32_t uiIndex = m_kObjects.m_uiSize;
        if (uiIndex >= m_kObjects.m_uiMaxSize) {
            m_kObjects.SetSize(m_kObjects.m_uiGrowBy + uiIndex);
        }
        m_kObjects.SetAt(uiIndex, spObject);
    }
    return 1;
}

bool BSStream::LoadStream() {
    NiObject* pObject;
    if (!pObjectRefMap) {
        return NiStream::LoadStream();
    }
    if (!LoadHeader()) {
        return 0;
    }
    m_uiPostLink = 0;
    m_uiLink = 0;
    m_uiLoad = 0;
    RemoveAllObjects();
    uint32_t uiNifFileVersion = this->GetFileVersion();
    bool bNew = uiNifFileVersion >= NiStream::GetVersionFromString("5.0.0.1");
    if (bNew && !LoadRTTI()) {
        return 0;
    }
    uint32_t uiVersion = this->GetFileVersion();
    if (uiVersion >= NiStream::GetVersionFromString("5.0.0.6")) {
        LoadObjectGroups();
    }
    uint32_t uiObjects = m_kObjects.m_uiMaxSize;
    while (m_uiLoad < uiObjects) {
        if (m_eBackgroundLoadStatus == NiStream::CANCELLING) {
            return 0;
        }
        if (bNew) {
            pObject = m_kObjects.m_pBase[m_uiLoad].m_pObject;
            pObject->LoadBinary(*this);
        }
        else {
            if (!LoadObject()) {
                return 0;
            }
        }
        ++m_uiLoad;
    }
    LoadTopLevelObjects();
    while (m_uiLink < uiObjects) {
        if (m_eBackgroundLoadStatus == NiStream::CANCELLING) {
            return 0;
        }
        pObject = m_kObjects.m_pBase[m_uiLink].m_pObject;
        NiObjectNET* pNET;
        if (pObject) {
            bool bIsKind = false;
            for (const NiRTTI* pRTTI = pObject->GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
                if (pRTTI == &NiObjectNET::ms_RTTI) {
                    bIsKind = true;
                    break;
                }
            }
            pNET = bIsKind ? static_cast<NiObjectNET*>(pObject) : 0;
        }
        else {
            pNET = 0;
        }
        NiObjectNET* pFound = 0;
        if (pNET) {
            const char* pName = pNET->m_kName.GetHandle();
            if (pObjectRefMap && pName) {
                pObjectRefMap->GetAt(pName, pFound);
            }
        }
        if (!pFound) {
            pObject->LinkObject(*this);
        }
        ++m_uiLink;
    }
    while (m_uiPostLink < uiObjects) {
        if (m_eBackgroundLoadStatus == NiStream::CANCELLING) {
            return 0;
        }
        pObject = m_kObjects.m_pBase[m_uiPostLink].m_pObject;
        NiObjectNET* pNET;
        if (pObject) {
            bool bIsKind = false;
            for (const NiRTTI* pRTTI = pObject->GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
                if (pRTTI == &NiObjectNET::ms_RTTI) {
                    bIsKind = true;
                    break;
                }
            }
            pNET = bIsKind ? static_cast<NiObjectNET*>(pObject) : 0;
        }
        else {
            pNET = 0;
        }
        NiObjectNET* pFound = 0;
        if (pNET) {
            const char* pName = pNET->m_kName.GetHandle();
            if (pObjectRefMap && pName) {
                pObjectRefMap->GetAt(pName, pFound);
            }
        }
        if (!pFound) {
            pObject->PostLinkObject(*this);
        }
        ++m_uiPostLink;
    }
    SetSelectiveUpdateFlagsForOldVersions();
    FreeLoadData();
    return 1;
}

bool BSStream::SaveStream() {
    if (pObjectRefMap) {
        BSNodeReferences* pRefs = static_cast<BSNodeReferences*>(NiMemObject::operator new(sizeof(BSNodeReferences)));
        if (pRefs) {
            ::new (pRefs) BSNodeReferences;
        }
        spNodeReferences = pRefs;
        InsertObject(static_cast<NiObject*>(spNodeReferences.m_pObject));
    }
    bool bres = NiStream::SaveStream();
    spNodeReferences = static_cast<NiRefObject*>(nullptr);
    return bres;
}

bool BSStream::Load(const char* apFileName, NiFile* apFile) {
    if (!apFile) {
        apFile = NiFile::GetFile(apFileName, NiFile::READ_ONLY, 0x2800u);
    }
    bool bres = 0;
    if (apFile) {
        strcpy_s(m_acFileName, 260, apFileName);
        NiPath::StandardizeNoResult(m_acFileName);
        bres = NiStream::Load(apFile);
        delete apFile;
    }
    return bres;
}

void BSStream::FreeAllObjects() {
    NiStream::RemoveAllObjects();
    m_kLinkIDs.m_uiUsed = (uint32_t)0;
    m_kLinkIDBlocks.m_uiUsed = (uint32_t)0;
    m_uiLinkIndex = (uint32_t)0;
    m_uiLinkBlockIndex = (uint32_t)0;
    NiStream::FreeLoadData();
}

void BSStream::KillObjectRefMap() {
    if (pObjectRefMap) {
        delete pObjectRefMap;
    }
    pObjectRefMap = 0;
}

void BSStream::LoadShaderTextures(char* apcName) {
    for (const char** ppSuffix = pTextureSuffixes; *ppSuffix; ++ppSuffix) {
        char cPath[268];
        BSStream::GetModifiedTextureFilename(cPath, apcName, *ppSuffix);
        NiPointer<NiTexture> sptexture;
        BSTexturePalette::GetTexture(cPath, sptexture);
        if (!sptexture.m_pObject && NiFile::Access(cPath, NiFile::READ_ONLY)) {
            FNVD_INLINED_LOCAL(NiFixedString, kFileName, std::nullopt, std::nullopt);
            // The array's address (cPath) is never null.
            kFileName.m_kHandle = NiGlobalStringTable::AddString(cPath);
            sptexture.operator=(
                static_cast<NiTexture*>(NiSourceTexture::Create(kFileName, NiSourceTexture::ms_kDefaultPrefs, 1, 0))
            );
            // Destructor of kFileName.
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(kFileName.GetHandle());
            if (sptexture.m_pObject) {
                BSTexturePalette::SetTexture(sptexture.m_pObject, 0);
            }
        }
    }
}

void BSStream::ObjectRefRec(NiObjectNET* apObject, uint32_t aiLevels) {
    if (pObjectRefMap) {
        const char* pName = apObject->m_kName.GetHandle();
        if (pName) {
            if ((aiLevels & 128) != 0) {
                NiObjectNET* pFound;
                if (!pObjectRefMap->GetAt(pName, pFound)) {
                    pObjectRefMap->SetAt(pName, apObject);
                }
            }
            else {
                pObjectRefMap->RemoveAt(pName);
            }
        }
        if ((aiLevels & 31) != 0) {
            NiAVObject* pAVObject;
            if (apObject) {
                pAVObject = apObject->IsKindOfMember(&NiAVObject::ms_RTTI) ? static_cast<NiAVObject*>(apObject) : 0;
            }
            else {
                pAVObject = 0;
            }
            if (pAVObject) {
                if ((aiLevels & 4) != 0) {
                    NiTListItem<NiPointer<NiProperty>>* pItem = pAVObject->m_kPropertyList.m_pkHead;
                    while (pItem) {
                        NiProperty** ppProperty = &pItem->m_element.m_pObject;
                        pItem = pItem->m_pkNext;
                        if (*ppProperty) {
                            ObjectRefRec(*ppProperty, aiLevels);
                        }
                    }
                }
                NiNode* pNode = apObject->IsNode();
                if (pNode) {
                    if ((aiLevels & 1) != 0) {
                        uint32_t i = pNode->m_kChildren.m_usSize;
                        while (i--) {
                            NiObjectNET* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
                            if (pChild) {
                                if ((aiLevels & 3) == 0 || pChild->IsNode()) {
                                    ObjectRefRec(pChild, aiLevels);
                                }
                            }
                        }
                    }
                }
            }
            else {
                if ((aiLevels & 16) != 0) {
                    NiTexturingProperty* pTexProp;
                    if (apObject) {
                        bool bIsKind = false;
                        for (const NiRTTI* pRTTI = apObject->GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
                            if (pRTTI == &NiTexturingProperty::ms_RTTI) {
                                bIsKind = true;
                                break;
                            }
                        }
                        pTexProp = bIsKind ? static_cast<NiTexturingProperty*>(apObject) : 0;
                    }
                    else {
                        pTexProp = 0;
                    }
                    if (pTexProp) {
                        // EXE-INLINED 0x4BC240
                        NiTexturingProperty::Map* pBaseMap = pTexProp->m_kMaps.m_pBase[NiTexturingProperty::BASE_INDEX];
                        NiTexture* pTexture;
                        if (pBaseMap) {
                            pTexture = pBaseMap->m_spTexture.m_pObject;
                        }
                        else {
                            pTexture = 0;
                        }
                        if (pTexture) {
                            ObjectRefRec(pTexture, aiLevels);
                        }
                    }
                }
            }
        }
    }
}

void BSStream::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NiStream::CreateFunction BSStream::GetCreatorFunction(const char* apObjectName) {
    NiStream::CreateFunction pfnCreate = nullptr;
    ms_pkLoaders->GetAt(apObjectName, pfnCreate);
    return pfnCreate;
}

void BSStream::GetModifiedTextureFilename(char* apDstFileName, const char* apSrcFileName, const char* apPostfix) {
    char pcDest[264];
    strcpy_s(pcDest, 260, apSrcFileName);
    *apDstFileName = 0;
    char* pcPath = pcDest;
    // Casts away the const of the C strrchr's result, as the char* strrchr(char*, int) overload does.
    char* pcExtension = const_cast<char*>(std::strrchr(static_cast<const char*>(pcDest), '.'));
    char* pcSlash = const_cast<char*>(std::strrchr(static_cast<const char*>(pcPath), '\\'));
    char* pcUnderscore = const_cast<char*>(std::strrchr(static_cast<const char*>(pcPath), '_'));
    if (pcSlash) {
        if (pcUnderscore) {
            if (pcUnderscore < pcSlash) {
                pcUnderscore = 0;
            }
        }
    }
    if (pcExtension) {
        char Dst[12];
        strcpy_s(Dst, 10, pcExtension);
        if (pcUnderscore) {
            *pcUnderscore = 0;
        }
        *pcExtension = 0;
        if (*pcPath == '\\' || (pcPath[1] == ':' || !_strnicmp(pcPath, "data", 4u))) {
            strcpy_s(apDstFileName, 260, pcPath);
            strcat_s(apDstFileName, 260, apPostfix);
            strcat_s(apDstFileName, 260, Dst);
        }
        else {
            strcpy_s(apDstFileName, 260, "DATA\\");
            strcat_s(apDstFileName, 260, pcPath);
            strcat_s(apDstFileName, 260, apPostfix);
            strcat_s(apDstFileName, 260, Dst);
        }
    }
}

void BSStream::InitLoaders() {
    NiStream::RegisterLoader("BSReference", &BSReference::CreateObject);
    NiStream::RegisterLoader("BSNodeReferences", &BSNodeReferences::CreateObject);
    NiStream::RegisterLoader("NiBSBoneLODController", &NiBSBoneLODController::CreateObject);
    NiStream::RegisterLoader("BSBound", &BSBound::CreateObject);
    NiStream::RegisterLoader("BSXFlags", &BSXFlags::CreateObject);
    NiStream::RegisterLoader("BSDecalPlacementVectorExtraData", &BSDecalPlacementVectorExtraData::CreateObject);
    NiStream::RegisterLoader("BSWindModifier", &BSWindModifier::CreateObject);
    NiStream::RegisterLoader("BSParentVelocityModifier", &BSParentVelocityModifier::CreateObject);
    NiStream::RegisterLoader("BSFurnitureMarker", &BSFurnitureMarker::CreateObject);
    NiStream::RegisterLoader("BSPSysArrayEmitter", &BSPSysArrayEmitter::CreateObject);
    NiStream::RegisterLoader("BSWArray", &BSWArray::CreateObject);
    NiStream::RegisterLoader("BSDamageStage", &BSDamageStage::CreateObject);
    NiStream::RegisterLoader("BSBlastNode", &BSBlastNode::CreateObject);
    NiStream::RegisterLoader("BSValueNode", &BSValueNode::CreateObject);
    NiStream::RegisterLoader("BSPSysMultiTargetEmitterCtlr", &BSPSysMultiTargetEmitterCtlr::CreateObject);
    NiStream::RegisterLoader("BSMasterParticleSystem", &BSMasterParticleSystem::CreateObject);
    NiStream::RegisterLoader("BSMultiBound", &BSMultiBound::CreateObject);
    NiStream::RegisterLoader("BSMultiBoundNode", &BSMultiBoundNode::CreateObject);
    NiStream::RegisterLoader("BSMultiBoundAABB", &BSMultiBoundAABB::CreateObject);
    NiStream::RegisterLoader("BSMultiBoundOBB", &BSMultiBoundOBB::CreateObject);
    NiStream::RegisterLoader("BSMultiBoundCapsule", &BSMultiBoundCapsule::CreateObject);
    NiStream::RegisterLoader("BSMultiBoundSphere", &BSMultiBoundSphere::CreateObject);
    NiStream::RegisterLoader("BSPSysSimpleColorModifier", &BSPSysSimpleColorModifier::CreateObject);
    NiStream::RegisterLoader("BSDebrisNode", &BSDebrisNode::CreateObject);
    NiStream::RegisterLoader("BSPSysStripUpdateModifier", &BSPSysStripUpdateModifier::CreateObject);
}

void BSStream::RegisterLoadersINI(const char* apINIFilePath) {
    char ReturnedString[4100];
    char FileName[264];
    strcpy_s(FileName, 260, apINIFilePath);
    size_t uiLength = std::strlen(FileName);
    if (uiLength < 4 || FileName[uiLength - 4] != '.') {
        if (FileName[uiLength - 1] != '\\') {
            strcat_s(FileName, 260, "\\");
        }
        strcat_s(FileName, 260, "Loaders.ini");
    }
    uiLength = platform::IniGetKeys(FileName, "Loaders", " ", std::span<char>(ReturnedString, 1024));
    char* pcName = 0;
    if (uiLength) {
        for (const char* lpKeyName = ReturnedString; *lpKeyName; lpKeyName += std::strlen(lpKeyName) + 1) {
            char apObjectName[128];
            uint32_t uiSize =
                platform::IniGetString(FileName, "Loaders", lpKeyName, "", std::span<char>(apObjectName, 128));
            if (uiSize) {
                NiStream::CreateFunction pfnFunc = BSStream::GetCreatorFunction(apObjectName);
                if (pfnFunc) {
                    uiSize = std::strlen(lpKeyName);
                    pcName = static_cast<char*>(MemoryManager_Singleton.Allocate(uiSize + 1));
                    strcpy_s(pcName, uiSize + 1, lpKeyName);
                    NiStream::RegisterLoader(pcName, pfnFunc);
                }
            }
        }
    }
}

BSReference::~BSReference() {
    SetName(nullptr);
}
