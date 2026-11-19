#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNoteListener.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiRTTI NiControllerManager::ms_RTTI("NiControllerManager", &NiTimeController::ms_RTTI);

NiControllerManager::NiControllerManager()
  : m_kSequenceArray(0, 10), m_kStringMap(37, false), m_bCumulative(false), m_spObjectPalette(nullptr) {
    pListener = new BSAnimNoteListener;
}

NiControllerManager::NiControllerManager(NiAVObject* pkTarget, bool bCumulativeAnimations)
  : m_kSequenceArray(0, 10), m_kStringMap(37, false), m_bCumulative(bCumulativeAnimations), m_spObjectPalette(nullptr) {
    NiTimeController::SetTarget(pkTarget);
    m_spObjectPalette = new NiDefaultAVObjectPalette(pkTarget, 79u);
    pListener = new BSAnimNoteListener;
}

NiControllerManager::~NiControllerManager() {
    m_kSequenceArray.RemoveAll();
    m_kTempBlendSeqs.RemoveAll();
    delete pListener;
}

NiObject* NiControllerManager::CreateClone(NiCloningProcess& kCloning) {
    NiControllerManager* v4 = new NiControllerManager;
    CopyMembers(v4, kCloning);
    return v4;
}

// The object palette is in files from 10.1.0.104 on.
void NiControllerManager::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);

    // If the read stores nothing, ucCumulative deliberately holds the low byte of the stream pointer, as the game does.
    uint8_t ucCumulative = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(&kStream));
    NiStreamLoadBinary(kStream, ucCumulative);
    m_bCumulative = ucCumulative != 0;

    kStream.ReadMultipleLinkIDs();

    if (kStream.GetFileVersion() >= 0x0A010068) {
        m_spObjectPalette = static_cast<NiDefaultAVObjectPalette*>(kStream.ResolveLinkID());
    }
}

// Adds each linked sequence that has no owner yet: it is owned by this manager, takes the target's name as its
// accumulation root when it has none, fills the first empty slot and is mapped by its name. Targets are stored later,
// in PostLinkObject. The owner test is skipped for a null this, as the game does.
void NiControllerManager::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);

    uint32_t uiNumSequences = kStream.GetNumberOfLinkIDs();
    m_kSequenceArray.SetSize(uiNumSequences);
    for (uint32_t i = 0; i < uiNumSequences; i++) {
        NiControllerSequence* pkSequence = static_cast<NiControllerSequence*>(kStream.GetObjectFromLinkID());
        if (this && pkSequence->m_pkOwner) {
            continue;
        }

        pkSequence->m_pkOwner = this;
        if (!pkSequence->m_kAccumRootName.Exists()) {
            // NiFixedString::operator= is written out here: acquire the new handle, release the old, store it.
            NiGlobalStringTable::GlobalStringHandle kHandle = m_pkTarget->m_kName.GetHandle();
            if (pkSequence->m_kAccumRootName.GetHandle() != kHandle) {
                NiGlobalStringTable::AcquireHandle(kHandle);
                NiGlobalStringTable::ReleaseHandle(pkSequence->m_kAccumRootName.GetHandle());
                pkSequence->m_kAccumRootName.SetHandle(kHandle);
            }
        }
        if (EmptyFS.GetHandle() != NiGlobalStringTable::NULL_STRING) {
            // Assigns the empty string's handle to the sequence name.
            NiGlobalStringTable::GlobalStringHandle kHandle = EmptyFS.GetHandle();
            if (pkSequence->m_kName.GetHandle() != kHandle) {
                NiGlobalStringTable::AcquireHandle(kHandle);
                NiGlobalStringTable::ReleaseHandle(pkSequence->m_kName.GetHandle());
                pkSequence->m_kName.SetHandle(kHandle);
            }
        }

        m_kSequenceArray.AddFirstEmpty(NiPointer<NiControllerSequence>(pkSequence));
        m_kStringMap.SetAt(pkSequence->m_kName.GetHandle(), pkSequence);
    }
}

