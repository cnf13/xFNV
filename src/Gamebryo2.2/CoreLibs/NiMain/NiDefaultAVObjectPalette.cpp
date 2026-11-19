#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

NiRTTI NiDefaultAVObjectPalette::ms_RTTI("NiDefaultAVObjectPalette", &NiAVObjectPalette::ms_RTTI);
namespace {

    using AVObjectMap = NiTFixedStringMap<NiAVObject*>;
    using AVObjectItem = NiTMapItem<NiFixedString, NiAVObject*>;

    // GetFirstPos: the head of the first non-empty bucket.
    AVObjectItem* AVObjectMap_FirstPosInline(const AVObjectMap& arMap) {
        for (uint32_t ui = 0; ui < arMap.m_uiHashSize; ++ui) {
            if (arMap.m_ppkHashTable[ui]) {
                return arMap.m_ppkHashTable[ui];
            }
        }

        return nullptr;
    }

    // Gives back the reference a local name holds. Each local's own destructor releases it, so this does nothing.
    void ReleaseName(const NiFixedString& akName) {}

} // namespace

NiDefaultAVObjectPalette::NiDefaultAVObjectPalette() : m_kHash(37), m_pkScene(nullptr) {}

NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(NiAVObject* pkScene, uint32_t uiHashSize)
  : m_kHash(uiHashSize), m_pkScene(pkScene) {
    ResetAndFillFromScenegraph();
}

NiDefaultAVObjectPalette::~NiDefaultAVObjectPalette() {}

const NiRTTI* NiDefaultAVObjectPalette::GetRTTI() const {
    return &ms_RTTI;
}

// NiImplementCreateClone: the default constructor (hash size 37), then CopyMembers, which is NiAVObjectPalette's.
// CopyMembers is called even when the allocation fails, as the game does.
NiObject* NiDefaultAVObjectPalette::CreateClone(NiCloningProcess& kCloning) {
    NiDefaultAVObjectPalette* pkClone =
        static_cast<NiDefaultAVObjectPalette*>(NiMemObject::operator new(sizeof(NiDefaultAVObjectPalette)));
    if (!pkClone) {
        NiAVObjectPalette::CopyMembers(nullptr, kCloning);
        return nullptr;
    }
    ::new (pkClone) NiDefaultAVObjectPalette;
    NiAVObjectPalette::CopyMembers(pkClone, kCloning);
    return pkClone;
}

// Pairs whose name or object is missing are skipped.
void NiDefaultAVObjectPalette::LoadBinary(NiStream& stream) {
    NiAVObjectPalette::LoadBinary(stream);
    m_pkScene = static_cast<NiAVObject*>(stream.ResolveLinkID());

    uint32_t uiCount = 0;
    NiStreamLoadBinary(stream, uiCount);
    while (uiCount--) {
        char* pcName = nullptr;
        stream.LoadCString(pcName);
        NiAVObject* pkObject = static_cast<NiAVObject*>(stream.ResolveLinkID());
        if (pcName && pkObject) {
            NiFixedString kName(pcName);
            SetAVObject(kName, pkObject);
            ReleaseName(kName);
        }

        _NiFree(pcName);
    }
}

