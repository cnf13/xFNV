#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequenceStreamHelper.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStringExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"

NiRTTI NiSequence::ms_RTTI("NiSequence", &NiObject::ms_RTTI);

NiSequence::NiSequence(const char* pcName, uint32_t uiInitSize, uint32_t uiGrowBy)
  : m_pcName(nullptr), m_spTextKeys(nullptr), m_uiTextKeyReference(0) {
    if (pcName) {
        size_t stLength = 0;
        while (pcName[stLength]) {
            ++stLength;
        }
        ++stLength;
        m_pcName = static_cast<char*>(_NiMalloc(stLength));
        strcpy_s(m_pcName, stLength, pcName);
    }
    m_kObjectName.SetSize(uiInitSize);
    m_kObjectName.m_usGrowBy = static_cast<uint16_t>(uiGrowBy);
    m_kCtrl.SetSize(uiInitSize);
    m_kCtrl.m_usGrowBy = static_cast<uint16_t>(uiGrowBy);
}

// The member arrays and m_spTextKeys are released by their own destructors.
NiSequence::~NiSequence() {
    _NiFree(m_pcName);
    for (uint32_t i = 0; i < m_kCtrl.m_usSize; ++i) {
        m_kCtrl.RemoveAt(i) = nullptr;
        _NiFree(m_kObjectName.RemoveAt(i));
    }
}

const NiRTTI* NiSequence::GetRTTI() const {
    return &ms_RTTI;
}

// Each object name is followed by the link ID of its controller.
void NiSequence::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    m_pcName = nullptr;
    kStream.LoadCString(m_pcName);
    NiStreamLoadBinary(kStream, m_uiTextKeyReference);
    kStream.ReadLinkID();

    uint32_t uiCount;
    NiStreamLoadBinary(kStream, uiCount);
    m_kObjectName.SetSize(uiCount);
    for (uint32_t ui = 0; ui < uiCount; ++ui) {
        char* pcObjectName = nullptr;
        kStream.LoadCString(pcObjectName);
        // EXE-INLINED 0x96AE90
        if (ui >= m_kObjectName.m_usSize) {
            m_kObjectName.m_usSize = static_cast<uint16_t>(ui + 1);
            if (pcObjectName) {
                ++m_kObjectName.m_usESize;
            }
        }
        else if (pcObjectName) {
            if (!m_kObjectName.m_pBase[ui]) {
                ++m_kObjectName.m_usESize;
            }
        }
        else if (m_kObjectName.m_pBase[ui]) {
            --m_kObjectName.m_usESize;
        }
        m_kObjectName.m_pBase[ui] = pcObjectName;
        kStream.ReadLinkID();
    }
}

// The linked controllers are stored without a type check.
void NiSequence::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    m_spTextKeys = kStream.GetObjectFromLinkID();

    uint32_t uiCount = m_kObjectName.GetSize();
    m_kCtrl.SetSize(uiCount);
    for (uint32_t ui = 0; ui < uiCount; ++ui) {
        NiPointer<NiTimeController> spCtrl = static_cast<NiTimeController*>(kStream.GetObjectFromLinkID());
        m_kCtrl.SetAt(ui, spCtrl);
    }
}

bool NiSequence::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiSequence::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
}

bool NiSequence::IsEqual(NiObject* pkObject) {
    return NiObject::IsEqual(pkObject);
}

// KEEP: the exe reads the name count into the slot its `push ecx` (0xA47660) set to this object's address (lea
// ecx,[esp+0x18] at 0xA476C1), so a short read (malformed data) sizes the arrays by that address, a value src cannot
// express; porting.md's unwritten-slot exception covers GetChunkData only.
// Frees the old name, then allocates strlen + 1 bytes for the new one. The exe inlines the strlen scan as a byte
// loop, so src scans too rather than calling the CRT.
void NiSequence::SetName(const char* pcName) {
    _NiFree(m_pcName);
    m_pcName = nullptr;
    if (pcName) {
        size_t stLength = 0;
        while (pcName[stLength]) {
            ++stLength;
        }
        ++stLength;
        m_pcName = static_cast<char*>(_NiMalloc(stLength));
        strcpy_s(m_pcName, stLength, pcName);
    }
}