// Registers the base, each sequence and the object palette. A manager without a palette gets a new default palette over
// its target (37 buckets) instead, unregistered.
bool NiControllerManager::RegisterStreamables(NiStream& kStream) {
    if (!NiTimeController::RegisterStreamables(kStream)) {
        return false;
    }

    for (uint32_t i = 0; i < m_kSequenceArray.m_usSize; i++) {
        NiControllerSequence* pkSequence = m_kSequenceArray.m_pBase[i].m_pObject;
        if (pkSequence) {
            pkSequence->RegisterStreamables(kStream);
        }
    }
    NiDefaultAVObjectPalette* pkPalette = m_spObjectPalette.m_pObject;
    if (pkPalette) {
        pkPalette->RegisterStreamables(kStream);
        return true;
    }
    NiDefaultAVObjectPalette* pkNewPalette = new NiDefaultAVObjectPalette(static_cast<NiAVObject*>(m_pkTarget), 37);
    m_spObjectPalette = pkNewPalette;
    return true;
}

// The count includes empty sequence slots but only the filled ones are linked, so a file saved with gaps reads back
// misaligned.
void NiControllerManager::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);

    uint8_t ucCumulative = m_bCumulative;
    NiStreamSaveBinary(kStream, ucCumulative);

    uint32_t uiNumSequences = m_kSequenceArray.GetSize();
    NiStreamSaveBinary(kStream, uiNumSequences);
    for (uint32_t i = 0; i < uiNumSequences; i++) {
        NiControllerSequence* pkSequence = m_kSequenceArray[i];
        if (pkSequence) {
            kStream.SaveLinkID(pkSequence);
        }
    }

    kStream.SaveLinkID(m_spObjectPalette);
}

// Every named sequence must have an equal one under the same name in apObject, and the sequence slots must match
// pairwise (both empty or equal).
bool NiControllerManager::IsEqual(NiObject* pkObject) {
    if (!NiTimeController::IsEqual(pkObject)) {
        return false;
    }

    NiControllerManager* pkOther = static_cast<NiControllerManager*>(pkObject);
    if (m_kSequenceArray.GetSize() != pkOther->m_kSequenceArray.GetSize()) {
        return false;
    }

    NiTMapItem<const char*, NiControllerSequence*>* pkPos = m_kStringMap.GetFirstPos();
    while (pkPos) {
        const char* pcName;
        NiControllerSequence* pkSequence;
        m_kStringMap.GetNext(pkPos, pcName, pkSequence);

        NiControllerSequence* pkOtherSequence;
        if (!pkOther->m_kStringMap.GetAt(pcName, pkOtherSequence) || !pkSequence->IsEqual(pkOtherSequence)) {
            return false;
        }
    }

    for (uint32_t i = 0; i < m_kSequenceArray.GetSize(); i++) {
        NiControllerSequence* pkSequence = m_kSequenceArray[i];
        NiControllerSequence* pkOtherSequence = pkOther->m_kSequenceArray[i];
        if (pkSequence && pkOtherSequence) {
            if (!pkSequence->IsEqual(pkOtherSequence)) {
                return false;
            }
        }
        else if (pkSequence != pkOtherSequence) {
            return false;
        }
    }
    return true;
}

// The sequences are processed and re-bound to this (original) target. The clone's blend sequences become the clones of
// this one's, or the same sequences when they were not cloned.
void NiControllerManager::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);

    for (uint32_t i = 0; i < m_kSequenceArray.GetSize(); i++) {
        NiControllerSequence* pkSequence = m_kSequenceArray[i];
        if (pkSequence) {
            pkSequence->ProcessClone(kCloning);
            pkSequence->StoreTargets(static_cast<NiAVObject*>(m_pkTarget));
        }
    }

    NiObject* pkCloneObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkCloneObject);
    NiControllerManager* pkClone = static_cast<NiControllerManager*>(pkCloneObject);

    for (uint32_t i = 0; i < m_kTempBlendSeqs.GetSize(); i++) {
        NiTPointerMap<NiObject*, NiObject*>* pkCloneMap = kCloning.m_pkCloneMap;
        NiObject* pkKey = m_kTempBlendSeqs.m_pBase[i].m_pObject;
        NiTMapItem<NiObject*, NiObject*>* pkItem = pkCloneMap->m_ppkHashTable[pkCloneMap->KeyToHashIndex(pkKey)];
        while (pkItem && !pkCloneMap->IsKeysEqual(pkKey, pkItem->m_key)) {
            pkItem = pkItem->m_pkNext;
        }
        if (pkItem) {
            NiControllerSequence* pkCloneSequence = static_cast<NiControllerSequence*>(pkItem->m_val);
            if (pkCloneSequence) {
                pkCloneSequence->IncRefCount();
            }
            pkClone->m_kTempBlendSeqs.Add(reinterpret_cast<const NiPointer<NiControllerSequence>&>(pkCloneSequence));
            if (pkCloneSequence) {
                pkCloneSequence->DecRefCount();
            }
        }
        else {
            pkClone->m_kTempBlendSeqs.Add(m_kTempBlendSeqs.m_pBase[i]);
        }
    }

    NiDefaultAVObjectPalette* pkPalette = m_spObjectPalette.m_pObject;
    if (pkPalette) {
        pkPalette->ProcessClone(kCloning);
    }
}