// Objects whose own name is not their key are entered again under their name: the renames are gathered in a local map
// first, then each object is moved from its old key to its new one.
void NiDefaultAVObjectPalette::LinkObject(NiStream& stream) {
    NiAVObjectPalette::LinkObject(stream);

    using RenameMap = NiTFixedStringMap<const char*>;
    using RenameItem = NiTMapItem<NiFixedString, const char*>;
    RenameMap kRenames(37);

    AVObjectItem* pkPos = AVObjectMap_FirstPosInline(m_kHash);
    while (pkPos) {
        NiFixedString kName;
        NiAVObject* pkObject;
        m_kHash.GetNext(pkPos, kName, pkObject);
        const char* pcName = kName.GetHandle();
        if (pcName && pkObject) {
            const char* pcObjectName = pkObject->m_kName.GetHandle();
            if (pcObjectName) {
                // EXE-INLINED 0xEC6DA0
                const char* pcA = pcName;
                const char* pcB = pcObjectName;
                while (*pcA == *pcB && *pcA) {
                    pcA++;
                    pcB++;
                }
                if (*pcA != *pcB) {
                    kRenames.SetAt(kName, pcObjectName);
                }
            }
        }
    }

    RenameItem* pkRenamePos = nullptr;
    for (uint32_t ui = 0; ui < kRenames.m_uiHashSize; ++ui) {
        if (kRenames.m_ppkHashTable[ui]) {
            pkRenamePos = kRenames.m_ppkHashTable[ui];
            break;
        }
    }
    while (pkRenamePos) {
        NiFixedString kOldName;
        const char* pcNewName;
        NiAVObject* pkObject = nullptr;
        kRenames.GetNext(pkRenamePos, kOldName, pcNewName);
        const char* pcOldName = kOldName.GetHandle();
        if (pcOldName && pcNewName) {
            NiFixedString kNewName(pcNewName);
            // EXE-INLINED none: NiTFixedStringMapBase::GetAt
            // The bucket is the string handle's address modulo the table size (Gamebryo's KeyToHashIndex).
            uint32_t uiIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcOldName) % m_kHash.m_uiHashSize);
            for (AVObjectItem* pkItem = m_kHash.m_ppkHashTable[uiIndex]; pkItem; pkItem = pkItem->m_pkNext) {
                if (pcOldName == pkItem->m_key.GetHandle()) {
                    pkObject = pkItem->m_val;
                    break;
                }
            }
            m_kHash.SetAt(kNewName, pkObject);
            m_kHash.RemoveAt(kOldName);
        }
    }
}

// A jump to the NiAVObjectPalette body.
bool NiDefaultAVObjectPalette::RegisterStreamables(NiStream& stream) {
    return NiAVObjectPalette::RegisterStreamables(stream);
}

void NiDefaultAVObjectPalette::SaveBinary(NiStream& stream) {
    NiAVObjectPalette::SaveBinary(stream);
    stream.SaveLinkID(m_pkScene);

    uint32_t uiCount = m_kHash.m_kAllocator.m_uiCount;
    NiStreamSaveBinary(stream, uiCount);
    if (!m_kHash.m_kAllocator.m_uiCount) {
        return;
    }

    AVObjectItem* pkPos = AVObjectMap_FirstPosInline(m_kHash);
    while (pkPos) {
        NiFixedString kName;
        NiAVObject* pkObject;
        m_kHash.GetNext(pkPos, kName, pkObject);
        stream.SaveCString(kName);
        stream.SaveLinkID(pkObject);
        ReleaseName(kName);
    }
}

// Equal when the other palette finds an object under every name of this one; the objects are not compared.
bool NiDefaultAVObjectPalette::IsEqual(NiObject* pObject) {
    if (!NiAVObjectPalette::IsEqual(pObject)) {
        return false;
    }

    NiDefaultAVObjectPalette* pkPalette = static_cast<NiDefaultAVObjectPalette*>(pObject);
    if (pkPalette->m_kHash.m_kAllocator.m_uiCount != m_kHash.m_kAllocator.m_uiCount) {
        return false;
    }

    if (!m_kHash.m_kAllocator.m_uiCount) {
        return true;
    }

    AVObjectItem* pkPos = m_kHash.GetFirstPos();
    while (pkPos) {
        NiFixedString kName;
        NiAVObject* pkObject;
        m_kHash.GetNext(pkPos, kName, pkObject);
        if (!pkPalette->GetAVObject(kName)) {
            ReleaseName(kName);
            return false;
        }

        ReleaseName(kName);
    }

    return true;
}

// The scene root is read without a null check, as the game does. Allocate is called on the singleton directly.
void NiDefaultAVObjectPalette::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiAVObjectPalette::GetViewerStrings(pStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    pStrings->Add(pcString);

    pcString = static_cast<char*>(MemoryManager_Singleton.Allocate(0x60));
    const char* pcSceneName = m_pkScene->m_kName;
    if (!pcSceneName) {
        pcSceneName = "UNKNOWN";
    }

    std::sprintf(pcString, "m_pkScene = %s", pcSceneName);
    pStrings->Add(pcString);
    if (!m_kHash.m_kAllocator.m_uiCount) {
        return;
    }

    AVObjectItem* pkPos = AVObjectMap_FirstPosInline(m_kHash);
    while (pkPos) {
        NiFixedString kName;
        NiAVObject* pkObject;
        m_kHash.GetNext(pkPos, kName, pkObject);
        pcString = static_cast<char*>(MemoryManager_Singleton.Allocate(0x60));
        std::sprintf(pcString, "%s", static_cast<const char*>(kName));
        pStrings->Add(pcString);
        ReleaseName(kName);
    }
}

