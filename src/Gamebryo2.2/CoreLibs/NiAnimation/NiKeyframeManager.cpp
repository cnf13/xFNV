#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyframeManager.h"

#include "BSCore/MemoryManager.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiRTTI NiKeyframeManager::ms_RTTI("NiKeyframeManager", &NiTimeController::ms_RTTI);

NiKeyframeManager::LinkRecord::LinkRecord()
  : m_kTextKeyReferences(0, 1), m_kSequenceNames(0, 1), m_kNumSequenceKeys(0, 1), m_kObjectName(0, 1) {}

NiKeyframeManager::NiKeyframeManager() : m_kMap(37, false) {}

NiKeyframeManager::~NiKeyframeManager() {}

// Each value is read through NiStreamLoadBinary (one component of the value's size). uiSizeOrFlag holds both the first
// read's component size and each sequence's "from file" byte: the byte is read over the size.
void NiKeyframeManager::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);
    NiBinaryStream* pkIstr = kStream.Istr();
    uint32_t uiSizeOrFlag = 4;
    if (kStream.GetFileVersion() < NiStream::GetVersion(4, 1, 0, 3)) {
        uint32_t uiNumSequences;
        pkIstr->m_uiAbsoluteCurrentPos += pkIstr->m_pfnRead(pkIstr, &uiNumSequences, 4, &uiSizeOrFlag, 1);
        while (uiNumSequences) {
            NiBinaryStream* pkFlagIstr = kStream.Istr();
            --uiNumSequences;
            uint32_t uiFlagSize = 1;
            pkFlagIstr->m_uiAbsoluteCurrentPos += pkFlagIstr->m_pfnRead(pkFlagIstr, &uiSizeOrFlag, 1, &uiFlagSize, 1);
            char* pcSequenceName = nullptr;
            kStream.LoadCString(pcSequenceName);
            if (static_cast<uint8_t>(uiSizeOrFlag)) {
                char* pcFileName = nullptr;
                kStream.LoadCString(pcFileName);
                NiBinaryStream* pkIndexIstr = kStream.Istr();
                uint32_t uiSize = 4;
                uint32_t uiIndex;
                pkIndexIstr->m_uiAbsoluteCurrentPos += pkIndexIstr->m_pfnRead(pkIndexIstr, &uiIndex, 4, &uiSize, 1);
                Add(pcFileName, pcSequenceName, uiIndex);
                _NiFree(pcFileName);
                _NiFree(pcSequenceName);
            }
            else {
                LinkRecord* pkRecord = static_cast<LinkRecord*>(MemoryManager_Singleton.Allocate(sizeof(LinkRecord)));
                if (pkRecord) {
                    pkRecord = ::new (pkRecord) LinkRecord;
                }
                ms_pkLinkRecord = pkRecord;
                NiTPrimitiveArray<char*>& kSequenceNames = pkRecord->m_kSequenceNames;
                kSequenceNames.Add(pcSequenceName);
                NiBinaryStream* pkRefIstr = kStream.Istr();
                uint32_t uiSize = 4;
                uint32_t uiTextKeyReference;
                pkRefIstr->m_uiAbsoluteCurrentPos +=
                    pkRefIstr->m_pfnRead(pkRefIstr, &uiTextKeyReference, 4, &uiSize, 1);
                NiTPrimitiveArray<uint32_t>& kTextKeyReferences = ms_pkLinkRecord->m_kTextKeyReferences;
                kTextKeyReferences.Add(uiTextKeyReference);
                kStream.ReadLinkID();
                NiBinaryStream* pkKeysIstr = kStream.Istr();
                uiSize = 4;
                uint32_t uiNumKeys;
                pkKeysIstr->m_uiAbsoluteCurrentPos += pkKeysIstr->m_pfnRead(pkKeysIstr, &uiNumKeys, 4, &uiSize, 1);
                NiTPrimitiveArray<uint32_t>& kNumSequenceKeys = ms_pkLinkRecord->m_kNumSequenceKeys;
                kNumSequenceKeys.Add(uiNumKeys);
                while (uiNumKeys) {
                    --uiNumKeys;
                    char* pcObjectName = nullptr;
                    kStream.LoadCString(pcObjectName);
                    NiTPrimitiveArray<char*>& kObjectName = ms_pkLinkRecord->m_kObjectName;
                    uint32_t uiObjectAt = kObjectName.m_usSize;
                    if (uiObjectAt >= kObjectName.m_usMaxSize) {
                        kObjectName.SetSize(uiObjectAt + kObjectName.m_usGrowBy);
                    }
                    // EXE-INLINED 0x96AE90
                    if (uiObjectAt < kObjectName.m_usSize) {
                        if (pcObjectName) {
                            if (!kObjectName.m_pBase[uiObjectAt]) {
                                ++kObjectName.m_usESize;
                            }
                        }
                        else if (kObjectName.m_pBase[uiObjectAt]) {
                            --kObjectName.m_usESize;
                        }
                    }
                    else {
                        kObjectName.m_usSize = static_cast<uint16_t>(uiObjectAt + 1);
                        if (pcObjectName) {
                            ++kObjectName.m_usESize;
                        }
                    }
                    kObjectName.m_pBase[uiObjectAt] = pcObjectName;
                    kStream.ReadLinkID();
                }
            }
        }
    }
    else {
        uint32_t uiNumLinkIDs = 0;
        uint32_t uiNumSequences;
        pkIstr->m_uiAbsoluteCurrentPos += pkIstr->m_pfnRead(pkIstr, &uiNumSequences, 4, &uiSizeOrFlag, 1);
        while (uiNumSequences) {
            NiBinaryStream* pkFlagIstr = kStream.Istr();
            --uiNumSequences;
            uint32_t uiSize = 1;
            pkFlagIstr->m_uiAbsoluteCurrentPos += pkFlagIstr->m_pfnRead(pkFlagIstr, &uiSizeOrFlag, 1, &uiSize, 1);
            if (static_cast<uint8_t>(uiSizeOrFlag)) {
                char* pcSequenceName = nullptr;
                kStream.LoadCString(pcSequenceName);
                char* pcFileName = nullptr;
                kStream.LoadCString(pcFileName);
                NiBinaryStream* pkIndexIstr = kStream.Istr();
                uiSize = 4;
                uint32_t uiIndex;
                pkIndexIstr->m_uiAbsoluteCurrentPos += pkIndexIstr->m_pfnRead(pkIndexIstr, &uiIndex, 4, &uiSize, 1);
                Add(pcFileName, pcSequenceName, uiIndex);
                _NiFree(pcSequenceName);
                _NiFree(pcFileName);
            }
            else {
                kStream.ReadLinkID();
                ++uiNumLinkIDs;
            }
        }
        kStream.SetNumberOfLinkIDs(uiNumLinkIDs);
    }
}