// The sequences bind to the target's scene graph once it is linked.
void NiControllerManager::PostLinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);

    NiAVObject* pkRoot = static_cast<NiAVObject*>(m_pkTarget);
    for (uint32_t i = 0; i < m_kSequenceArray.GetSize(); i++) {
        NiControllerSequence* pkSequence = m_kSequenceArray[i];
        if (pkSequence) {
            pkSequence->StoreTargets(pkRoot);
        }
    }
}

// Advances the active sequences; a sequence that has become inactive is dropped, the last one taking its slot. Tests
// the active bit directly rather than calling GetActive.
void NiControllerManager::Update(NiUpdateData& UpdateData) {
    if (!(m_usFlags & ACTIVE_MASK)) {
        return;
    }

    for (uint32_t i = 0; i < m_kActiveSequences.GetSize(); i++) {
        NiControllerSequence* pkSequence = m_kActiveSequences.GetAt(i);
        pkSequence->Update(UpdateData.fTime, true);
        if (pkSequence->m_eState == NiControllerSequence::kInactive) {
            m_kActiveSequences.RemoveAt(i);
            i--;
        }
    }
}

bool NiControllerManager::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

// This differs from the reference Gamebryo source: the array size is always pkSequence's, the first inactive temporary
// sequence is reused whatever its size (its targets cleared, its accumulation root renamed after the target, and
// PopulatePoseSequence told to remove its interpolators), and a new one is added the way AddSequence does it (owner,
// accumulation root, the empty name when it exists, the sequence array and the name map). PopulatePoseSequence reads
// only the low byte of the flag it is passed.
NiControllerSequence* NiControllerManager::CreateTempBlendSequence(
    NiControllerSequence* pkSequence,
    NiControllerSequence* pkSequenceToSynchronize
) {
    uint32_t uiArraySize = pkSequence->m_uiArraySize;
    NiControllerSequence* pkTempSequence = nullptr;
    for (uint32_t ui = 0; ui < m_kTempBlendSeqs.m_uiUsed; ui++) {
        NiControllerSequence* pkSeq = m_kTempBlendSeqs.m_pBase[ui].m_pObject;
        if (pkSeq->m_eState == NiControllerSequence::kInactive) {
            pkTempSequence = pkSeq;
            break;
        }
    }

    bool bRemInterps = false;
    if (!pkTempSequence) {
        pkTempSequence = new NiControllerSequence("__TempBlendSequence__", uiArraySize, 12);

        // The set receives a smart-pointer temporary.
        m_kTempBlendSeqs.Add(pkTempSequence);

        // EXE-INLINED 0xA2F0C0: AddSequence(pkTempSequence, EmptyFS, false)
        if (!pkTempSequence->m_pkOwner) {
            bool bSetAccumRootName = pkTempSequence->m_kAccumRootName.GetHandle() == nullptr;
            pkTempSequence->m_pkOwner = this;
            if (bSetAccumRootName) {
                const char* pcTargetName = m_pkTarget->m_kName.GetHandle();
                if (pkTempSequence->m_kAccumRootName.GetHandle() != pcTargetName) {
                    if (pcTargetName != NiGlobalStringTable::NULL_STRING) {
                        InterlockedIncrementUInt32(
                            reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcTargetName) - 8)
                        );
                    }
                    const char* pcOld = pkTempSequence->m_kAccumRootName.GetHandle();
                    if (pcOld != NiGlobalStringTable::NULL_STRING) {
                        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
                    }
                    pkTempSequence->m_kAccumRootName.SetHandle(pcTargetName);
                }
            }
            const char* pcEmpty = EmptyFS.GetHandle();
            if (pcEmpty != NiGlobalStringTable::NULL_STRING && pkTempSequence->m_kName.GetHandle() != pcEmpty) {
                if (pcEmpty != NiGlobalStringTable::NULL_STRING) {
                    InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcEmpty) - 8));
                }
                const char* pcOld = pkTempSequence->m_kName.GetHandle();
                if (pcOld != NiGlobalStringTable::NULL_STRING) {
                    InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
                }
                pkTempSequence->m_kName.SetHandle(pcEmpty);
            }
            m_kSequenceArray.AddFirstEmpty(pkTempSequence);
            m_kStringMap.SetAt(pkTempSequence->m_kName.GetHandle(), pkTempSequence);
        }
    }
    else {
        pkTempSequence->ClearTargets();
        if (!pkTempSequence->m_pkOwner) {
            pkTempSequence->m_pkOwner = this;
        }
        const char* pcTargetName = m_pkTarget->m_kName.GetHandle();
        if (pkTempSequence->m_kAccumRootName.GetHandle() != pcTargetName) {
            if (pcTargetName != NiGlobalStringTable::NULL_STRING) {
                InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcTargetName) - 8));
            }
            const char* pcOld = pkTempSequence->m_kAccumRootName.GetHandle();
            if (pcOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
            }
            pkTempSequence->m_kAccumRootName.SetHandle(pcTargetName);
        }
        bRemInterps = true;
    }

    pkSequence->PopulatePoseSequence(pkTempSequence, uiArraySize, pkSequenceToSynchronize, bRemInterps);
    return pkTempSequence;
}