// The clone's scene is the scene's clone; each named object with a clone is entered in the clone palette under its
// name. The clone itself is looked up without a check.
void NiDefaultAVObjectPalette::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiDefaultAVObjectPalette* pkDest = static_cast<NiDefaultAVObjectPalette*>(pkObject);
    NiAVObject* pkScene = m_pkScene;
    if (pkScene) {
        kCloning.m_pkCloneMap->GetAt(pkScene, pkObject);
        pkDest->m_pkScene = static_cast<NiAVObject*>(pkObject);
    }

    if (!m_kHash.m_kAllocator.m_uiCount) {
        return;
    }

    AVObjectItem* pkPos = AVObjectMap_FirstPosInline(m_kHash);
    while (pkPos) {
        NiFixedString kName;
        NiAVObject* pkValue;
        m_kHash.GetNext(pkPos, kName, pkValue);
        if (kName.GetHandle() && pkValue) {
            NiObject* pkClonedValue;
            if (kCloning.m_pkCloneMap->GetAt(pkValue, pkClonedValue)) {
                pkDest->SetAVObject(kName, static_cast<NiAVObject*>(pkClonedValue));
            }
        }
    }
}

// A null name finds nothing.
NiAVObject* NiDefaultAVObjectPalette::GetAVObject(const NiFixedString& arName) {
    const char* pcHandle = arName.GetHandle();
    if (!pcHandle) {
        return nullptr;
    }

    uint32_t uiIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcHandle) % m_kHash.m_uiHashSize);
    for (AVObjectItem* pkItem = m_kHash.m_ppkHashTable[uiIndex]; pkItem; pkItem = pkItem->m_pkNext) {
        if (pkItem->m_key.GetHandle() == pcHandle) {
            return pkItem->m_val;
        }
    }

    return nullptr;
}

// A null object removes the name.
void NiDefaultAVObjectPalette::SetAVObject(NiFixedString& arName, NiAVObject* pkObj) {
    if (!pkObj) {
        m_kHash.RemoveAt(arName);
    }
    else {
        m_kHash.SetAt(arName, pkObj);
    }
}

// Empties the palette and refills it from the scene; does nothing without a scene.
void NiDefaultAVObjectPalette::ResetAndFillFromScenegraph() {
    if (m_pkScene) {
        m_kHash.RemoveAll();
        RecurseAndAddObjectsToPalette(m_pkScene, this);
    }
}

NiObject* NiDefaultAVObjectPalette::CreateObject() {
    NiDefaultAVObjectPalette* pkObject =
        static_cast<NiDefaultAVObjectPalette*>(NiMemObject::operator new(sizeof(NiDefaultAVObjectPalette)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiDefaultAVObjectPalette;
}

void RecurseAndAddObjectsToPalette(NiAVObject* apObj3D, NiDefaultAVObjectPalette* apPalette) {
    if ((apObj3D) && (apPalette)) {
        if (apObj3D->m_kName.GetHandle()) {
            apPalette->SetAVObject(apObj3D->m_kName, apObj3D);
        }
        NiNode* v2 = apObj3D->IsNode();
        if (v2) {
            uint32_t v4 = 0;
            if (v2->m_kChildren.m_usSize) {
                do {
                    RecurseAndAddObjectsToPalette(v2->m_kChildren.m_pBase[v4++].m_pObject, apPalette);
                }
                while (v4 < v2->m_kChildren.m_usSize);
            }
        }
    }
}

void RecurseAndRemoveObjectsFromPalette(NiAVObject* apObj3D, NiDefaultAVObjectPalette* apPalette) {
    if ((apObj3D) && (apPalette)) {
        if (apObj3D->m_kName.GetHandle()) {
            apPalette->SetAVObject(apObj3D->m_kName, nullptr);
        }
        NiNode* v2 = apObj3D->IsNode();
        if (v2) {
            uint32_t v4 = 0;
            if (v2->m_kChildren.m_usSize) {
                do {
                    RecurseAndRemoveObjectsFromPalette(v2->m_kChildren.m_pBase[v4++].m_pObject, apPalette);
                }
                while (v4 < v2->m_kChildren.m_usSize);
            }
        }
    }
}