// Files from 4.1.0.3 on link each sequence by ID. Older ones build the sequences from the record LoadBinary filled
// (name, text key reference, text keys, then the sequence's name/controller pairs), then delete the record.
void NiKeyframeManager::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 3)) {
        uint32_t uiSize = kStream.GetNumberOfLinkIDs();
        for (uint32_t ui = uiSize; ui != 0; ui--) {
            NiSequence* pkSequence = static_cast<NiSequence*>(kStream.GetObjectFromLinkID());
            m_kMap.SetAt(pkSequence->m_pcName, NiPointer<NiSequence>(pkSequence));
        }
        return;
    }

    LinkRecord* pkRecord = ms_pkLinkRecord;
    uint32_t uiNumSequences = pkRecord->m_kNumSequenceKeys.m_usSize;
    uint32_t uiKey = 0;
    for (uint32_t ui = 0; ui < uiNumSequences; ui++) {
        char* pcName = pkRecord->m_kSequenceNames.m_pBase[ui];
        NiSequence* pkMemory = static_cast<NiSequence*>(NiMemObject::operator new(sizeof(NiSequence)));
        NiSequence* pkSequence = pkMemory ? ::new (pkMemory) NiSequence(pcName, 12, 12) : nullptr;
        _NiFree(pcName);
        pkSequence->m_uiTextKeyReference = ms_pkLinkRecord->m_kTextKeyReferences.m_pBase[ui];
        pkSequence->m_spTextKeys = kStream.GetObjectFromLinkID();

        uint32_t uiNumKeys = ms_pkLinkRecord->m_kNumSequenceKeys.m_pBase[ui];
        while (uiNumKeys) {
            char* pcObjectName = ms_pkLinkRecord->m_kObjectName.m_pBase[uiKey];
            uiNumKeys--;
            uiKey++;
            NiTransformController* pkController = static_cast<NiTransformController*>(kStream.GetObjectFromLinkID());
            pkSequence->AddNameControllerPair(pcObjectName, pkController);
            _NiFree(pcObjectName);
        }

        m_kMap.SetAt(pkSequence->m_pcName, NiPointer<NiSequence>(pkSequence));
        pkRecord = ms_pkLinkRecord;
    }

    if (pkRecord) {
        // EXE-INLINED none: NiKeyframeManager::LinkRecord::~LinkRecord, the four NiTArray destructors (inlined too) in
        // reverse member order.
        FNVD_DESTRUCT_AS(DestructInlined, NiTArray<char*, NiTMallocInterface<char*>>)(&pkRecord->m_kObjectName);
        FNVD_DESTRUCT_AS(DestructInlined, NiTArray<uint32_t, NiTMallocInterface<uint32_t>>)(
            &pkRecord->m_kNumSequenceKeys
        );
        FNVD_DESTRUCT_AS(DestructInlined, NiTArray<char*, NiTMallocInterface<char*>>)(&pkRecord->m_kSequenceNames);
        FNVD_DESTRUCT_AS(DestructInlined, NiTArray<uint32_t, NiTMallocInterface<uint32_t>>)(
            &pkRecord->m_kTextKeyReferences
        );
        MemoryManager_Singleton.Deallocate(pkRecord);
    }
    ms_pkLinkRecord = nullptr;
}

bool NiKeyframeManager::RegisterStreamables(NiStream& kStream) {
    return NiTimeController::RegisterStreamables(kStream);
}

void NiKeyframeManager::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
}

bool NiKeyframeManager::IsEqual(NiObject* pkObject) {
    return NiTimeController::IsEqual(pkObject);
}

bool NiKeyframeManager::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

bool NiKeyframeManager::Add(const char* pcSequenceFile, const char* pcSequenceName, uint32_t uiIndex) {
    NiPointer<NiSequence> spSequence;
    NiSequence::CreateFromKFFile(pcSequenceFile, uiIndex, spSequence, pcSequenceName);
    if (!spSequence.m_pObject) {
        return false;
    }
    if (pcSequenceName) {
        spSequence.m_pObject->SetName(pcSequenceName);
    }
    m_kMap.SetAt(spSequence.m_pObject->m_pcName, spSequence);
    return true;
}

NiObject* NiKeyframeManager::CreateObject() {
    NiKeyframeManager* pkObject = static_cast<NiKeyframeManager*>(NiMemObject::operator new(sizeof(NiKeyframeManager)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiKeyframeManager;
}