// Copies the cumulative flag and every sequence, each cloned and added to apDest under the empty name, then clones the
// object palette.
void NiControllerManager::CopyMembers(NiControllerManager* pkDest, NiCloningProcess& kCloning) {
    NiTimeController::CopyMembers(pkDest, kCloning);
    pkDest->m_bCumulative = m_bCumulative;
    for (uint32_t ui = 0; ui < m_kSequenceArray.m_usSize; ++ui) {
        NiControllerSequence* pkSequence = m_kSequenceArray.m_pBase[ui];
        if (pkSequence) {
            NiControllerSequence* pkClone = static_cast<NiControllerSequence*>(pkSequence->CreateClone(kCloning));
            pkDest->AddSequence(static_cast<BSAnimGroupSequence*>(pkClone), EmptyFS, false);
        }
    }
    NiDefaultAVObjectPalette* pkPalette = m_spObjectPalette.m_pObject;
    if (pkPalette) {
        NiDefaultAVObjectPalette* pkNewPalette =
            static_cast<NiDefaultAVObjectPalette*>(pkPalette->CreateClone(kCloning));
        pkDest->m_spObjectPalette = pkNewPalette;
    }
}

NiControllerSequence* NiControllerManager::FindControllerSequence(NiInterpolator* apInterpolator, bool abActiveOnly) {
    NiControllerSequence* m_pObject;
    NiControllerManager* v3 = this;
    int32_t v4 = 0;
    if (!m_kSequenceArray.m_usSize) {
        return nullptr;
    }
    while (true) {
        m_pObject = v3->m_kSequenceArray.m_pBase[v4].m_pObject;
        if (m_pObject) {
            break;
        }
    LABEL_13:;
        if (++v4 >= static_cast<uint32_t>(v3->m_kSequenceArray.m_usSize)) {
            return nullptr;
        }
    }
    NiIncRefCount(m_pObject);
    if (abActiveOnly && m_pObject->m_eState == NiControllerSequence::kInactive) {
    LABEL_11:;
        NiDecRefCount(m_pObject);
        goto LABEL_13;
    }
    uint32_t m_uiArraySize = m_pObject->m_uiArraySize;
    uint32_t v7 = 0;
    if (!m_uiArraySize) {
    LABEL_10:;
        v3 = this;
        goto LABEL_11;
    }
    NiControllerSequence::InterpArrayItem* m_pkInterpArray = m_pObject->m_pkInterpArray;
    while (!m_pkInterpArray->m_spInterpolator.m_pObject ||
           m_pkInterpArray->m_spInterpolator.m_pObject != apInterpolator) {
        ++v7;
        ++m_pkInterpArray;
        if (v7 >= m_uiArraySize) {
            goto LABEL_10;
        }
    }
    NiDecRefCount(m_pObject);
    return m_pObject;
}