// Both arrays grow as SetAtGrow does. The controller goes in through a temporary smart pointer, taken before the second
// array's size check and released after its SetAt.
uint32_t NiSequence::AddNameControllerPair(const char* pcName, NiTransformController* pkController) {
    uint32_t uiIndex = m_kObjectName.GetSize();
    // EXE-INLINED none: strlen
    size_t stLength = 0;
    while (pcName[stLength]) {
        ++stLength;
    }
    ++stLength;
    char* pcNameCopy = static_cast<char*>(_NiMalloc(stLength));
    strcpy_s(pcNameCopy, stLength, pcName);
    // EXE-INLINED none: NiTArray<char*>::SetAtGrow
    if (uiIndex >= m_kObjectName.m_usMaxSize) {
        m_kObjectName.SetSize(uiIndex + m_kObjectName.m_usGrowBy);
    }
    m_kObjectName.SetAt(uiIndex, pcNameCopy);
    NiPointer<NiTimeController> spController = pkController;
    // EXE-INLINED none: NiTArray<NiPointer<NiTimeController>>::SetAtGrow
    if (uiIndex >= m_kCtrl.m_usMaxSize) {
        m_kCtrl.SetSize(uiIndex + m_kCtrl.m_usGrowBy);
    }
    m_kCtrl.SetAt(uiIndex, spController);
    return uiIndex;
}

// Loads a sequence from a KF stream: files older than 4.1.0.3 are converted, newer ones take the object at uiIndex. The
// out parameter is the caller's smart pointer: the found sequence takes a reference, the caller's old object is
// released, and the local reference is dropped before returning.
void NiSequence::CreateFromKFFile(
    const NiStream& kStream,
    uint32_t uiIndex,
    NiPointer<NiSequence>& aspSequence,
    const char* pcSequenceName
) {
    NiPointer<NiSequence> spSequence;
    if (kStream.m_uiNifFileVersion < NiStream::GetVersion(4, 1, 0, 3)) {
        NiSequence* pkConverted = ConvertSequence(kStream, uiIndex, pcSequenceName);
        spSequence.m_pObject = pkConverted;
        if (pkConverted) {
            pkConverted->IncRefCount();
        }
    }
    else if (uiIndex >= kStream.m_kTopObjects.m_uiSize) {
        // Out of range: releases the caller's pointer and returns.
        aspSequence = nullptr;
        return;
    }
    else {
        NiObject* pkObject = kStream.m_kTopObjects.m_pBase[uiIndex].m_pObject;
        NiSequence* pkCast = nullptr;
        if (pkObject) {
            pkCast = static_cast<NiSequence*>(pkObject->DynamicCastMember(&ms_RTTI));
        }
        spSequence.SetNew(pkCast);
    }
    if (!spSequence.m_pObject) {
        aspSequence = nullptr;
        return;
    }
    if (pcSequenceName) {
        spSequence.m_pObject->SetName(pcSequenceName);
    }
    aspSequence = spSequence;
}

// The text keys move to the sequence, then each string extra data names the next controller of the container's chain:
// the pair is added and the controller is unlinked from the chain. A failed allocation is stored as a null sequence and
// written through, as the engine does.
NiSequence* NiSequence::ConvertSequence(const NiStream& kStream, uint32_t uiIndex, const char* pcSequenceName) {
    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 3) || uiIndex >= kStream.m_kTopObjects.m_uiSize) {
        return nullptr;
    }
    NiObject* pkObject = kStream.m_kTopObjects.m_pBase[uiIndex].m_pObject;
    if (!pkObject) {
        return nullptr;
    }
    NiSequenceStreamHelper* pkContainer =
        static_cast<NiSequenceStreamHelper*>(pkObject->DynamicCastMember(&NiSequenceStreamHelper::ms_RTTI));
    if (!pkContainer) {
        return nullptr;
    }
    if (!pcSequenceName) {
        pcSequenceName = pkContainer->m_kName.GetHandle();
    }
    NiSequence* pkSequence;
    NiSequence* pkNew = static_cast<NiSequence*>(NiMemObject::operator new(sizeof(NiSequence)));
    if (pkNew) {
        pkSequence = ::new (pkNew) NiSequence(pcSequenceName, 12u, 12u);
    }
    else {
        pkSequence = nullptr;
    }
    pkSequence->m_uiTextKeyReference = 0;

    for (uint32_t i = 0; i < pkContainer->m_usExtraDataSize; ++i) {
        NiExtraData* pkExtra = static_cast<NiObjectNET&>(*pkContainer).m_ppkExtra[static_cast<uint16_t>(i)];
        if (!pkExtra) {
            continue;
        }
        NiObject* pkTextKeys = pkExtra->DynamicCastMember(&NiTextKeyExtraData::ms_RTTI);
        if (pkTextKeys) {
            SetTextKeys(pkSequence, nullptr, pkTextKeys);
            pkContainer->RemoveExtraDataAt(i);
            break;
        }
    }

    NiTimeController* pkCtrl = pkContainer->m_spControllers;
    if (pkCtrl) {
        NiIncRefCount(pkCtrl);
    }
    for (uint32_t i = 0; i < pkContainer->m_usExtraDataSize; ++i) {
        // EXE-INLINED none: NiDynamicCast(NiStringExtraData, pkExtra), an IsKindOf walk of the RTTI chain
        NiExtraData* pkExtra = static_cast<NiObjectNET&>(*pkContainer).m_ppkExtra[static_cast<uint16_t>(i)];
        NiStringExtraData* pkString = nullptr;
        if (pkExtra) {
            for (const NiRTTI* pkRTTI = pkExtra->GetRTTI(); pkRTTI; pkRTTI = pkRTTI->GetBaseRTTI()) {
                if (pkRTTI == &NiStringExtraData::ms_RTTI) {
                    pkString = static_cast<NiStringExtraData*>(pkExtra);
                    break;
                }
            }
        }
        // EXE-INLINED none: NiDynamicCast(NiTransformController, pkCtrl)
        NiTransformController* pkController = nullptr;
        if (pkCtrl) {
            for (const NiRTTI* pkRTTI = pkCtrl->GetRTTI(); pkRTTI; pkRTTI = pkRTTI->GetBaseRTTI()) {
                if (pkRTTI == &NiTransformController::ms_RTTI) {
                    pkController = static_cast<NiTransformController*>(pkCtrl);
                    break;
                }
            }
        }
        pkSequence->AddNameControllerPair(pkString->m_kString.GetHandle(), pkController);

        // Equivalent to spNext = pkCtrl->GetNext(); pkCtrl->SetNext(0); pkCtrl = spNext.
        NiTimeController* pkNext = pkCtrl->m_spNext.m_pObject;
        if (pkNext) {
            NiIncRefCount(pkNext);
        }
        pkCtrl->m_spNext = nullptr;
        if (pkCtrl != pkNext) {
            NiDecRefCount(pkCtrl);
            pkCtrl = pkNext;
            if (!pkNext) {
                continue;
            }
            NiIncRefCount(pkNext);
        }
        if (pkNext) {
            NiDecRefCount(pkNext);
        }
    }
    pkContainer->RemoveAllExtraData();
    if (pkCtrl) {
        NiDecRefCount(pkCtrl);
    }
    return pkSequence;
}

NiObject* NiSequence::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiSequence));
    if (v0) {
        return ::new (v0) NiSequence(nullptr, 0xCu, 0xCu);
    }
    else {
        return nullptr;
    }
}

void NiSequence::CreateFromKFFile(
    const char* pcSequenceFile,
    uint32_t uiIndex,
    NiPointer<NiSequence>& aspSequence,
    const char* pcSequenceName
) {
    NiStream kStream;
    if (kStream.NiStream::Load(pcSequenceFile)) {
        CreateFromKFFile(kStream, uiIndex, aspSequence, pcSequenceName);
    }
    else {
        aspSequence = nullptr;
    }
}

void SetTextKeys(NiSequence* apThis, [[maybe_unused]] void* apEdx, NiRefObject* a2) {
    NiRefObject* m_pObject;
    m_pObject = apThis->m_spTextKeys.m_pObject;
    if (m_pObject != a2) {
        if (m_pObject) {
            if (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount)) {
                m_pObject->DeleteThis();
            }
        }
        apThis->m_spTextKeys.m_pObject = a2;
        if (a2) {
            InterlockedIncrementUInt32(&a2->m_uiRefCount);
        }
    }
}