bool NiControllerManager::StopTransSourceSequence(float fEaseOutTime) {
    for (uint32_t i = 0; i < m_kActiveSequences.m_uiUsed; i++) {
        NiControllerSequence* pkSequence = m_kActiveSequences.m_pBase[i];
        if (pkSequence->m_eState == NiControllerSequence::kTransSource) {
            pkSequence->Deactivate(fEaseOutTime, false);
        }
    }
    return false;
}

bool NiControllerManager::Morph(
    NiControllerSequence* pkSourceSequence,
    NiControllerSequence* pkDestSequence,
    float fDuration,
    int32_t iPriority,
    float fSourceWeight,
    float fDestWeight
) {
    return (bool)((int8_t)
                      pkSourceSequence->StartMorph(pkDestSequence, fDuration, iPriority, fSourceWeight, fDestWeight));
}

bool NiControllerManager::CrossFade(
    NiControllerSequence* pkSourceSequence,
    NiControllerSequence* pkDestSequence,
    float fDuration,
    int32_t iPriority,
    bool bStartOver,
    float fWeight,
    NiControllerSequence* pkTimeSyncSeq
) {
    if (pkSourceSequence->m_eState == NiControllerSequence::kInactive ||
        pkDestSequence->m_eState != NiControllerSequence::kInactive) {
        return false;
    }
    pkSourceSequence->Deactivate(fDuration, false);
    return pkDestSequence->Activate(static_cast<char>(iPriority), bStartOver, fWeight, fDuration, pkTimeSyncSeq, false);
}

// The temporary sequence holding the current pose blends into pkSequence at full weight on both sides, with no
// time-sync sequence.
bool NiControllerManager::BlendFromPose(
    NiControllerSequence* pkSequence,
    float fDestFrame,
    float fDuration,
    int32_t iPriority,
    NiControllerSequence* pkSequenceToSynchronize
) {
    if (pkSequence->m_eState != NiControllerSequence::kInactive) {
        return false;
    }

    NiControllerSequence* pkTempSequence = CreateTempBlendSequence(pkSequence, pkSequenceToSynchronize);
    return pkTempSequence->StartBlend(pkSequence, fDuration, fDestFrame, iPriority, 1.0f, 1.0f, nullptr);
}

void NiControllerManager::RemoveAllSequences() {
    for (uint32_t ui = 0; ui < m_kSequenceArray.m_usSize; ++ui) {
        NiControllerSequence* pkSequence = m_kSequenceArray.m_pBase[ui].m_pObject;
        if (pkSequence) {
            pkSequence->ClearTargets();
            pkSequence->m_pkOwner = nullptr;
        }
    }
    m_kSequenceArray.RemoveAll();
    m_kStringMap.RemoveAll();
}

void NiControllerManager::RemoveSequence(NiControllerSequence* apSequence) {
    NiPointer<NiControllerSequence> spsequence;
    RemoveSequence(apSequence, spsequence);
}

// RemoveAt's returned pointer is released at once.
void NiControllerManager::RemoveSequence(
    NiControllerSequence* pkSequence,
    NiPointer<NiControllerSequence>& aspControllerSequence
) {
    uint32_t ui = 0;
    if (m_kSequenceArray.m_usSize) {
        NiControllerSequence* m_pObject;
        NiTMapItem<const char*, NiControllerSequence*>* v12;
        NiControllerSequence* v21;
        while (true) {
            m_pObject = m_kSequenceArray.m_pBase[ui].m_pObject;
            v21 = m_pObject;
            if (m_pObject) {
                NiIncRefCount(m_pObject);
            }
            if (m_pObject == pkSequence) {
                break;
            }
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            if (++ui >= m_kSequenceArray.m_usSize) {
                goto LABEL_9;
            }
        }
        m_kSequenceArray.RemoveAt(ui);
        uint32_t m_uiHashSize = m_kStringMap.m_uiHashSize;
        NiTStringPointerMap<NiControllerSequence*>* p_m_kStringMap = &m_kStringMap;
        NiTMapBase<NiTPointerAllocator<size_t>, const char*, NiControllerSequence*>* pkMap = &m_kStringMap;
        uint32_t v10 = 0;
        if (m_uiHashSize) {
            NiTMapItem<const char*, NiControllerSequence*>** m_ppkHashTable = m_kStringMap.m_ppkHashTable;
            while (!*m_ppkHashTable) {
                ++v10;
                ++m_ppkHashTable;
                if (v10 >= m_uiHashSize) {
                    goto LABEL_21;
                }
            }
            v12 = m_kStringMap.m_ppkHashTable[v10];
        }
        else {
        LABEL_21:;
            v12 = nullptr;
        }
        if (v12) {
            const char* pcName;
            while (true) {
                NiControllerSequence* pkVal = v12->m_val;
                pcName = v12->m_key;
                v12 = v12->m_pkNext;
                if (!v12) {
                    uint32_t v15 = pkMap->KeyToHashIndex(pcName);
                    uint32_t v16 = m_kStringMap.m_uiHashSize;
                    uint32_t v17 = v15 + 1;
                    if (v17 >= v16) {
                    LABEL_28:;
                        v12 = nullptr;
                    }
                    else {
                        NiTMapItem<const char*, NiControllerSequence*>** v18 = &m_kStringMap.m_ppkHashTable[v17];
                        while (!*v18) {
                            ++v17;
                            ++v18;
                            if (v17 >= v16) {
                                goto LABEL_28;
                            }
                        }
                        v12 = *v18;
                    }
                }
                if (pkVal == pkSequence) {
                    break;
                }
                if (!v12) {
                    goto LABEL_35;
                }
            }
            p_m_kStringMap->RemoveAt(pcName);
        LABEL_35:;
            m_pObject = v21;
        }
        for (uint32_t i = 0; i < m_kActiveSequences.m_uiUsed; ++i) {
            if (m_kActiveSequences.m_pBase[i] == pkSequence) {
                m_kActiveSequences.m_pBase[i] = m_kActiveSequences.m_pBase[--m_kActiveSequences.m_uiUsed];
            }
        }
        m_pObject->ClearTargets();
        m_pObject->m_pkOwner = nullptr;
        NiControllerSequence* v20 = aspControllerSequence.m_pObject;
        if (aspControllerSequence.m_pObject != m_pObject) {
            if (v20) {
                NiDecRefCount(v20);
            }
            aspControllerSequence.m_pObject = m_pObject;
            NiIncRefCount(m_pObject);
        }
        NiDecRefCount(m_pObject);
    }
    else {
    LABEL_9:;
        NiControllerSequence* v6 = aspControllerSequence.m_pObject;
        if (aspControllerSequence.m_pObject) {
            NiDecRefCount(v6);
            aspControllerSequence.m_pObject = nullptr;
        }
    }
}

// The null-this rejection is bypassed and the function continues when this is null, as the game does. The name is set
// unless its handle equals NiGlobalStringTable::NULL_STRING.
bool NiControllerManager::AddSequence(
    NiControllerSequence* pkSequence,
    const NiFixedString& kName,
    bool bStoreTargets
) {
    if (this && pkSequence->m_pkOwner) {
        return false;
    }
    bool v5 = pkSequence->m_kAccumRootName.GetHandle() == nullptr;
    pkSequence->m_pkOwner = this;
    if (v5) {
        pkSequence->SetAccumRootName(m_pkTarget->m_kName);
    }
    if (bStoreTargets && !pkSequence->StoreTargets(static_cast<NiAVObject*>(m_pkTarget))) {
        pkSequence->m_pkOwner = nullptr;
        return false;
    }
    if (kName.GetHandle() != NiGlobalStringTable::NULL_STRING) {
        pkSequence->SetName(kName);
    }
    m_kSequenceArray.AddFirstEmpty(pkSequence);
    m_kStringMap.SetAt(pkSequence->m_kName.GetHandle(), pkSequence);
    return true;
}

NiObject* NiControllerManager::CreateObject() {
    return new NiControllerManager;
}

NiAVObjectPalette::~NiAVObjectPalette() {}
