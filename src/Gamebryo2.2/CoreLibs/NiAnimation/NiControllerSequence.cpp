#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNoteListener.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStringPalette.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

#include <cfloat>
#include <cstdio>
#include <cstring>

static uint32_t strBSTreadTransfController_guard;
static uint32_t b01_guard;

NiRTTI NiControllerSequence::ms_RTTI("NiControllerSequence", &NiObject::ms_RTTI);

namespace {

    // Holds a reference on apObject in a smart-pointer field, released from the old one; the link pass stores objects
    // unchecked.
    template <class T>
    void NiControllerSequence_SetLinked(NiPointer<T>& arspField, NiObject* apObject) {
        arspField = static_cast<T*>(apObject);
    }

} // namespace

NiControllerSequence::InterpArrayItem::InterpArrayItem()
  : m_spInterpolator(nullptr), m_spInterpCtlr(nullptr), m_pkBlendInterp(nullptr), m_ucBlendIdx(0xFF),
    m_ucPriority(0xFF) {}

void NiControllerSequence::InterpArrayItem::ClearValues() {
    NiInterpolator* m_pObject;
    m_pObject = m_spInterpolator.m_pObject;
    if (m_spInterpolator.m_pObject) {
        m_pObject->DecRefCount();
        m_spInterpolator.m_pObject = nullptr;
    }
    m_spInterpCtlr = nullptr;
    m_pkBlendInterp = nullptr;
    m_ucBlendIdx = (uint8_t)(-1);
    m_ucPriority = (uint8_t)(-1);
}

// Vector deleting destructor of InterpArrayItem: with flag 2 the items of the new[] block this starts are released last
// to first (ReleaseRefs), then with flag 1 the block is freed; otherwise only this one item is released. Returns the
// block (the array cookie's address) or this.
void* NiControllerSequence::InterpArrayItem::Free(int32_t aiFlags) {
    if ((aiFlags & 2) != 0) {
        // The element count new[] stores just before the array (its cookie).
        int32_t iCount = static_cast<int32_t>(reinterpret_cast<size_t*>(this)[-1]);
        NiControllerSequence::InterpArrayItem* pkItem = this + iCount;
        for (int32_t i = iCount - 1; i >= 0; --i) {
            --pkItem;
            // EXE-INLINED 0xA30700
            NiInterpController* pkCtlr = pkItem->m_spInterpCtlr.m_pObject;
            if (pkCtlr) {
                // EXE-INLINED none: NiDecRefCount
                NiDecRefCount(pkCtlr);
            }
            NiInterpolator* pkInterp = pkItem->m_spInterpolator.m_pObject;
            if (pkInterp) {
                // EXE-INLINED none: NiDecRefCount
                NiDecRefCount(pkInterp);
            }
        }
        if ((aiFlags & 1) != 0) {
            MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(this) - 1);
        }
        return reinterpret_cast<size_t*>(this) - 1;
    }

    ReleaseRefs();
    if ((aiFlags & 1) != 0) {
        MemoryManager_Singleton.Deallocate(this);
    }
    return this;
}

bool NiControllerSequence::InterpArrayItem::IsEqual(const InterpArrayItem& kDest) const {
    NiInterpolator* pkInterp = m_spInterpolator;
    NiInterpolator* pkOtherInterp = kDest.m_spInterpolator;
    if ((pkInterp && !pkOtherInterp) || (!pkInterp && pkOtherInterp)) {
        return false;
    }
    if (pkInterp && pkOtherInterp && !pkInterp->IsEqual(pkOtherInterp)) {
        return false;
    }

    NiInterpController* pkCtlr = m_spInterpCtlr;
    NiInterpController* pkOtherCtlr = kDest.m_spInterpCtlr;
    if ((pkCtlr && !pkOtherCtlr) || (!pkCtlr && pkOtherCtlr)) {
        return false;
    }
    if (pkCtlr && pkOtherCtlr && !pkCtlr->IsEqual(pkOtherCtlr)) {
        return false;
    }

    return m_ucPriority == kDest.m_ucPriority;
}

void NiControllerSequence::InterpArrayItem::LoadBinary(NiStream& kStream) {
    NiStream* pKStream = &kStream;
    uint32_t v14;
    uint32_t v15;
    NiStream* v2 = pKStream;
    NiInterpolator* v4 = static_cast<NiInterpolator*>(pKStream->ResolveLinkID());
    m_spInterpolator = v4;
    NiObject* v7 = v2->ResolveLinkID();
    m_spInterpCtlr = static_cast<NiInterpController*>(v7);
    if (v2->GetFileVersion() < 0xA01006F) {
        v2->ResolveLinkID();
        NiBinaryStream* m_pkIstr = v2->Istr();
        uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
        v14 = 2;
        m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pKStream, 2, &v14, 1);
    }
    if (v2->m_BSStreamHeader.uiVersion) {
        NiBinaryStream* v12 = v2->Istr();
        uint32_t (*v13)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v12->m_pfnRead;
        v15 = 1;
        v12->m_uiAbsoluteCurrentPos += v13(v12, &m_ucPriority, 1, &v15, 1);
    }
}

NiControllerSequence::InterpArrayItem&
NiControllerSequence::InterpArrayItem::operator=(const NiControllerSequence::InterpArrayItem& kOther) {
    m_spInterpolator = kOther.m_spInterpolator;
    m_spInterpCtlr = kOther.m_spInterpCtlr;
    m_pkBlendInterp = kOther.m_pkBlendInterp;
    m_ucBlendIdx = kOther.m_ucBlendIdx;
    m_ucPriority = kOther.m_ucPriority;
    return *this;
}

void NiControllerSequence::InterpArrayItem::ReleaseRefs() {
    NiInterpController* m_pObject = m_spInterpCtlr.m_pObject;
    if (m_pObject) {
        // EXE-INLINED none: NiDecRefCount
        NiDecRefCount(m_pObject);
    }
    NiInterpolator* v3 = m_spInterpolator.m_pObject;
    if (m_spInterpolator.m_pObject) {
        NiDecRefCount(v3);
    }
}

NiControllerSequence::IDTag::IDTag(
    const NiFixedString& akAVObjectName,
    const NiFixedString& akPropertyType,
    const NiFixedString& akCtlrType,
    const NiFixedString& akCtlrID,
    const NiFixedString& akInterpolatorID
)
  : m_kAVObjectName(akAVObjectName), m_kPropertyType(akPropertyType), m_kCtlrType(akCtlrType), m_kCtlrID(akCtlrID),
    m_kInterpolatorID(akInterpolatorID) {}

NiControllerSequence::IDTag::~IDTag() {}

// Files from 10.1.0.113 on name the objects by fixed strings (20.1.0.1 on) or by string palette offsets, which are
// kept in the handle fields until the palette is resolved; older files hold C strings.
void NiControllerSequence::IDTag::LoadBinary(NiStream& kStream, void*& pvDeprecatedExtraData) {
    uint32_t uiNifFileVersion = kStream.GetFileVersion();
    if (uiNifFileVersion >= NiStream::GetVersion(10, 1, 0, 113)) {
        if (uiNifFileVersion >= NiStream::GetVersion(20, 1, 0, 1)) {
            kStream.LoadFixedString(m_kAVObjectName);
            kStream.LoadFixedString(m_kPropertyType);
            kStream.LoadFixedString(m_kCtlrType);
            kStream.LoadFixedString(m_kCtlrID);
            kStream.LoadFixedString(m_kInterpolatorID);
        }
        else {
            pvDeprecatedExtraData = kStream.ResolveLinkID();
            NiBinaryStream* m_pkIstr = kStream.Istr();
            // The offsets are 4 bytes, read into zeroed pointer-sized locals.
            char* uiAVObjectName = nullptr;
            char* uiPropertyType = nullptr;
            char* uiCtlrType = nullptr;
            char* uiCtlrID = nullptr;
            char* uiInterpolatorID = nullptr;
            uint32_t v25;
            NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = m_pkIstr->m_pfnRead;
            v25 = 4;
            m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &uiAVObjectName, 4, &v25, 1);
            NiBinaryStream* v9 = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN v10 = v9->m_pfnRead;
            v25 = 4;
            v9->m_uiAbsoluteCurrentPos += v10(v9, &uiPropertyType, 4, &v25, 1);
            NiBinaryStream* v11 = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN v12 = v11->m_pfnRead;
            v25 = 4;
            v11->m_uiAbsoluteCurrentPos += v12(v11, &uiCtlrType, 4, &v25, 1);
            NiBinaryStream* v13 = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN v14 = v13->m_pfnRead;
            v25 = 4;
            v13->m_uiAbsoluteCurrentPos += v14(v13, &uiCtlrID, 4, &v25, 1);
            NiBinaryStream* v15 = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN v16 = v15->m_pfnRead;
            v25 = 4;
            v15->m_uiAbsoluteCurrentPos += v16(v15, &uiInterpolatorID, 4, &v25, 1);
            char* uiPropertyType_2 = uiPropertyType;
            char* uiCtlrType_2 = uiCtlrType;
            m_kAVObjectName.m_kHandle = uiAVObjectName;
            char* uiCtlrID_2 = uiCtlrID;
            m_kPropertyType.m_kHandle = uiPropertyType_2;
            m_kCtlrType.m_kHandle = uiCtlrType_2;
            char* uiInterpolatorID_2 = uiInterpolatorID;
            m_kCtlrID.m_kHandle = uiCtlrID_2;
            m_kInterpolatorID.m_kHandle = uiInterpolatorID_2;
        }
    }
    else {
        kStream.LoadCStringAsFixedString(m_kAVObjectName);
        kStream.LoadCStringAsFixedString(m_kPropertyType);
        kStream.LoadCStringAsFixedString(m_kCtlrType);
        kStream.LoadCStringAsFixedString(m_kCtlrID);
        kStream.LoadCStringAsFixedString(m_kInterpolatorID);
    }
}

// Sets the five names to the empty handle.
void NiControllerSequence::IDTag::ClearValues() {
    const char* pcAVObjectName = m_kAVObjectName.GetHandle();
    if (pcAVObjectName) {
        m_kAVObjectName.SetHandle(NiGlobalStringTable::AddString(nullptr));
        if (pcAVObjectName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcAVObjectName) - 8));
        }
    }
    const char* pcPropertyType = m_kPropertyType.GetHandle();
    if (pcPropertyType) {
        m_kPropertyType.SetHandle(NiGlobalStringTable::AddString(nullptr));
        if (pcPropertyType != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcPropertyType) - 8));
        }
    }
    const char* pcCtlrType = m_kCtlrType.GetHandle();
    if (pcCtlrType) {
        m_kCtlrType.SetHandle(NiGlobalStringTable::AddString(nullptr));
        if (pcCtlrType != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcCtlrType) - 8));
        }
    }
    const char* pcCtlrID = m_kCtlrID.GetHandle();
    if (pcCtlrID) {
        m_kCtlrID.SetHandle(NiGlobalStringTable::AddString(nullptr));
        if (pcCtlrID != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcCtlrID) - 8));
        }
    }
    const char* pcInterpolatorID = m_kInterpolatorID.GetHandle();
    if (pcInterpolatorID) {
        m_kInterpolatorID.SetHandle(NiGlobalStringTable::AddString(nullptr));
        if (pcInterpolatorID != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcInterpolatorID) - 8));
        }
    }
}

NiControllerSequence::IDTag* NiControllerSequence::IDTag::Operator_Set(const NiControllerSequence::IDTag* pkOther) {
    const char* m_kHandle = pkOther->m_kAVObjectName.GetHandle();
    if (m_kAVObjectName.GetHandle() != pkOther->m_kAVObjectName.GetHandle()) {
        NiGlobalStringTable::AcquireHandle(m_kHandle);
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(m_kAVObjectName.GetHandle());
        m_kAVObjectName.m_kHandle = m_kHandle;
    }
    const char* v4 = pkOther->m_kPropertyType.GetHandle();
    if (m_kPropertyType.GetHandle() != v4) {
        NiGlobalStringTable::AcquireHandle(v4);
        const char* v5 = m_kPropertyType.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v5);
        m_kPropertyType.m_kHandle = v4;
    }
    const char* v6 = pkOther->m_kCtlrType.GetHandle();
    if (m_kCtlrType.GetHandle() != v6) {
        NiGlobalStringTable::AcquireHandle(v6);
        const char* v7 = m_kCtlrType.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v7);
        m_kCtlrType.m_kHandle = v6;
    }
    const char* v8 = pkOther->m_kCtlrID.GetHandle();
    if (m_kCtlrID.GetHandle() != v8) {
        NiGlobalStringTable::AcquireHandle(v8);
        const char* v9 = m_kCtlrID.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v9);
        m_kCtlrID.m_kHandle = v8;
    }
    const char* v10 = pkOther->m_kInterpolatorID.GetHandle();
    if (m_kInterpolatorID.GetHandle() != v10) {
        NiGlobalStringTable::AcquireHandle(v10);
        const char* v11 = m_kInterpolatorID.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v11);
        m_kInterpolatorID.m_kHandle = v10;
    }
    return this;
}

// Vector deleting destructor of IDTag: with flag 2 the tags of the new[] block this starts are released last to first,
// then with flag 1 the block is freed; otherwise only this one tag is released. Returns the block or this.
void* NiControllerSequence::IDTag::Free(char a1) {
    if ((a1 & 2) != 0) {
        // The element count new[] stores just before the array (its cookie).
        int32_t iCount = static_cast<int32_t>(reinterpret_cast<size_t*>(this)[-1]);
        NiControllerSequence::IDTag* pkTag = this + iCount;
        for (int32_t i = iCount - 1; i >= 0; --i) {
            --pkTag;
            // EXE-INLINED 0xA30750
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(pkTag->m_kInterpolatorID.GetHandle());
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(pkTag->m_kCtlrID.GetHandle());
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(pkTag->m_kCtlrType.GetHandle());
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(pkTag->m_kPropertyType.GetHandle());
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(pkTag->m_kAVObjectName.GetHandle());
        }
        if ((a1 & 1) != 0) {
            MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(this) - 1);
        }
        return reinterpret_cast<size_t*>(this) - 1;
    }

    this->~IDTag();
    if ((a1 & 1) != 0) {
        MemoryManager_Singleton.Deallocate(this);
    }
    return this;
}

// Files before 20.1.0.1 hold each name as an offset into the deprecated string palette, in the name's handle
// (INVALID_INDEX for none). The names start empty and each valid offset takes its palette string.
void NiControllerSequence::IDTag::HandleDeprecatedExtraData(NiStringPalette* pkPalette, NiStream& kStream) {
    // The NiFixedString handle holds the palette offset here, so it is read as an untyped uint32_t.
    uint32_t uiAVObjectName = *reinterpret_cast<const uint32_t*>(&m_kAVObjectName);
    uint32_t uiPropertyType = *reinterpret_cast<const uint32_t*>(&m_kPropertyType);
    uint32_t uiCtlrType = *reinterpret_cast<const uint32_t*>(&m_kCtlrType);
    uint32_t uiCtlrID = *reinterpret_cast<const uint32_t*>(&m_kCtlrID);
    uint32_t uiInterpolatorID = *reinterpret_cast<const uint32_t*>(&m_kInterpolatorID);

    NiFixedString kString;
    m_kAVObjectName.SetHandle(kString.GetHandle());
    m_kPropertyType.SetHandle(kString.GetHandle());
    m_kCtlrType.SetHandle(kString.GetHandle());
    m_kCtlrID.SetHandle(kString.GetHandle());
    m_kInterpolatorID.SetHandle(kString.GetHandle());

    if (uiAVObjectName != NiControllerSequence::INVALID_INDEX) {
        const char* pcName = &pkPalette->m_pcBuffer[uiAVObjectName];
        const char* pcOld = m_kAVObjectName.GetHandle();
        if (pcOld != pcName) {
            m_kAVObjectName.SetHandle(NiGlobalStringTable::AddString(pcName));
            NiGlobalStringTable::ReleaseHandle(pcOld);
        }
    }
    if (uiPropertyType != NiControllerSequence::INVALID_INDEX) {
        const char* pcName = &pkPalette->m_pcBuffer[uiPropertyType];
        const char* pcOld = m_kPropertyType.GetHandle();
        if (pcOld != pcName) {
            m_kPropertyType.SetHandle(NiGlobalStringTable::AddString(pcName));
            NiGlobalStringTable::ReleaseHandle(pcOld);
        }
    }
    if (uiCtlrType != NiControllerSequence::INVALID_INDEX) {
        const char* pcName = &pkPalette->m_pcBuffer[uiCtlrType];
        const char* pcOld = m_kCtlrType.GetHandle();
        if (pcOld != pcName) {
            m_kCtlrType.SetHandle(NiGlobalStringTable::AddString(pcName));
            NiGlobalStringTable::ReleaseHandle(pcOld);
        }
    }
    if (uiCtlrID != NiControllerSequence::INVALID_INDEX) {
        const char* pcName = &pkPalette->m_pcBuffer[uiCtlrID];
        const char* pcOld = m_kCtlrID.GetHandle();
        if (pcOld != pcName) {
            m_kCtlrID.SetHandle(NiGlobalStringTable::AddString(pcName));
            NiGlobalStringTable::ReleaseHandle(pcOld);
        }
    }
    if (uiInterpolatorID != NiControllerSequence::INVALID_INDEX) {
        const char* pcName = &pkPalette->m_pcBuffer[uiInterpolatorID];
        const char* pcOld = m_kInterpolatorID.GetHandle();
        if (pcOld != pcName) {
            m_kInterpolatorID.SetHandle(NiGlobalStringTable::AddString(pcName));
            NiGlobalStringTable::ReleaseHandle(pcOld);
        }
    }
}

NiControllerSequence::NiControllerSequence()
  : m_uiArraySize(0), m_uiArrayGrowBy(0), m_pkInterpArray(nullptr), m_pkIDTagArray(nullptr), m_fSeqWeight(1.0f),
    m_spTextKeys(nullptr), m_eCycleType(CYCLE_LOOP), m_fFrequency(1.0f), m_fBeginKeyTime(3.4028235e+38f),
    m_fEndKeyTime(-3.4028235e+38f), m_fLastTime(-3.4028235e+38f), m_fWeightedLastTime(-3.4028235e+38f),
    m_fLastScaledTime(-3.4028235e+38f), m_pkOwner(nullptr), m_eState(kInactive), m_fOffset(-3.4028235e+38f),
    m_fStartTime(-3.4028235e+38f), m_fEndTime(-3.4028235e+38f), m_fDestFrame(-3.4028235e+38f),
    m_pkPartnerSequence(nullptr), m_pkAccumRoot(nullptr), m_spDeprecatedStringPalette(nullptr), ssCurAnimNIdx(-1),
    spAnimNotesA(nullptr), usNumNotes(0), bRemovableObjects(false) {}

NiControllerSequence::NiControllerSequence(const NiFixedString& kName, uint32_t uiArraySize, uint32_t uiArrayGrowBy)
  : m_kName(kName), m_uiArraySize(uiArraySize), m_uiArrayGrowBy(uiArrayGrowBy), m_pkInterpArray(nullptr),
    m_pkIDTagArray(nullptr), m_fSeqWeight(1.0f), m_spTextKeys(nullptr), m_eCycleType(CYCLE_LOOP), m_fFrequency(1.0f),
    m_fBeginKeyTime(3.4028235e+38f), m_fEndKeyTime(-3.4028235e+38f), m_fLastTime(-3.4028235e+38f),
    m_fWeightedLastTime(-3.4028235e+38f), m_fLastScaledTime(-3.4028235e+38f), m_pkOwner(nullptr), m_eState(kInactive),
    m_fOffset(-3.4028235e+38f), m_fStartTime(-3.4028235e+38f), m_fEndTime(-3.4028235e+38f),
    m_fDestFrame(-3.4028235e+38f), m_pkPartnerSequence(nullptr), m_pkAccumRoot(nullptr),
    m_spDeprecatedStringPalette(nullptr), ssCurAnimNIdx(-1), spAnimNotesA(nullptr), usNumNotes(0),
    bRemovableObjects(false) {
    if (m_uiArraySize) {
        m_pkInterpArray = new InterpArrayItem[m_uiArraySize];
        m_pkIDTagArray = new IDTag[m_uiArraySize];
    }
}

NiControllerSequence::~NiControllerSequence() {
    if (m_eState != kInactive) {
        NiControllerSequence::Deactivate(0.0f, false);
    }
    delete[] m_pkInterpArray;
    delete[] m_pkIDTagArray;
    if (usNumNotes) {
        delete[] spAnimNotesA;
    }
}

const NiRTTI* NiControllerSequence::GetRTTI() const {
    return &ms_RTTI;
}

// The members are copied even when the allocation failed.
NiObject* NiControllerSequence::CreateClone(NiCloningProcess& kCloning) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiControllerSequence));
    NiControllerSequence* pClone = nullptr;
    if (pvMemory) {
        pClone = ::new (pvMemory) NiControllerSequence;
    }
    CopyMembers(pClone, kCloning);
    return pClone;
}

// Files before 10.1.0.104 hold only the names, the object names of the ID tags and a controller link per entry
// (LinkObject reads those). The arrays are `new T[n]`: the count is stored in front of the elements and the engine's
// element constructors run on each.
void NiControllerSequence::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    const bool bOldFormat = kStream.m_uiNifFileVersion < NiStream::GetVersion(10, 1, 0, 104);
    if (bOldFormat) {
        kStream.LoadCStringAsFixedString(m_kName);
        kStream.LoadCStringAsFixedString(m_kAccumRootName);
        kStream.ReadLinkID();
        NiStreamLoadBinary(kStream, m_uiArraySize);
        m_uiArrayGrowBy = 12;
    }
    else {
        if (kStream.m_uiNifFileVersion >= NiStream::GetVersion(20, 1, 0, 1)) {
            kStream.LoadFixedString(m_kName);
        }
        else {
            kStream.LoadCStringAsFixedString(m_kName);
        }
        NiStreamLoadBinary(kStream, m_uiArraySize);
        NiStreamLoadBinary(kStream, m_uiArrayGrowBy);
    }

    uint32_t uiCount = m_uiArraySize;
    uint64_t ullSize = uint64_t{sizeof(InterpArrayItem)} * uiCount;
    size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
    InterpArrayItem* pkInterpArray = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiCount;
        pkInterpArray = reinterpret_cast<InterpArrayItem*>(static_cast<size_t*>(pvMemory) + 1);
        rt::eh_vector_constructor_iterator(
            pkInterpArray, sizeof(InterpArrayItem), uiCount,
            &xfnv::constructElement<NiControllerSequence::InterpArrayItem>
        );
    }
    m_pkInterpArray = pkInterpArray;

    uiCount = m_uiArraySize;
    ullSize = uint64_t{sizeof(IDTag)} * uiCount;
    stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    pvMemory = MemoryManager_Singleton.Allocate(stSize);
    IDTag* pkIDTagArray = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiCount;
        pkIDTagArray = reinterpret_cast<IDTag*>(static_cast<size_t*>(pvMemory) + 1);
        rt::eh_vector_constructor_iterator(
            pkIDTagArray, sizeof(IDTag), uiCount, &xfnv::constructElement<NiControllerSequence::IDTag>
        );
    }
    m_pkIDTagArray = pkIDTagArray;

    if (bOldFormat) {
        for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
            NiFixedString kStr;
            kStream.LoadCStringAsFixedString(kStr);
            m_pkIDTagArray[ui].m_kAVObjectName = kStr;
            kStream.ReadLinkID();
        }
        return;
    }

    void* pvOldStringPaletteAddress = nullptr;
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        m_pkInterpArray[ui].LoadBinary(kStream);
        m_pkIDTagArray[ui].LoadBinary(kStream, pvOldStringPaletteAddress);
    }

    NiStreamLoadBinary(kStream, m_fSeqWeight);
    // The text keys' link; LinkObject resolves it.
    kStream.ReadLinkID();
    int32_t iCycleType;
    NiStreamLoadBinary(kStream, iCycleType);
    m_eCycleType = static_cast<CycleType>(iCycleType);
    NiStreamLoadBinary(kStream, m_fFrequency);
    if (kStream.m_uiNifFileVersion < NiStream::GetVersion(10, 3, 0, 1)) {
        float fPhase;
        NiStreamLoadBinary(kStream, fPhase);
    }
    NiStreamLoadBinary(kStream, m_fBeginKeyTime);
    NiStreamLoadBinary(kStream, m_fEndKeyTime);
    if (kStream.m_uiNifFileVersion < NiStream::GetVersion(10, 1, 0, 107)) {
        uint8_t bPlayBackwards;
        NiStreamLoadBinary(kStream, bPlayBackwards);
    }
    // The owner's link; LinkObject resolves it.
    kStream.ReadLinkID();
    if (kStream.m_uiNifFileVersion >= NiStream::GetVersion(20, 1, 0, 1)) {
        kStream.LoadFixedString(m_kAccumRootName);
    }
    else {
        kStream.LoadCStringAsFixedString(m_kAccumRootName);
    }

    uint32_t uiVersion = kStream.m_uiNifFileVersion;
    if (uiVersion < NiStream::GetVersion(10, 1, 0, 113) || uiVersion >= NiStream::GetVersion(20, 1, 0, 1)) {
        m_spDeprecatedStringPalette = nullptr;
    }
    else {
        NiObject* pkPalette = kStream.ResolveLinkID();
        m_spDeprecatedStringPalette = pkPalette;
    }

    uiVersion = kStream.m_BSStreamHeader.uiVersion;
    if (uiVersion > 0x15 && uiVersion < 0x1D) {
        kStream.ReadLinkID();
    }
    if (kStream.m_BSStreamHeader.uiVersion > 0x1C) {
        NiStreamLoadBinary(kStream, usNumNotes);
        if (usNumNotes) {
            uiCount = usNumNotes;
            ullSize = uint64_t{sizeof(NiPointer<BSAnimNote>)} * uiCount;
            stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
            stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
            pvMemory = MemoryManager_Singleton.Allocate(stSize);
            NiPointer<BSAnimNote>* pspAnimNotes = nullptr;
            if (pvMemory) {
                *static_cast<size_t*>(pvMemory) = uiCount;
                pspAnimNotes = reinterpret_cast<NiPointer<BSAnimNote>*>(static_cast<size_t*>(pvMemory) + 1);
                rt::eh_vector_constructor_iterator(
                    pspAnimNotes, sizeof(NiPointer<BSAnimNote>), uiCount, &xfnv::constructElement<NiPointer<BSAnimNote>>
                );
            }
            spAnimNotesA = pspAnimNotes;
        }
        // The notes' links; LinkObject resolves them.
        for (uint16_t us = 0; us < usNumNotes; ++us) {
            kStream.ReadLinkID();
        }
    }
}

// Pre-10.1.0.104 files link controllers instead of interpolators. Bethesda streams 22 to 28 carry one unused link
// that drops the notes; later ones link the notes.
void NiControllerSequence::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    if (kStream.m_uiNifFileVersion < NiStream::GetVersion(10, 1, 0, 104)) {
        NiControllerSequence_SetLinked(m_spTextKeys, kStream.GetObjectFromLinkID());
        for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
            NiControllerSequence_SetLinked(m_pkInterpArray[ui].m_spInterpCtlr, kStream.GetObjectFromLinkID());
        }
        return;
    }

    NiControllerSequence_SetLinked(m_spTextKeys, kStream.GetObjectFromLinkID());
    m_pkOwner = static_cast<NiControllerManager*>(kStream.GetObjectFromLinkID());
    uint32_t uiVersion = kStream.m_BSStreamHeader.uiVersion;
    if (uiVersion > 0x15 && uiVersion < 0x1D) {
        kStream.GetObjectFromLinkID();
        spAnimNotesA = nullptr;
        usNumNotes = 0;
    }

    if (kStream.m_BSStreamHeader.uiVersion > 0x1C) {
        for (uint32_t ui = 0; ui < usNumNotes; ++ui) {
            NiControllerSequence_SetLinked(spAnimNotesA[ui], kStream.GetObjectFromLinkID());
        }
    }
}

bool NiControllerSequence::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem& rkItem = m_pkInterpArray[ui];
        if (rkItem.m_spInterpolator) {
            rkItem.m_spInterpolator->RegisterStreamables(kStream);
        }
        if (rkItem.m_spInterpCtlr) {
            rkItem.m_spInterpCtlr->RegisterStreamables(kStream);
        }

        IDTag& rkTag = m_pkIDTagArray[ui];
        kStream.RegisterFixedString(rkTag.m_kAVObjectName);
        kStream.RegisterFixedString(rkTag.m_kPropertyType);
        kStream.RegisterFixedString(rkTag.m_kCtlrType);
        kStream.RegisterFixedString(rkTag.m_kCtlrID);
        kStream.RegisterFixedString(rkTag.m_kInterpolatorID);
    }

    if (m_spTextKeys) {
        m_spTextKeys->RegisterStreamables(kStream);
    }
    kStream.RegisterFixedString(m_kName);
    kStream.RegisterFixedString(m_kAccumRootName);
    for (uint32_t ui = 0; ui < usNumNotes; ++ui) {
        spAnimNotesA[ui]->RegisterStreamables(kStream);
    }

    return true;
}

// Always writes the current format, the notes included.
void NiControllerSequence::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    kStream.SaveFixedString(m_kName);
    NiStreamSaveBinary(kStream, m_uiArraySize);
    NiStreamSaveBinary(kStream, m_uiArrayGrowBy);
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem& rkItem = m_pkInterpArray[ui];
        kStream.SaveLinkID(rkItem.m_spInterpolator);
        kStream.SaveLinkID(rkItem.m_spInterpCtlr);
        NiStreamSaveBinary(kStream, rkItem.m_ucPriority);

        IDTag& rkTag = m_pkIDTagArray[ui];
        kStream.SaveFixedString(rkTag.m_kAVObjectName);
        kStream.SaveFixedString(rkTag.m_kPropertyType);
        kStream.SaveFixedString(rkTag.m_kCtlrType);
        kStream.SaveFixedString(rkTag.m_kCtlrID);
        kStream.SaveFixedString(rkTag.m_kInterpolatorID);
    }

    NiStreamSaveBinary(kStream, m_fSeqWeight);
    kStream.SaveLinkID(m_spTextKeys);
    NiStreamSaveBinary(kStream, m_eCycleType);
    NiStreamSaveBinary(kStream, m_fFrequency);
    NiStreamSaveBinary(kStream, m_fBeginKeyTime);
    NiStreamSaveBinary(kStream, m_fEndKeyTime);
    kStream.SaveLinkID(m_pkOwner);
    kStream.SaveFixedString(m_kAccumRootName);
    NiStreamSaveBinary(kStream, usNumNotes);
    for (uint32_t ui = 0; ui < usNumNotes; ++ui) {
        kStream.SaveLinkID(spAnimNotesA[ui]);
    }
}

// Names compare without case; the ID tags and the accumulation root name by handle. The owners are compared too.
bool NiControllerSequence::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiControllerSequence* pkOther = static_cast<NiControllerSequence*>(pkObject);
    const char* pcName = m_kName;
    if (pcName != pkOther->m_kName.GetHandle() && NiStricmp(pcName, pkOther->m_kName)) {
        return false;
    }

    if (m_uiArraySize != pkOther->m_uiArraySize || m_uiArrayGrowBy != pkOther->m_uiArrayGrowBy) {
        return false;
    }

    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        if (!m_pkInterpArray[ui].IsEqual(pkOther->m_pkInterpArray[ui])) {
            return false;
        }

        IDTag& rkTag = m_pkIDTagArray[ui];
        IDTag& rkOtherTag = pkOther->m_pkIDTagArray[ui];
        if (!(rkTag.m_kAVObjectName == rkOtherTag.m_kAVObjectName) ||
            !(rkTag.m_kPropertyType == rkOtherTag.m_kPropertyType) || !(rkTag.m_kCtlrType == rkOtherTag.m_kCtlrType) ||
            !(rkTag.m_kCtlrID == rkOtherTag.m_kCtlrID) || !(rkTag.m_kInterpolatorID == rkOtherTag.m_kInterpolatorID)) {
            return false;
        }
    }

    if (pkOther->m_fSeqWeight != m_fSeqWeight) {
        return false;
    }

    NiTextKeyExtraData* pkTextKeys = m_spTextKeys;
    NiTextKeyExtraData* pkOtherTextKeys = pkOther->m_spTextKeys;
    if (pkTextKeys) {
        if (!pkOtherTextKeys || !pkTextKeys->IsEqual(pkOtherTextKeys)) {
            return false;
        }
    }
    else if (pkOtherTextKeys) {
        return false;
    }

    if (m_eCycleType != pkOther->m_eCycleType || pkOther->m_fFrequency != m_fFrequency ||
        pkOther->m_fBeginKeyTime != m_fBeginKeyTime || pkOther->m_fEndKeyTime != m_fEndKeyTime) {
        return false;
    }

    if (m_pkOwner) {
        if (!pkOther->m_pkOwner || !m_pkOwner->IsEqual(pkOther->m_pkOwner)) {
            return false;
        }
    }
    else if (pkOther->m_pkOwner) {
        return false;
    }

    return m_kAccumRootName == pkOther->m_kAccumRootName;
}

void NiControllerSequence::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiObject::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    apStrings->Add(GetViewerString("State", m_eState));
    apStrings->Add(NiGetViewerString("ArraySize", m_uiArraySize));
    apStrings->Add(NiGetViewerString("Weight", m_fSeqWeight));
    apStrings->Add(
        NiTimeController::GetViewerString("CycleType", static_cast<NiTimeController::CycleType>(m_eCycleType))
    );
    apStrings->Add(NiGetViewerString("Freq", m_fFrequency));
    apStrings->Add(NiGetViewerString("Begin", m_fBeginKeyTime));
    apStrings->Add(NiGetViewerString("End", m_fEndKeyTime));
    apStrings->Add(NiGetViewerString("Last", m_fLastTime));
    apStrings->Add(NiGetViewerString("Offset", m_fOffset));
    apStrings->Add(NiGetViewerString("Start", m_fStartTime));
    apStrings->Add(NiGetViewerString("End", m_fEndTime));
    apStrings->Add(NiGetViewerString("WeightLast", m_fWeightedLastTime));
    apStrings->Add(NiGetViewerString("LastScaled", m_fLastScaledTime));
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        IDTag& rkTag = m_pkIDTagArray[ui];
        char acPrefix[128];
        char acValue[256];
        NiSprintf(acPrefix, sizeof(acPrefix), "Target %d, ", ui);
        NiSprintf(
            acValue, sizeof(acValue), "%s, %s, %d", rkTag.m_kAVObjectName.GetHandle(), rkTag.m_kCtlrType.GetHandle(),
            m_pkInterpArray[ui].m_ucPriority
        );
        apStrings->Add(NiGetViewerString(acPrefix, static_cast<const char*>(acValue)));
    }
}

// Points the clone's entries, owner, partner and accum root at their clones (the originals where none was cloned);
// a sequence without an owner only passes the cloning process on to its interpolators and controllers.
void NiControllerSequence::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);
    NiObject* pkClone = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkClone);
    NiControllerSequence* pkDest = static_cast<NiControllerSequence*>(pkClone);
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem* pkItem = &m_pkInterpArray[ui];
        if (!pkItem->m_spInterpolator.m_pObject) {
            continue;
        }
        if (!m_pkOwner) {
            pkItem->m_spInterpolator.m_pObject->ProcessClone(kCloning);
            NiInterpController* pkCtlr = m_pkInterpArray[ui].m_spInterpCtlr.m_pObject;
            if (pkCtlr) {
                pkCtlr->ProcessClone(kCloning);
            }
            continue;
        }

        NiInterpolator* pkInterp = pkItem->m_spInterpolator.m_pObject;
        if (m_eState == kInactive) {
            pkInterp->ProcessClone(kCloning);
        }
        else if (kCloning.m_pkCloneMap->GetAt(pkInterp, pkClone)) {
            pkDest->m_pkInterpArray[ui].m_spInterpolator.SetNew(static_cast<NiInterpolator*>(pkClone));
        }
        else {
            pkDest->m_pkInterpArray[ui].m_spInterpolator = m_pkInterpArray[ui].m_spInterpolator;
        }

        if (kCloning.m_pkCloneMap->GetAt(m_pkInterpArray[ui].m_spInterpCtlr.m_pObject, pkClone)) {
            NiPointer<NiInterpController>* pspCtlr = &pkDest->m_pkInterpArray[ui].m_spInterpCtlr;
            NiInterpController* pkOldCtlr = pspCtlr->m_pObject;
            // The clone is read again after the release, as the game does.
            if (pkOldCtlr != pkClone) {
                if (pkOldCtlr) {
                    NiDecRefCount(pkOldCtlr);
                }
                NiObject* pkNewCtlr = pkClone;
                pspCtlr->m_pObject = static_cast<NiInterpController*>(pkNewCtlr);
                if (pkNewCtlr) {
                    NiIncRefCount(pkNewCtlr);
                }
            }
        }
        else {
            pkDest->m_pkInterpArray[ui].m_spInterpCtlr = m_pkInterpArray[ui].m_spInterpCtlr;
        }

        if (kCloning.m_pkCloneMap->GetAt(m_pkInterpArray[ui].m_pkBlendInterp, pkClone)) {
            pkDest->m_pkInterpArray[ui].m_pkBlendInterp = static_cast<NiBlendInterpolator*>(pkClone);
        }
        else {
            pkDest->m_pkInterpArray[ui].m_pkBlendInterp = m_pkInterpArray[ui].m_pkBlendInterp;
        }
    }
    if (kCloning.m_pkCloneMap->GetAt(m_pkOwner, pkClone)) {
        pkDest->m_pkOwner = static_cast<NiControllerManager*>(pkClone);
    }
    else {
        pkDest->m_pkOwner = m_pkOwner;
    }
    if (kCloning.m_pkCloneMap->GetAt(m_pkPartnerSequence, pkClone)) {
        pkDest->m_pkPartnerSequence = static_cast<NiControllerSequence*>(pkClone);
    }
    else {
        pkDest->m_pkPartnerSequence = m_pkPartnerSequence;
    }
    if (kCloning.m_pkCloneMap->GetAt(m_pkAccumRoot, pkClone)) {
        pkDest->m_pkAccumRoot = static_cast<NiAVObject*>(pkClone);
    }
    else {
        pkDest->m_pkAccumRoot = m_pkAccumRoot;
    }
}

// Before 10.1.0.104 the key time range and cycle come from the controllers, and each name and controller pair is moved
// from the ID tags and interpolator items to the name/controller map.
void NiControllerSequence::PostLinkObject(NiStream& kStream) {
    NiObject::PostLinkObject(kStream);

    if (m_spDeprecatedStringPalette.m_pObject) {
        for (uint32_t ui = 0; ui < m_uiArraySize; ui++) {
            m_pkIDTagArray[ui].HandleDeprecatedExtraData(
                static_cast<NiStringPalette*>(m_spDeprecatedStringPalette.m_pObject), kStream
            );
        }
        m_spDeprecatedStringPalette = nullptr;
    }

    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        uint32_t uiArraySize = m_uiArraySize;
        m_fBeginKeyTime = FLT_MAX;
        m_fEndKeyTime = -FLT_MAX;
        for (uint32_t ui = 0; ui < uiArraySize; ui++) {
            NiInterpController* pkCtlr = m_pkInterpArray[ui].m_spInterpCtlr.m_pObject;
            if (m_fBeginKeyTime > pkCtlr->m_fLoKeyTime) {
                m_fBeginKeyTime = pkCtlr->m_fLoKeyTime;
            }
            if (m_fEndKeyTime < pkCtlr->m_fHiKeyTime) {
                m_fEndKeyTime = pkCtlr->m_fHiKeyTime;
            }
        }
        if (m_fBeginKeyTime == FLT_MAX && m_fEndKeyTime == -FLT_MAX) {
            m_fEndKeyTime = 0.0f;
            m_fBeginKeyTime = 0.0f;
        }

        XFNV_INLINED_LOCAL(NiTPrimitiveArray<char*>, kNames, std::nullopt, std::nullopt);
        kNames.m_usSize = 0;
        if (static_cast<uint16_t>(uiArraySize)) {
            // EXE-INLINED 0x96AFC0
            kNames.m_pBase = static_cast<char**>(_NiMalloc(sizeof(char*) * static_cast<uint16_t>(uiArraySize)));
        }
        else {
            kNames.m_pBase = nullptr;
        }
        XFNV_INLINED_LOCAL(NiTObjectArray<NiPointer<NiInterpController>>, kCtlrs, std::nullopt, std::nullopt);
        ::new (&kCtlrs) NiTObjectArray<NiPointer<NiInterpController>>(static_cast<uint16_t>(m_uiArraySize), 1u);

        for (uint32_t ui = 0; ui < m_uiArraySize; ui++) {
            const char* pcStoredName = m_pkIDTagArray[ui].m_kAVObjectName.GetHandle();
            uint32_t uiLen = std::strlen(pcStoredName) + 1;
            char* pcName = static_cast<char*>(_NiMalloc(uiLen));
            strcpy_s(pcName, uiLen, pcStoredName);
            // EXE-INLINED 0x96AE90
            if (ui >= kNames.m_usSize) {
                kNames.m_usSize = static_cast<uint16_t>(ui + 1);
            }
            kNames.m_pBase[ui] = pcName;
            m_pkIDTagArray[ui].ClearValues();

            kCtlrs.SetAt(ui, NiPointer<NiInterpController>(m_pkInterpArray[ui].m_spInterpCtlr.m_pObject));
            InterpArrayItem& kItem = m_pkInterpArray[ui];
            // EXE-INLINED 0xA31260
            kItem.m_spInterpolator = nullptr;
            kItem.m_spInterpCtlr = nullptr;
            kItem.m_pkBlendInterp = nullptr;
            kItem.m_ucBlendIdx = NiBlendInterpolator::INVALID_INDEX;
            kItem.m_ucPriority = 0xFF;
        }

        uint32_t uiCtlrs = kCtlrs.m_usSize;
        for (uint32_t ui = 0; ui < uiCtlrs; ui++) {
            NiInterpController* pkCtlr = kCtlrs.m_pBase[ui].m_pObject;
            if (ui == 0) {
                m_eCycleType = static_cast<CycleType>((pkCtlr->m_usFlags >> 1) & 3);
                if (m_eCycleType != CYCLE_CLAMP && m_eCycleType != CYCLE_LOOP) {
                    m_eCycleType = CYCLE_LOOP;
                }
                m_fFrequency = pkCtlr->m_fFrequency;
            }
            AddNameControllerPair(kNames.m_pBase[ui], pkCtlr);
        }

        uint32_t uiNames = kNames.m_usSize;
        for (uint32_t ui = 0; ui < uiNames; ui++) {
            _NiFree(kNames.m_pBase[ui]);
        }
        kCtlrs.~NiTObjectArray();
        kNames.~NiTPrimitiveArray();
    }
}

bool NiControllerSequence::Deactivate(float fEaseOutTime, bool bTransition) {
    State eState = m_eState;
    if (eState == kInactive) {
        return false;
    }
    if (!(fEaseOutTime > 0.0f)) {
        if (m_fLastTime != -FLT_MAX) {
            m_fOffset = m_fWeightedLastTime / m_fFrequency - m_fLastTime + m_fOffset;
        }
        if (eState == kTransDest) {
            NiControllerManager* pkOwner = m_pkOwner;
            if (pkOwner) {
                pkOwner->StopTransSourceSequence(0.0f);
            }
        }
        m_fDestFrame = -FLT_MAX;
        m_eState = kInactive;
        m_pkPartnerSequence = nullptr;
        DetachInterpolators();
        return true;
    }
    m_fStartTime = -FLT_MAX;
    m_fEndTime = fEaseOutTime;
    m_eState = bTransition ? kTransSource : kEaseOut;
    return true;
}

// Deactivates a running sequence, then releases every interpolator and blend interpolator of the target list.
void NiControllerSequence::ClearTargets() {
    if (m_eState != kInactive) {
        Deactivate(0.0f, false);
    }
    for (uint32_t i = 0; i < m_uiArraySize; i++) {
        m_pkInterpArray[i].m_spInterpCtlr = nullptr;
        m_pkInterpArray[i].m_pkBlendInterp = nullptr;
    }
}

// This sequence is activated as the morph source timed to pkDestSequence, which then starts at fDestWeight
// (animating at once when fDuration is not above 0, a NaN included; else as the transition destination), and this
// sequence becomes the morph source.
bool NiControllerSequence::StartMorph(
    NiControllerSequence* pkDestSequence,
    float fDuration,
    int32_t iPriority,
    float fSourceWeight,
    float fDestWeight
) {
    Deactivate(0.0f, true);
    if (!Activate(iPriority, false, fSourceWeight, fDuration, pkDestSequence, true) ||
        pkDestSequence->m_eState != kInactive) {
        return false;
    }
    pkDestSequence->m_fSeqWeight = fDestWeight;
    pkDestSequence->m_pkPartnerSequence = nullptr;
    pkDestSequence->AttachInterpolators(iPriority);
    float fInvalidTime;
    if (!(fDuration > 0.0f)) {
        bool bWasInactive = (pkDestSequence->m_eState == kInactive);
        pkDestSequence->m_eState = kAnimating;
        if (bWasInactive) {
            NiControllerSequence* pkSequence = pkDestSequence;
            pkDestSequence->m_pkOwner->m_kActiveSequences.AddUnique(pkSequence);
        }
        pkDestSequence->m_fStartTime = 0.0f;
        fInvalidTime = -FLT_MAX;
    }
    else {
        bool bWasInactive = (pkDestSequence->m_eState == kInactive);
        pkDestSequence->m_eState = kTransDest;
        if (bWasInactive) {
            NiControllerSequence* pkSequence = pkDestSequence;
            pkDestSequence->m_pkOwner->m_kActiveSequences.AddUnique(pkSequence);
        }
        pkDestSequence->m_fStartTime = -FLT_MAX;
        pkDestSequence->m_fEndTime = fDuration;
        fInvalidTime = -FLT_MAX;
    }
    pkDestSequence->m_fOffset = fInvalidTime;
    pkDestSequence->m_fLastTime = fInvalidTime;

    bool bWasInactive = (m_eState == kInactive);
    m_eState = kMorphSource;
    if (bWasInactive) {
        NiControllerSequence* pkSequence = this;
        m_pkOwner->m_kActiveSequences.AddUnique(pkSequence);
    }
    return true;
}

NiControllerSequence::State NiControllerSequence::GetState() {
    return m_eState;
}

// Copies the sequence into apDest: the interpolators cloned (shared clones with their blend slots when the sequence
// has an owner, else full clones of the interpolators and their controllers), the tags, the key times and the notes.
void NiControllerSequence::CopyMembers(NiControllerSequence* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    const char* m_kHandle = m_kName.GetHandle();
    if (pkDest->m_kName.GetHandle() != m_kHandle) {
        NiGlobalStringTable::AcquireHandle(m_kHandle);
        const char* v6 = pkDest->m_kName.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v6);
        pkDest->m_kName.m_kHandle = m_kHandle;
    }
    pkDest->m_uiArraySize = m_uiArraySize;
    pkDest->m_uiArrayGrowBy = m_uiArrayGrowBy;

    // Emulates `new InterpArrayItem[n]` and `new IDTag[n]`: the byte count saturates to SIZE_MAX on overflow, and the
    // block is untyped memory laid out by hand, with the count in front of the elements.
    uint32_t uiCount = m_uiArraySize;
    uint64_t ullSize = uint64_t{sizeof(InterpArrayItem)} * uiCount;
    size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
    InterpArrayItem* pkInterpArray = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiCount;
        pkInterpArray = reinterpret_cast<InterpArrayItem*>(static_cast<size_t*>(pvMemory) + 1);
        rt::eh_vector_constructor_iterator(
            pkInterpArray, sizeof(InterpArrayItem), uiCount,
            &xfnv::constructElement<NiControllerSequence::InterpArrayItem>
        );
    }
    pkDest->m_pkInterpArray = pkInterpArray;

    uiCount = m_uiArraySize;
    ullSize = uint64_t{sizeof(IDTag)} * uiCount;
    stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    pvMemory = MemoryManager_Singleton.Allocate(stSize);
    IDTag* pkIDTagArray = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiCount;
        pkIDTagArray = reinterpret_cast<IDTag*>(static_cast<size_t*>(pvMemory) + 1);
        rt::eh_vector_constructor_iterator(
            pkIDTagArray, sizeof(IDTag), uiCount, &xfnv::constructElement<NiControllerSequence::IDTag>
        );
    }
    pkDest->m_pkIDTagArray = pkIDTagArray;

    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem* pkItem = &m_pkInterpArray[ui];
        if (pkItem->m_spInterpolator.m_pObject) {
            if (m_pkOwner) {
                NiObject* pkClone = pkItem->m_spInterpolator.m_pObject->CreateSharedClone(kCloning);
                NiPointer<NiInterpolator>* pspInterp = &pkDest->m_pkInterpArray[ui].m_spInterpolator;
                NiInterpolator* pkOldInterp = pspInterp->m_pObject;
                if (pkOldInterp != pkClone) {
                    if (pkOldInterp) {
                        NiDecRefCount(pkOldInterp);
                    }
                    pspInterp->m_pObject = static_cast<NiInterpolator*>(pkClone);
                    if (pkClone) {
                        NiIncRefCount(pkClone);
                    }
                }
                pkDest->m_pkInterpArray[ui].m_ucBlendIdx = m_pkInterpArray[ui].m_ucBlendIdx;
            }
            else {
                NiObject* pkClone = pkItem->m_spInterpolator.m_pObject->CreateClone(kCloning);
                NiPointer<NiInterpolator>* pspInterp = &pkDest->m_pkInterpArray[ui].m_spInterpolator;
                NiInterpolator* pkOldInterp = pspInterp->m_pObject;
                if (pkOldInterp != pkClone) {
                    if (pkOldInterp) {
                        NiDecRefCount(pkOldInterp);
                    }
                    pspInterp->m_pObject = static_cast<NiInterpolator*>(pkClone);
                    if (pkClone) {
                        NiIncRefCount(pkClone);
                    }
                }
                NiInterpController* pkCtlr = m_pkInterpArray[ui].m_spInterpCtlr.m_pObject;
                if (pkCtlr) {
                    NiInterpController* pkCtlrClone = static_cast<NiInterpController*>(pkCtlr->CreateClone(kCloning));
                    NiPointer<NiInterpController>* pspCtlr = &pkDest->m_pkInterpArray[ui].m_spInterpCtlr;
                    NiInterpController* pkOldCtlr = pspCtlr->m_pObject;
                    if (pkOldCtlr != pkCtlrClone) {
                        if (pkOldCtlr) {
                            NiDecRefCount(pkOldCtlr);
                        }
                        pspCtlr->m_pObject = pkCtlrClone;
                        if (pkCtlrClone) {
                            NiIncRefCount(pkCtlrClone);
                        }
                    }
                }
            }
        }
        pkDest->m_pkInterpArray[ui].m_ucPriority = m_pkInterpArray[ui].m_ucPriority;
        pkDest->m_pkIDTagArray[ui].Operator_Set(&m_pkIDTagArray[ui]);
    }
    pkDest->m_fSeqWeight = m_fSeqWeight;
    pkDest->m_spTextKeys = m_spTextKeys;
    pkDest->m_eCycleType = m_eCycleType;
    pkDest->m_fFrequency = m_fFrequency;
    pkDest->m_fBeginKeyTime = m_fBeginKeyTime;
    pkDest->m_fEndKeyTime = m_fEndKeyTime;
    const char* v34 = m_kAccumRootName.GetHandle();
    if (pkDest->m_kAccumRootName.GetHandle() != v34) {
        NiGlobalStringTable::AcquireHandle(v34);
        const char* v35 = pkDest->m_kAccumRootName.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v35);
        pkDest->m_kAccumRootName.m_kHandle = v34;
    }
    pkDest->ssCurAnimNIdx = ssCurAnimNIdx;
    pkDest->usNumNotes = usNumNotes;
    if (usNumNotes) {
        // `new NiPointer<BSAnimNote>[n]`, laid out by hand as the arrays above, with its elements null-constructed.
        uint32_t uiNotes = usNumNotes;
        ullSize = uint64_t{sizeof(NiPointer<BSAnimNote>)} * uiNotes;
        stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
        stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
        pvMemory = MemoryManager_Singleton.Allocate(stSize);
        NiPointer<BSAnimNote>* pspAnimNotes = nullptr;
        if (pvMemory) {
            *static_cast<size_t*>(pvMemory) = uiNotes;
            pspAnimNotes = reinterpret_cast<NiPointer<BSAnimNote>*>(static_cast<size_t*>(pvMemory) + 1);
            for (uint32_t ui = 0; ui < uiNotes; ++ui) {
                pspAnimNotes[ui].m_pObject = nullptr;
            }
        }
        pkDest->spAnimNotesA = pspAnimNotes;
    }
    for (uint16_t us = 0; us < usNumNotes; ++us) {
        pkDest->spAnimNotesA[us] = spAnimNotesA[us];
    }
}

// AddInterpInfo reads only the low byte of the priority argument, so whether cPriority is zero- or sign-extended does
// not matter.
void NiControllerSequence::AttachInterpolators(int32_t cPriority) {
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem* pItem = &m_pkInterpArray[ui];
        if (!pItem->m_spInterpolator || !pItem->m_pkBlendInterp) {
            continue;
        }
        // A priority of 0xFF means the caller's priority; anything else is the priority stored in the item.
        char cItemPriority =
            (pItem->m_ucPriority == 0xFF) ? static_cast<char>(cPriority) : static_cast<char>(pItem->m_ucPriority);
        pItem->m_ucBlendIdx =
            pItem->m_pkBlendInterp->AddInterpInfo(pItem->m_spInterpolator, m_fSeqWeight, cItemPriority, 1.0f);
    }
}

void NiControllerSequence::DetachInterpolators() {
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem* pItem = &m_pkInterpArray[ui];
        if (pItem->m_pkBlendInterp) {
            pItem->m_pkBlendInterp->RemoveInterpInfo(pItem->m_ucBlendIdx);
        }
    }
}

float NiControllerSequence::ComputeScaledTime(float fTime, bool bStoreLastTime) {
    float fWeightedLastTime = m_fWeightedLastTime;
    float fDeltaTime;
    // -NI_INFINITY, compared as the double -FLT_MAX.
    if (m_fLastTime == -3.4028234663852886e+38) {
        fWeightedLastTime = 0.0f;
        fDeltaTime = fTime;
    }
    else {
        fDeltaTime = fTime - m_fLastTime;
    }
    fWeightedLastTime = m_fFrequency * fDeltaTime + fWeightedLastTime;
    float fScaledTime = fWeightedLastTime;
    if (m_eCycleType == CYCLE_LOOP) {
        float fSpan = m_fEndKeyTime - m_fBeginKeyTime;
        if (fSpan != 0.0f) {
            // The asm tests fScaledTime != m_fEndKeyTime as fSpan != fScaledTime - m_fBeginKeyTime (rounded to
            // float), the operand it passes to __CIfmod.
            float fOffset = fScaledTime - m_fBeginKeyTime;
            if (fSpan != fOffset) {
                float fMod = std::fmod(fOffset, fSpan);
                fScaledTime = fMod + m_fBeginKeyTime;
            }
            else {
                fScaledTime = m_fEndKeyTime;
            }
            if (fScaledTime < m_fBeginKeyTime) {
                fScaledTime += fSpan;
            }
        }
        else {
            fScaledTime = m_fBeginKeyTime;
        }
    }
    if (fScaledTime > m_fEndKeyTime) {
        fScaledTime = m_fEndKeyTime;
    }
    else if (fScaledTime < m_fBeginKeyTime) {
        fScaledTime = m_fBeginKeyTime;
    }
    if (bStoreLastTime) {
        m_fWeightedLastTime = fWeightedLastTime;
        m_fLastTime = fTime;
        m_fLastScaledTime = fScaledTime;
    }
    return fScaledTime;
}

// Builds the object's name and optional property line.
void NiControllerSequence::GetObjectNameAt(uint32_t uiIndex, char* pcObjectName) {
    if (!pcObjectName) {
        return;
    }

    if (!m_pkInterpArray[uiIndex].m_spInterpolator.m_pObject) {
        *pcObjectName = 0;
        return;
    }

    IDTag* pkTag = &m_pkIDTagArray[uiIndex];
    const char* pcObjectNameAt = pkTag->m_kAVObjectName.GetHandle();
    size_t stObjectNameLength = 0;
    while (pcObjectNameAt[stObjectNameLength]) {
        ++stObjectNameLength;
    }

    const char* pcPropertyType = pkTag->m_kPropertyType.GetHandle();
    if (pcPropertyType) {
        const char* pcPropertyPrefix = "PROP\n";
        size_t stPrefixLength = 0;
        while (pcPropertyPrefix[stPrefixLength]) {
            ++stPrefixLength;
        }
        size_t stPropertyLength = 0;
        while (pcPropertyType[stPropertyLength]) {
            ++stPropertyLength;
        }
        NiSprintf(
            pcObjectName, stObjectNameLength + stPrefixLength + stPropertyLength + 2, "%s\n%s%s", pcObjectNameAt,
            pcPropertyPrefix, pcPropertyType
        );
    }
    else {
        strcpy_s(pcObjectName, stObjectNameLength + 1, pcObjectNameAt);
    }
}

// Sets controller phases except for empty tags and the special transform controller, then stores the sequence offset.
void NiControllerSequence::SetPhase(float afPhase, bool abUpdateControllers) {
    float fOffset = m_fBeginKeyTime + afPhase;
    if (abUpdateControllers) {
        uint32_t uiIndex = 0;
        while (uiIndex < m_uiArraySize) {
            NiInterpController* pkController = m_pkInterpArray[uiIndex].m_spInterpCtlr.m_pObject;
            const char* pcControllerType = m_pkIDTagArray[uiIndex].m_kCtlrType.GetHandle();
            if (pkController && pcControllerType && pcControllerType != NiGlobalStringTable::NULL_STRING &&
                (pcControllerType[2] != 'T' || pcControllerType[7] != 'f')) {
                pkController->m_fPhase = afPhase;
            }
            ++uiIndex;
        }
    }
    m_fOffset = fOffset;
}

// Grows both arrays by m_uiArrayGrowBy: the old entries are copied over, then the old arrays freed.
bool NiControllerSequence::ReallocateArrays() {
    if (!m_uiArrayGrowBy) {
        return false;
    }
    uint32_t uiNewSize = m_uiArrayGrowBy + m_uiArraySize;

    uint64_t ullSize = uint64_t{sizeof(InterpArrayItem)} * uiNewSize;
    size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
    InterpArrayItem* pkNewInterpArray = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiNewSize;
        pkNewInterpArray = reinterpret_cast<InterpArrayItem*>(static_cast<size_t*>(pvMemory) + 1);
        rt::eh_vector_constructor_iterator(
            pkNewInterpArray, sizeof(InterpArrayItem), uiNewSize,
            &xfnv::constructElement<NiControllerSequence::InterpArrayItem>
        );
    }

    ullSize = uint64_t{sizeof(IDTag)} * uiNewSize;
    stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    pvMemory = MemoryManager_Singleton.Allocate(stSize);
    IDTag* pkNewIDTagArray = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiNewSize;
        pkNewIDTagArray = reinterpret_cast<IDTag*>(static_cast<size_t*>(pvMemory) + 1);
        rt::eh_vector_constructor_iterator(
            pkNewIDTagArray, sizeof(IDTag), uiNewSize, &xfnv::constructElement<NiControllerSequence::IDTag>
        );
    }

    // m_uiArraySize is read again on every pass.
    for (uint32_t ui = 0; ui < uiNewSize; ++ui) {
        if (ui < m_uiArraySize) {
            pkNewInterpArray[ui] = m_pkInterpArray[ui];
            pkNewIDTagArray[ui].Operator_Set(&m_pkIDTagArray[ui]);
        }
    }

    if (m_pkInterpArray) {
        m_pkInterpArray->Free(3);
    }
    if (m_pkIDTagArray) {
        m_pkIDTagArray->Free(3);
    }
    m_uiArraySize = uiNewSize;
    m_pkIDTagArray = pkNewIDTagArray;
    m_pkInterpArray = pkNewInterpArray;
    return true;
}

// Adds or reuses an interpolator slot, then copies its ID tag and priority.
uint32_t NiControllerSequence::AddInterpolator(
    NiInterpolator* pkInterpolator,
    const NiControllerSequence::IDTag& kIDTag,
    uint8_t ucPriority
) {
    uint32_t uiArraySize = m_uiArraySize;
    uint32_t uiIndex = 0;
    if (uiArraySize) {
        InterpArrayItem* pkItem = m_pkInterpArray;
        while (pkItem->m_spInterpolator.m_pObject) {
            ++uiIndex;
            ++pkItem;
            if (uiIndex >= uiArraySize) {
                break;
            }
        }
    }

    if (uiIndex == uiArraySize && !ReallocateArrays()) {
        return static_cast<uint32_t>(-1);
    }

    InterpArrayItem* pkItem = &m_pkInterpArray[uiIndex];
    pkItem->m_spInterpolator = pkInterpolator;

    m_pkIDTagArray[uiIndex].Operator_Set(&kIDTag);
    m_pkInterpArray[uiIndex].m_ucPriority = ucPriority;
    return uiIndex;
}

// The copy of the entry's interpolator takes its reference before the entry is cleared; the local is then assigned to
// the caller's aspInterpolator (releasing what it held) and released.
void NiControllerSequence::RemoveInterpolator(uint32_t uiIndex, NiPointer<NiInterpolator>& aspInterpolator) {
    NiPointer<NiInterpolator> spInterpolator = m_pkInterpArray[uiIndex].m_spInterpolator;
    m_pkInterpArray[uiIndex].ClearValues();
    m_pkIDTagArray[uiIndex].ClearValues();
    aspInterpolator = spInterpolator;
}

// Empties pkPoseSequence first when abRemInterps, takes this sequence's cycle type (loop or clamp only), frequency and
// key range, then adds to it a pose interpolator for every item that has an interpolator and a controller knowing the
// item's object name. uiArraySize and pkSequenceToSynchronize are ignored.
void NiControllerSequence::PopulatePoseSequence(
    NiControllerSequence* pkPoseSequence,
    uint32_t uiArraySize,
    NiControllerSequence* pkSequenceToSynchronize,
    bool abRemInterps
) {
    if (abRemInterps) {
        for (uint32_t ui = 0; ui < pkPoseSequence->m_uiArraySize; ui++) {
            // RemoveInterpolator fills spInterpolator, and the removed interpolator is released explicitly; the pointer
            // is cleared so the NiPointer destructor does nothing.
            NiPointer<NiInterpolator> spInterpolator;
            pkPoseSequence->RemoveInterpolator(ui, spInterpolator);
            NiInterpolator* pkRemoved = spInterpolator.m_pObject;
            spInterpolator.m_pObject = nullptr;
            if (pkRemoved) {
                pkRemoved->DecRefCount();
            }
        }
    }
    CycleType eCycleType = m_eCycleType;
    if (eCycleType == CYCLE_CLAMP || eCycleType == CYCLE_LOOP) {
        pkPoseSequence->m_eCycleType = eCycleType;
    }
    pkPoseSequence->m_fFrequency = m_fFrequency;
    pkPoseSequence->m_fBeginKeyTime = m_fBeginKeyTime;
    pkPoseSequence->m_fEndKeyTime = m_fEndKeyTime;
    for (uint32_t ui = 0; ui < m_uiArraySize; ui++) {
        InterpArrayItem& kItem = m_pkInterpArray[ui];
        NiInterpController* pkCtlr = kItem.m_spInterpCtlr.m_pObject;
        if (!kItem.m_spInterpolator.m_pObject || !pkCtlr) {
            continue;
        }
        uint16_t usIndex = pkCtlr->GetInterpolatorIndexFx(m_pkIDTagArray[ui].m_kAVObjectName);
        if (usIndex == NiInterpController::INVALID_INDEX) {
            continue;
        }
        NiInterpolator* pkInterpolator = pkCtlr->CreatePoseInterpolator(usIndex);
        alignas(IDTag) uint8_t aucTag[sizeof(IDTag)];
        IDTag* pkTag = ::new (aucTag) IDTag(EmptyFS, EmptyFS, EmptyFS, EmptyFS, EmptyFS);
        uint32_t uiNewIdx = pkPoseSequence->AddInterpolator(pkInterpolator, *pkTag, 0xFF);
        pkTag->~IDTag();
        InterpArrayItem& kPoseItem = pkPoseSequence->m_pkInterpArray[uiNewIdx];
        kPoseItem.m_pkBlendInterp = kItem.m_pkBlendInterp;
        NiInterpController* pkOldCtlr = kPoseItem.m_spInterpCtlr.m_pObject;
        if (pkOldCtlr != kItem.m_spInterpCtlr.m_pObject) {
            if (pkOldCtlr) {
                pkOldCtlr->DecRefCount();
            }
            NiInterpController* pkNewCtlr = kItem.m_spInterpCtlr.m_pObject;
            kPoseItem.m_spInterpCtlr.m_pObject = pkNewCtlr;
            if (pkNewCtlr) {
                pkNewCtlr->IncRefCount();
            }
        }
        kPoseItem.m_ucPriority = kItem.m_ucPriority;
    }
}

bool NiControllerSequence::AddNameControllerPair(const char* pcObjectName, NiTimeController* pkController) {
    if (!pcObjectName || !pkController) {
        return false;
    }
    // Walks the RTTI chain inline (NiIsKindOf).
    const NiRTTI* pkRTTI = pkController->GetRTTI();
    while (pkRTTI != &NiInterpController::ms_RTTI) {
        if (!pkRTTI) {
            return false;
        }
        pkRTTI = pkRTTI->m_pkBaseRTTI;
    }
    pkController->m_usFlags =
        uint16_t((pkController->m_usFlags & ~NiTimeController::CYCLETYPE_MASK) | (m_eCycleType << 1));
    pkController->m_fFrequency = m_fFrequency;
    pkController->m_usFlags = uint16_t(pkController->m_usFlags & ~NiTimeController::DIRECTION_MASK);
    return ConvertOldNameControllerPair(pcObjectName, static_cast<NiInterpController*>(pkController));
}

// Each tag's names are built from NiFixedString temporaries, using NULL_STRING for a null name, except the object name,
// which is never null.
bool NiControllerSequence::ConvertOldNameControllerPair(const char* pcObjectName, NiInterpController* pkController) {
    char acObjectName[260];
    char acTargetType[260];
    char acTargetName[260];
    strcpy_s(acObjectName, 260, pcObjectName);
    acTargetType[0] = '\0';
    acTargetName[0] = '\0';
    char* pcPtr = std::strchr(acObjectName, '\n');
    if (pcPtr) {
        strcpy_s(acTargetType, 260, pcPtr + 1);
        *pcPtr = '\0';
        pcPtr = std::strchr(acTargetType, '\n');
        strcpy_s(acTargetName, 260, pcPtr + 1);
        *pcPtr = '\0';
    }

    // The index is kept as 32 bits and only its low 16 bits are compared with the count.
    uint32_t uiIndex = 0;
    if (static_cast<uint16_t>(uiIndex) >= pkController->GetInterpolatorCount()) {
        return true;
    }
    do {
        XFNV_INLINED_LOCAL(IDTag, kIDTag, std::nullopt, std::nullopt);
        ::new (&kIDTag) IDTag(EmptyFS, EmptyFS, EmptyFS, EmptyFS, EmptyFS);

        // EXE-INLINED none: IDTag::SetAVObjectName
        const char* pcNew = NiGlobalStringTable::AddString(acObjectName);
        const char* pcOld = kIDTag.m_kAVObjectName.GetHandle();
        if (pcOld != pcNew) {
            if (pcNew != NiGlobalStringTable::NULL_STRING) {
                InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
            }
            if (pcOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
            }
            kIDTag.m_kAVObjectName.SetHandle(pcNew);
        }
        if (pcNew != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
        }

        // EXE-INLINED none: strcmp
        const char* pcA = acTargetType;
        const char* pcB = "PROP";
        while (*pcA == *pcB && *pcA) {
            pcA++;
            pcB++;
        }
        if (*pcA == *pcB) {
            // EXE-INLINED none: IDTag::SetPropertyType
            pcNew = NiGlobalStringTable::AddString(acTargetName);
            pcOld = kIDTag.m_kPropertyType.GetHandle();
            if (pcOld != pcNew) {
                if (pcNew != NiGlobalStringTable::NULL_STRING) {
                    InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
                }
                if (pcOld != NiGlobalStringTable::NULL_STRING) {
                    InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
                }
                kIDTag.m_kPropertyType.SetHandle(pcNew);
            }
            if (pcNew != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
            }
        }

        // EXE-INLINED none: IDTag::SetCtlrType
        const char* pcString = pkController->GetRTTI()->m_pcName;
        pcNew = pcString ? NiGlobalStringTable::AddString(pcString) : NiGlobalStringTable::NULL_STRING;
        pcOld = kIDTag.m_kCtlrType.GetHandle();
        if (pcOld != pcNew) {
            if (pcNew != NiGlobalStringTable::NULL_STRING) {
                InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
            }
            if (pcOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
            }
            kIDTag.m_kCtlrType.SetHandle(pcNew);
        }
        if (pcNew != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
        }

        // EXE-INLINED none: IDTag::SetCtlrID
        pcString = pkController->GetCtlrID();
        pcNew = pcString ? NiGlobalStringTable::AddString(pcString) : NiGlobalStringTable::NULL_STRING;
        pcOld = kIDTag.m_kCtlrID.GetHandle();
        if (pcOld != pcNew) {
            if (pcNew != NiGlobalStringTable::NULL_STRING) {
                InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
            }
            if (pcOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
            }
            kIDTag.m_kCtlrID.SetHandle(pcNew);
        }
        if (pcNew != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
        }

        // EXE-INLINED none: IDTag::SetInterpolatorID
        pcString = pkController->GetInterpolatorID(static_cast<uint16_t>(uiIndex));
        pcNew = pcString ? NiGlobalStringTable::AddString(pcString) : NiGlobalStringTable::NULL_STRING;
        pcOld = kIDTag.m_kInterpolatorID.GetHandle();
        if (pcOld != pcNew) {
            if (pcNew != NiGlobalStringTable::NULL_STRING) {
                InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
            }
            if (pcOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
            }
            kIDTag.m_kInterpolatorID.SetHandle(pcNew);
        }
        if (pcNew != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcNew) - 8));
        }

        NiInterpolator* pkInterpolator = pkController->GetInterpolator(static_cast<uint16_t>(uiIndex));
        uint32_t uiArrayIndex = AddInterpolator(pkInterpolator, kIDTag, 0xFF);
        bool bAdded = uiArrayIndex != 0xFFFFFFFFu;
        if (bAdded) {
            // EXE-INLINED none: NiPointer<NiInterpController>::operator=
            NiPointer<NiInterpController>& rspCtlr = m_pkInterpArray[uiArrayIndex].m_spInterpCtlr;
            NiInterpController* pkOldCtlr = rspCtlr.m_pObject;
            if (pkOldCtlr != pkController) {
                if (pkOldCtlr) {
                    pkOldCtlr->DecRefCount();
                }
                rspCtlr.m_pObject = pkController;
                pkController->IncRefCount();
            }
        }

        // EXE-INLINED none: ~IDTag (the five names released, last to first)
        const char* pcName = kIDTag.m_kInterpolatorID.GetHandle();
        if (pcName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcName) - 8));
        }
        pcName = kIDTag.m_kCtlrID.GetHandle();
        if (pcName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcName) - 8));
        }
        pcName = kIDTag.m_kCtlrType.GetHandle();
        if (pcName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcName) - 8));
        }
        pcName = kIDTag.m_kPropertyType.GetHandle();
        if (pcName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcName) - 8));
        }
        pcName = kIDTag.m_kAVObjectName.GetHandle();
        if (pcName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcName) - 8));
        }
        if (!bAdded) {
            return false;
        }

        uiIndex++;
    }
    while (static_cast<uint16_t>(uiIndex) < pkController->GetInterpolatorCount());
    return true;
}

void NiControllerSequence::SetInterpsWeightAndTime(float fWeight, float fEaseSpinner, float fTime) {
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem* pItem = &m_pkInterpArray[ui];
        NiBlendInterpolator* pBlend = pItem->m_pkBlendInterp;
        if (!pBlend) {
            continue;
        }
        uint8_t ucBlendIdx = pItem->m_ucBlendIdx;
        if (pBlend->m_ucInterpCount == 1 && ucBlendIdx == pBlend->m_ucSingleIdx) {
            pBlend->m_fSingleTime = fTime;
            continue;
        }
        NiBlendInterpolator::InterpArrayItem* pSlot = &pBlend->m_pkInterpArray[ucBlendIdx];
        pSlot->m_fUpdateTime = fTime;
        pSlot->m_fWeight = fWeight;
        pSlot->m_fEaseSpinner = fEaseSpinner;
        pBlend->SetComputeNormalizedWeights(true);
        pBlend->ClearWeightSums();
    }
}

bool NiControllerSequence::Activate(
    int32_t cPriority,
    bool bStartOver,
    float fWeight,
    float fEaseInTime,
    NiControllerSequence* pkTimeSyncSeq,
    bool bTransition
) {
    if (m_eState != kInactive) {
        return false;
    }
    m_pkPartnerSequence = nullptr;
    if (pkTimeSyncSeq) {
        NiControllerSequence* pkPartner = pkTimeSyncSeq->m_pkPartnerSequence;
        if ((pkPartner) && (pkPartner == this || !VerifyDependencies(pkPartner))) {
            return false;
        }

        if (!VerifyMatchingMorphKeys(pkTimeSyncSeq)) {
            return false;
        }
        m_pkPartnerSequence = pkTimeSyncSeq;
    }
    m_fSeqWeight = fWeight;
    AttachInterpolators(cPriority);
    float fInvalidTime;
    if (!(fEaseInTime > 0.0f)) {
        bool bWasInactive = (m_eState == kInactive);
        m_eState = kAnimating;
        if (bWasInactive) {
            NiControllerSequence* pkSequence = this;
            m_pkOwner->m_kActiveSequences.AddUnique(pkSequence);
        }
        m_fStartTime = 0.0f;
        fInvalidTime = -FLT_MAX;
    }
    else {
        bool bWasInactive = (m_eState == kInactive);
        // Tests bTransition's byte rather than reading it as a plain bool.
        m_eState = static_cast<uint8_t>(bTransition) ? kTransDest : kEaseIn;
        if (bWasInactive) {
            NiControllerSequence* pkSequence = this;
            m_pkOwner->m_kActiveSequences.AddUnique(pkSequence);
        }
        m_fStartTime = -FLT_MAX;
        m_fEndTime = fEaseInTime;
        fInvalidTime = -FLT_MAX;
    }
    if (bStartOver) {
        m_fOffset = fInvalidTime;
    }
    m_fLastTime = fInvalidTime;
    return true;
}

// Deactivates this sequence, then activates it again at once (the source weight, no partner, the interpolators
// attached, the state animating) and the destination over fDuration, raised to 0.0001 when not above 0 (a NaN duration
// stays). This sequence then eases out over fDuration and both destination frames are set. iPriority is passed whole to
// AttachInterpolators and Activate.
bool NiControllerSequence::StartBlend(
    NiControllerSequence* pkDestSequence,
    float fDuration,
    float fDestFrame,
    int32_t iPriority,
    float fSourceWeight,
    float fDestWeight,
    NiControllerSequence* pkTimeSyncSeq
) {
    Deactivate(0.0f, true);
    if (fDuration <= 0.0f) {
        fDuration = 0.0001f;
    }
    if (pkDestSequence->m_eState != kInactive || m_eState != kInactive) {
        return false;
    }
    // EXE-INLINED 0xA34F20
    m_fSeqWeight = fSourceWeight;
    m_pkPartnerSequence = nullptr;
    AttachInterpolators(iPriority);
    bool bWasInactive = (m_eState == kInactive);
    m_eState = kAnimating;
    if (bWasInactive) {
        NiControllerSequence* pkSequence = this;
        m_pkOwner->m_kActiveSequences.AddUnique(pkSequence);
    }
    m_fStartTime = 0.0f;
    m_fLastTime = -FLT_MAX;
    if (!pkDestSequence->Activate(iPriority, false, fDestWeight, fDuration, pkTimeSyncSeq, true)) {
        return false;
    }
    Deactivate(fDuration, true);
    m_fDestFrame = m_fLastScaledTime / m_fFrequency;
    pkDestSequence->m_fDestFrame = fDestFrame / pkDestSequence->m_fFrequency;
    return true;
}

// True when this sequence can take over pkTargetSequence's targets: the target's partner sequence is absent or is
// not this sequence and its dependencies match, and the morph keys line up.
bool NiControllerSequence::CanSyncTo(NiControllerSequence* pkTargetSequence) {
    if (!pkTargetSequence) {
        return false;
    }
    NiControllerSequence* pkPartner = pkTargetSequence->m_pkPartnerSequence;
    if ((pkPartner) && (pkPartner == this || !VerifyDependencies(pkPartner))) {
        return false;
    }

    if (!VerifyMatchingMorphKeys(pkTargetSequence)) {
        return false;
    }
    return true;
}

// False when this sequence is in pkSequence's partner chain.
// KEEP: the exe's callers read edx after the call (checkbody: "caller relies on preserved edx"); the exe body
// touches only eax (and reads ecx), which clang does not guarantee.
bool NiControllerSequence::VerifyDependencies(NiControllerSequence* pkSequence) {
    for (NiControllerSequence* pkPartner = pkSequence->m_pkPartnerSequence; pkPartner;
         pkPartner = pkPartner->m_pkPartnerSequence) {
        if (pkPartner == this) {
            return false;
        }
    }
    return true;
}

bool NiControllerSequence::VerifyMatchingMorphKeys(NiControllerSequence* pkTimeSyncSeq) {
    if (!m_spTextKeys.m_pObject || !pkTimeSyncSeq->m_spTextKeys.m_pObject) {
        return false;
    }
    // EXE-INLINED none: NiTextKeyExtraData::GetKeys (both)
    NiTextKeyExtraData* pkTextKeys1 = pkTimeSyncSeq->m_spTextKeys.m_pObject;
    NiTextKeyExtraData* pkTextKeys2 = m_spTextKeys.m_pObject;
    uint32_t uiNumKeys1 = pkTextKeys1->m_uiNumKeys;
    NiTextKey* pkKeys1 = pkTextKeys1->m_pKeys;
    uint32_t uiNumKeys2 = pkTextKeys2->m_uiNumKeys;
    NiTextKey* pkKeys2 = pkTextKeys2->m_pKeys;

    bool bMorphKeysExist1 = false;
    for (uint32_t ui = 0; ui < uiNumKeys1; ui++) {
        const char* pcText1 = pkKeys1[ui].m_kText.GetHandle();
        if (NiStrnicmp(pcText1, pMorphTag, iMorphTagLen) == 0) {
            bMorphKeysExist1 = true;
            bool bFound = false;
            for (uint32_t uj = 0; uj < uiNumKeys2; uj++) {
                // EXE-INLINED none: strcmp
                const char* pcA = pcText1;
                const char* pcB = pkKeys2[uj].m_kText.GetHandle();
                while (*pcA == *pcB && *pcA) {
                    pcA++;
                    pcB++;
                }
                if (*pcA == *pcB) {
                    bFound = true;
                    break;
                }
            }
            if (!bFound) {
                return false;
            }
        }
    }
    if (!bMorphKeysExist1) {
        return false;
    }

    bool bMorphKeysExist2 = false;
    for (uint32_t ui = 0; ui < uiNumKeys2; ui++) {
        const char* pcText2 = pkKeys2[ui].m_kText.GetHandle();
        if (NiStrnicmp(pcText2, pMorphTag, iMorphTagLen) == 0) {
            bMorphKeysExist2 = true;
            bool bFound = false;
            for (uint32_t uj = 0; uj < uiNumKeys1; uj++) {
                // EXE-INLINED none: strcmp
                const char* pcA = pcText2;
                const char* pcB = pkKeys1[uj].m_kText.GetHandle();
                while (*pcA == *pcB && *pcA) {
                    pcA++;
                    pcB++;
                }
                if (*pcA == *pcB) {
                    bFound = true;
                    break;
                }
            }
            if (!bFound) {
                return false;
            }
        }
    }
    return bMorphKeysExist2;
}

void NiControllerSequence::Update(float fTime, bool bUpdateInterpolators) {
    NiControllerSequence* pThis = this;
    float fStartFrame;
    NiControllerSequence::State m_eState = pThis->m_eState;
    if (m_eState != NiControllerSequence::kInactive) {
        double m_fLastScaledTime;
        double v5 = -3.4028234663852886e+38;
        double v6 = fTime;
        if (-3.4028234663852886e+38 == pThis->m_fOffset) {
            pThis->m_fOffset = -v6;
        }
        if (pThis->m_fStartTime == -3.4028235e+38f) {
            pThis->m_fStartTime = fTime;
            pThis->m_fEndTime = pThis->m_fEndTime + v6;
        }
        float fEaseSpinner = 1.0f;
        fStartFrame = 1.0f;
        switch (m_eState) {
            case NiControllerSequence::kAnimating: {
                m_fLastScaledTime = pThis->m_fLastScaledTime;
                goto LABEL_33;
            }
            case NiControllerSequence::kEaseIn: {
                if (!(pThis->m_fEndTime > v6)) {
                    goto LABEL_18;
                }
                if (pThis->m_fStartTime > v6) {
                    fTime = pThis->m_fStartTime;
                    v6 = fTime;
                }
                fEaseSpinner = (v6 - pThis->m_fStartTime) / (pThis->m_fEndTime - pThis->m_fStartTime);
                goto LABEL_34;
            }
            case NiControllerSequence::kEaseOut: {
                if (!(pThis->m_fEndTime > v6)) {
                    pThis->Deactivate(0.0f, false);
                    return;
                }
                if (pThis->m_fStartTime > v6) {
                    fTime = pThis->m_fStartTime;
                    v6 = fTime;
                }
                fEaseSpinner = (pThis->m_fEndTime - v6) / (pThis->m_fEndTime - pThis->m_fStartTime);
                goto LABEL_34;
            }
            case NiControllerSequence::kTransSource: {
                goto LABEL_27;
            }
            case NiControllerSequence::kTransDest: {
                if (!(pThis->m_fEndTime > v6)) {
                    if (pThis->m_fDestFrame != -3.4028235e+38f) {
                        pThis->m_fOffset = pThis->m_fDestFrame - v6;
                        pThis->m_fDestFrame = -3.4028235e+38f;
                    }
                LABEL_18:;
                    m_fLastScaledTime = 0.0;
                    pThis->m_eState = NiControllerSequence::kAnimating;
                LABEL_33:;
                    pThis->m_fStartTime = m_fLastScaledTime;
                }
                else {
                    if (pThis->m_fStartTime > v6) {
                        fTime = pThis->m_fStartTime;
                        v6 = fTime;
                    }
                    fStartFrame = (v6 - pThis->m_fStartTime) / (pThis->m_fEndTime - pThis->m_fStartTime);
                }
                goto LABEL_34;
            }
            case NiControllerSequence::kMorphSource: {
                NiControllerSequence* m_pkPartnerSequence;
                NiControllerSequence* v9;
                bool v10;
                m_pkPartnerSequence = pThis->m_pkPartnerSequence;
                fStartFrame = v6 + pThis->m_fOffset;
                // The result is rounded to a float before dividing. Scoped, so that the gotos into this case's later
                // labels do not cross its initialisation.
                {
                    float fMorphFrame = m_pkPartnerSequence->FindCorrespondingMorphFrame(pThis, fStartFrame);
                    v9 = pThis->m_pkPartnerSequence;
                    fStartFrame = fMorphFrame / v9->m_fFrequency;
                }
                v9->m_fOffset = fStartFrame - fTime;
                v10 = pThis->m_eState == NiControllerSequence::kInactive;
                pThis->m_eState = NiControllerSequence::kTransSource;
                if (v10) {
                    NiTPrimitiveSet<NiControllerSequence*>* p_m_kActiveSequences =
                        &pThis->m_pkOwner->m_kActiveSequences;
                    NiControllerSequence* pkSequence = pThis;
                    p_m_kActiveSequences->AddUnique(pkSequence);
                    v5 = -3.4028234663852886e+38;
                    v6 = fTime;
                }
                else {
                    v6 = fTime;
                    v5 = -3.4028234663852886e+38;
                }
            LABEL_27:;
                if (!(pThis->m_fEndTime > v6)) {
                    pThis->Deactivate(0.0f, true);
                }
                else {
                    if (pThis->m_fStartTime > v6) {
                        fTime = pThis->m_fStartTime;
                        v6 = fTime;
                    }
                    fStartFrame = (pThis->m_fEndTime - v6) / (pThis->m_fEndTime - pThis->m_fStartTime);
                LABEL_34:;
                    float flastUpdateTime = pThis->m_fLastScaledTime;
                    if (bUpdateInterpolators) {
                        double fUpdateTime;
                        bool v12 = pThis->m_fDestFrame == v5;
                        double v13 = v6;
                        if (v12) {
                            NiControllerSequence* v15 = pThis->m_pkPartnerSequence;
                            if (v15) {
                                if (v15->m_fOffset + v6 != v15->m_fLastTime) {
                                    v15->Update(v6, false);
                                    v13 = fTime;
                                }
                                float thisa = v13 + pThis->m_pkPartnerSequence->m_fOffset;
                                // The result is rounded to a float before dividing.
                                float fUpdateTime_2 =
                                    pThis->FindCorrespondingMorphFrame(pThis->m_pkPartnerSequence, thisa);
                                fUpdateTime = fUpdateTime_2 / pThis->m_fFrequency;
                            }
                            else {
                                fUpdateTime = v6 + pThis->m_fOffset;
                            }
                        }
                        else {
                            fUpdateTime = pThis->m_fDestFrame;
                        }
                        float fUpdateTime_3 = fUpdateTime;
                        float fTimea = pThis->ComputeScaledTime(fUpdateTime_3, true);
                        float fWeight = pThis->m_fSeqWeight * fStartFrame;
                        pThis->SetInterpsWeightAndTime(fWeight, fEaseSpinner, fTimea);
                        if (pThis->spAnimNotesA) {
                            if (pThis->m_fLastScaledTime < static_cast<double>(flastUpdateTime)) {
                                pThis->ssCurAnimNIdx = -1;
                            }
                            int16_t ssNextIdx = pThis->ssCurAnimNIdx + 1;
                            for (int32_t i = ssNextIdx; ssNextIdx < static_cast<int32_t>(pThis->usNumNotes);
                                 i = ++ssNextIdx) {
                                NiPointer<BSAnimNote>* v18 = &pThis->spAnimNotesA[i];
                                if (!(fUpdateTime_3 > static_cast<double>(v18->m_pObject->fTime))) {
                                    break;
                                }
                                NiControllerManager* m_pkOwner = pThis->m_pkOwner;
                                pThis->ssCurAnimNIdx = ssNextIdx;
                                m_pkOwner->pListener->Update(v18->m_pObject);
                            }
                        }
                    }
                }
                break;
            }
            default: {
                goto LABEL_34;
            }
        }
    }
    else {
        pThis->ssCurAnimNIdx = -1;
    }
}

// Binds each interpolator that resolves its dependencies and has no blend interpolator yet: the target is the named
// object, or its property of the tag's type; an existing controller of the tag's type and ID on it is used, else the
// item's own controller is retargeted (and moved behind the owner when the target is the owner's). Missing targets
// named "##..." mark the sequence as having removable objects. The palette is read without the owner's accessor.
bool NiControllerSequence::StoreTargets(NiAVObject* pkRoot) {
    NiAVObjectPalette* pkObjectPalette = m_pkOwner->m_spObjectPalette.m_pObject;
    if (m_kAccumRootName.GetHandle() != NiGlobalStringTable::NULL_STRING) {
        m_pkAccumRoot = pkObjectPalette->GetAVObject(m_kAccumRootName);
    }

    bool bNeedSelectiveUpdate = ResolveTransformInterpolators(pkRoot, pkObjectPalette, 0);
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        NiInterpolator* pkInterpolator = m_pkInterpArray[ui].m_spInterpolator.m_pObject;
        if (!pkInterpolator) {
            continue;
        }
        IDTag& kIDTag = m_pkIDTagArray[ui];
        if (!pkInterpolator->ResolveDependencies(pkObjectPalette)) {
            continue;
        }
        if (m_pkInterpArray[ui].m_pkBlendInterp) {
            continue;
        }

        NiAVObject* pkAVObject = pkObjectPalette->GetAVObject(kIDTag.m_kAVObjectName);
        NiObjectNET* pkTarget = pkAVObject;
        if (pkAVObject && kIDTag.m_kPropertyType.Exists()) {
            NiTListItem<NiPointer<NiProperty>>* kPos = pkAVObject->m_kPropertyList.GetHeadPos();
            while (kPos) {
                NiProperty* pkProperty = pkAVObject->m_kPropertyList.GetNext(kPos);
                if (strcmp(pkProperty->GetRTTI()->GetName(), kIDTag.m_kPropertyType) == 0) {
                    pkTarget = pkProperty;
                    break;
                }
            }
        }
        if (!pkTarget) {
            const char* pcName = kIDTag.m_kAVObjectName;
            if (pcName[0] == '#' && pcName[1] == '#') {
                bRemovableObjects = true;
            }
            continue;
        }

        NiInterpController* pkCtlr = nullptr;
        for (NiTimeController* pkTimeCtlr = pkTarget->m_spControllers; pkTimeCtlr;
             pkTimeCtlr = pkTimeCtlr->m_spNext.m_pObject) {
            if (strcmp(pkTimeCtlr->GetRTTI()->GetName(), kIDTag.m_kCtlrType) != 0) {
                continue;
            }
            NiInterpController* pkInterpCtlr = static_cast<NiInterpController*>(pkTimeCtlr);
            const char* pcCtlrID = pkInterpCtlr->GetCtlrID();
            bool bMatch;
            if (!pcCtlrID) {
                bMatch = !kIDTag.m_kCtlrID.Exists();
            }
            else {
                bMatch = kIDTag.m_kCtlrID.Exists() && strcmp(pcCtlrID, kIDTag.m_kCtlrID) == 0;
            }
            if (bMatch) {
                pkCtlr = pkInterpCtlr;
                break;
            }
        }

        if (!pkCtlr) {
            pkCtlr = m_pkInterpArray[ui].m_spInterpCtlr.m_pObject;
            if (!pkCtlr) {
                continue;
            }
            pkCtlr->SetTarget(pkTarget);
            bNeedSelectiveUpdate = true;
            if (pkTarget == m_pkOwner->m_pkTarget) {
                pkTarget->RemoveController(pkCtlr);
                pkCtlr->SetNext(m_pkOwner->m_spNext.m_pObject);
                m_pkOwner->SetNext(pkCtlr);
            }
        }

        uint16_t usIndex = pkCtlr->GetInterpolatorIndex(kIDTag.m_kInterpolatorID);
        if (usIndex == NiInterpController::INVALID_INDEX) {
            continue;
        }
        NiInterpolator* pkInterp = pkCtlr->GetInterpolator(usIndex);
        NiBlendInterpolator* pkBlendInterp;
        pkBlendInterp = static_cast<NiBlendInterpolator*>(
            pkInterp ? pkInterp->DynamicCast(&NiBlendInterpolator::ms_RTTI) : nullptr
        );
        bool bCumulative = false;
        if (m_pkAccumRoot == pkTarget && m_pkOwner->m_bCumulative) {
            bCumulative = true;
        }
        if (!pkBlendInterp) {
            pkBlendInterp = pkCtlr->CreateBlendInterpolator(usIndex, true, bCumulative, 0.0f, 2);
            pkCtlr->SetInterpolator(pkBlendInterp, usIndex);
            if (!pkBlendInterp) {
                continue;
            }
        }
        pkCtlr->m_usFlags |= NiTimeController::MANAGERCONTROLLED_MASK;
        m_pkInterpArray[ui].m_spInterpCtlr = pkCtlr;
        m_pkInterpArray[ui].m_pkBlendInterp = pkBlendInterp;
    }

    if (bNeedSelectiveUpdate) {
        // Both flags are outputs of the call; the sequence does not use them.
        bool bSelectiveUpdate;
        bool bRigid;
        static_cast<NiAVObject*>(m_pkOwner->m_pkTarget)->SetSelectiveUpdateFlags(bSelectiveUpdate, true, bRigid);
    }
    return true;
}

// Gamebryo's form, extended with the tread controllers: the tagged NiTransformController interpolators go to the root's
// NiMultiTargetTransformController and the BSTreadTransfController ones to its BSMultiTargetTreadTransfController, each
// made when missing. A controller type is tested for "NiTransformController" by its third and eighth characters only,
// as the game does, and a target's own transform controller is kept when the target is named Bip01.
// aiTransformControllerCount is never read.
bool NiControllerSequence::ResolveTransformInterpolators(
    NiAVObject* pkRoot,
    NiAVObjectPalette* pkObjectPalette,
    uint32_t aiTransformControllerCount
) {
    bool bAddedController = false;
    static NiFixedString strBSTreadTransfController("BSTreadTransfController");
    NiPointer<NiMultiTargetTransformController> spMTCtlr = static_cast<NiMultiTargetTransformController*>(
        pkRoot->GetController(&NiMultiTargetTransformController::ms_RTTI)
    );
    NiPointer<BSMultiTargetTreadTransfController> spTreadCtlr = static_cast<BSMultiTargetTreadTransfController*>(
        pkRoot->GetController(&BSMultiTargetTreadTransfController::ms_RTTI)
    );

    uint16_t usTreadInterpCount = 0;
    uint16_t usLinkCount = 0;
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        IDTag* pkID = &m_pkIDTagArray[ui];
        if ((pkID->m_kCtlrType.GetHandle() != NiGlobalStringTable::NULL_STRING) &&
            (strBSTreadTransfController.GetHandle() == pkID->m_kCtlrType.GetHandle())) {
            BSTreadTransfInterpolator* pkTreadInterp =
                static_cast<BSTreadTransfInterpolator*>(m_pkInterpArray[ui].m_spInterpolator.m_pObject);
            usLinkCount += static_cast<uint16_t>(pkTreadInterp->uiNumLinks);
            ++usTreadInterpCount;
        }
    }

    if (!spMTCtlr || (!spTreadCtlr && usTreadInterpCount)) {
        uint16_t usTransformInterpCount = 0;
        NiDefaultAVObjectPalette* pkDefaultPalette = static_cast<NiDefaultAVObjectPalette*>(
            NiObject::DynamicCast(&NiDefaultAVObjectPalette::ms_RTTI, pkObjectPalette)
        );
        if (pkDefaultPalette) {
            usTransformInterpCount =
                static_cast<uint16_t>(pkDefaultPalette->m_kHash.m_kAllocator.m_uiCount - usLinkCount);
        }
        else {
            for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
                const char* pcCtlrType = m_pkIDTagArray[ui].m_kCtlrType.GetHandle();
                if (pcCtlrType != NiGlobalStringTable::NULL_STRING && pcCtlrType && pcCtlrType[2] == 'T' &&
                    pcCtlrType[7] == 'f') {
                    ++usTransformInterpCount;
                }
            }
        }

        if (!usTransformInterpCount) {
            if (!usTreadInterpCount) {
                return false;
            }
        }
        else if (!spMTCtlr) {
            NiMultiTargetTransformController* pkNewCtlr = static_cast<NiMultiTargetTransformController*>(
                NiMemObject::operator new(sizeof(NiMultiTargetTransformController))
            );
            if (pkNewCtlr) {
                pkNewCtlr = ::new (pkNewCtlr) NiMultiTargetTransformController(usTransformInterpCount);
                spMTCtlr = pkNewCtlr;
            }
            spMTCtlr->SetTarget(pkRoot);
            spMTCtlr->m_usFlags |= NiTimeController::MANAGERCONTROLLED_MASK;
            bAddedController = true;
            if (pkRoot == m_pkOwner->m_pkTarget) {
                pkRoot->RemoveController(spMTCtlr);
                spMTCtlr->m_spNext = m_pkOwner->m_spNext.m_pObject;
                m_pkOwner->m_spNext = spMTCtlr.m_pObject;
            }
        }

        if (usTreadInterpCount && !spTreadCtlr) {
            BSMultiTargetTreadTransfController* pkNewCtlr = static_cast<BSMultiTargetTreadTransfController*>(
                NiMemObject::operator new(sizeof(BSMultiTargetTreadTransfController))
            );
            if (pkNewCtlr) {
                pkNewCtlr = ::new (pkNewCtlr) BSMultiTargetTreadTransfController(usTreadInterpCount, usLinkCount);
                if (pkNewCtlr) {
                    spTreadCtlr = pkNewCtlr;
                }
            }
            spTreadCtlr->SetTarget(pkRoot);
            spTreadCtlr->m_usFlags |= NiTimeController::MANAGERCONTROLLED_MASK;
            bAddedController = true;
            if (pkRoot == m_pkOwner->m_pkTarget) {
                pkRoot->RemoveController(spTreadCtlr);
                spTreadCtlr->m_spNext = m_pkOwner->m_spNext.m_pObject;
                m_pkOwner->m_spNext = spTreadCtlr.m_pObject;
            }
        }
    }

    uint16_t usTreadIndex = 0;
    static NiFixedString b01("Bip01");
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        InterpArrayItem* pkItem = &m_pkInterpArray[ui];
        IDTag* pkID = &m_pkIDTagArray[ui];
        const char* pcCtlrType = pkID->m_kCtlrType.GetHandle();
        if (pcCtlrType == NiGlobalStringTable::NULL_STRING) {
            continue;
        }
        if (pcCtlrType && pcCtlrType[2] == 'T' && pcCtlrType[7] == 'f') {
            NiAVObject* pkAVObj = pkObjectPalette->GetAVObject(pkID->m_kAVObjectName);
            if (!pkAVObj || (m_pkAccumRoot == pkAVObj && m_pkOwner->m_bCumulative)) {
                continue;
            }
            NiTransformController* pkTransCtlr =
                static_cast<NiTransformController*>(pkAVObj->GetController(&NiTransformController::ms_RTTI));

            // EXE-INLINED 0x43B3B0
            uint16_t usIndex = NiInterpController::INVALID_INDEX;
            for (uint16_t us = 0; us < spMTCtlr->m_usNumInterps; ++us) {
                if (spMTCtlr->m_ppkTargets[us] == pkAVObj) {
                    usIndex = us;
                    break;
                }
            }
            if (usIndex == NiInterpController::INVALID_INDEX) {
                // EXE-INLINED none: NiMultiTargetTransformController::AddInterpolatorTarget
                for (uint16_t us = 0; us < spMTCtlr->m_usNumInterps; ++us) {
                    if (!spMTCtlr->m_ppkTargets[us]) {
                        spMTCtlr->m_ppkTargets[us] = pkAVObj;
                        pkAVObj->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE;
                        usIndex = us;
                        break;
                    }
                }
                if (usIndex == NiInterpController::INVALID_INDEX) {
                    continue;
                }
            }
            else {
                // EXE-INLINED 0x43B320
                spMTCtlr->m_ppkTargets[usIndex] = pkAVObj;
                pkAVObj->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE;
            }

            if (pkAVObj->m_kName.GetHandle() != b01.GetHandle()) {
                pkAVObj->RemoveController(pkTransCtlr);
            }
            pkItem->m_spInterpCtlr = spMTCtlr.m_pObject;
            // EXE-INLINED 0x4B5B00
            pkItem->m_pkBlendInterp = &spMTCtlr->m_pkBlendInterps[usIndex];
            pkItem->m_pkBlendInterp->m_uFlags |= NiBlendInterpolator::MANAGER_CONTROLLED_MASK;
        }
        else {
            if (pcCtlrType == NiGlobalStringTable::NULL_STRING) {
                continue;
            }
            if (strBSTreadTransfController.GetHandle() != pkID->m_kCtlrType.GetHandle()) {
                continue;
            }
            BSTreadTransfInterpolator* pkTreadInterp =
                static_cast<BSTreadTransfInterpolator*>(pkItem->m_spInterpolator.m_pObject);
            for (uint32_t uiLink = 0; uiLink < pkTreadInterp->uiNumLinks; ++uiLink) {
                BSTreadTransfInterpolator::LinkInfo* pkLink = &pkTreadInterp->Links[uiLink];
                NiAVObject* pkLinkObj = pkObjectPalette->GetAVObject(pkLink->Name);
                uint16_t usLinkIndex = spTreadCtlr->GetInterpolatorTargetIndex(pkLinkObj);
                if (usLinkIndex == NiInterpController::INVALID_INDEX) {
                    spTreadCtlr->AddInterpolatorTarget(pkLinkObj, usTreadIndex, pkLink);
                }
                else {
                    spTreadCtlr->SetInterpolatorTarget(usLinkIndex, pkLinkObj, usTreadIndex, pkLink);
                }
            }
            pkItem->m_spInterpCtlr = spTreadCtlr.m_pObject;
            pkItem->m_pkBlendInterp = static_cast<NiBlendInterpolator*>(
                spTreadCtlr->BSMultiTargetTreadTransfController::GetInterpolator(usTreadIndex)
            );
            pkItem->m_pkBlendInterp->m_uFlags |= NiBlendInterpolator::MANAGER_CONTROLLED_MASK;
            ++usTreadIndex;
        }
    }
    return bAddedController;
}

float NiControllerSequence::GetOffset() {
    return m_fOffset;
}

float NiControllerSequence::GetEndKeyTime() {
    return m_fEndKeyTime;
}

float NiControllerSequence::GetBeginKeyTime() const {
    return m_fBeginKeyTime;
}

float NiControllerSequence::GetSequenceWeight() const {
    return m_fSeqWeight;
}

void NiControllerSequence::SetSequenceWeight(float fWeight) {
    m_fSeqWeight = fWeight;
    if (m_fSeqWeight < 0.0) {
        m_fSeqWeight = (float)0.0;
    }
}

void NiControllerSequence::ResetSequence() {
    m_fOffset = (-0x1.fffffe0000000p+127f);
}

bool NiControllerSequence::SetCycleType(NiTimeController::CycleType eCycleType) {
    if ((((int32_t)eCycleType) != 2) && eCycleType) {
        return false;
    }
    m_eCycleType = static_cast<NiControllerSequence::CycleType>((int32_t)eCycleType);
    return true;
}

NiControllerSequence::IDTag* NiControllerSequence::GetIDTag(uint32_t uiTagIndex) {
    return &m_pkIDTagArray[uiTagIndex];
}

// The source's morph text keys around its scaled time (the previous and next, wrapping around its key range), then
// the frame at the same fraction between this sequence's keys of the same names, wrapped into this key range.
float NiControllerSequence::FindCorrespondingMorphFrame(
    NiControllerSequence* pkSourceSequence,
    float fSourceSequenceTime
) {
    float fScaledSourceTime = pkSourceSequence->ComputeScaledTime(fSourceSequenceTime, false);
    NiTextKeyExtraData* pkTextKeys = pkSourceSequence->m_spTextKeys.m_pObject;
    uint32_t uiNumKeys = pkTextKeys->m_uiNumKeys;
    NiTextKey* pkKeys = pkTextKeys->m_pKeys;
    NiTextKey* pkNextKey = nullptr;
    NiTextKey* pkPrevKey = nullptr;
    NiTextKey* pkLastKey = nullptr;
    NiTextKey* pkFirstKey = nullptr;
    for (uint32_t ui = 0; ui < uiNumKeys; ++ui) {
        NiTextKey* pkKey = &pkKeys[ui];
        const char* pcKeyText = pkKey->m_kText.GetHandle();
        if (pcKeyText && !NiStrnicmp(pcKeyText, pMorphTag, iMorphTagLen)) {
            pkLastKey = pkKey;
            if (!pkFirstKey) {
                pkFirstKey = pkKey;
            }
            if (fScaledSourceTime > pkKey->GetTime()) {
                pkPrevKey = pkKey;
            }
            else if (!pkNextKey) {
                pkNextKey = pkKey;
            }
        }
    }

    bool bCreateNext = false;
    float fSourceRatio;
    if (pkNextKey) {
        float fStartTime;
        if (pkPrevKey) {
            fStartTime = pkPrevKey->GetTime();
        }
        else {
            pkPrevKey = pkLastKey;
            fStartTime = pkSourceSequence->m_fBeginKeyTime - pkSourceSequence->m_fEndKeyTime + pkLastKey->GetTime();
        }
        fSourceRatio = (fScaledSourceTime - fStartTime) / (pkNextKey->GetTime() - fStartTime);
    }
    else {
        float fStartTime = pkPrevKey->GetTime();
        pkNextKey = pkFirstKey;
        bCreateNext = true;
        float fEndTime = pkSourceSequence->m_fEndKeyTime - pkSourceSequence->m_fBeginKeyTime + pkFirstKey->GetTime();
        fSourceRatio = (fScaledSourceTime - fStartTime) / (fEndTime - fStartTime);
    }

    NiTextKeyExtraData* pkTargetTextKeys = m_spTextKeys.m_pObject;
    uint32_t uiTargetNumKeys = pkTargetTextKeys->m_uiNumKeys;
    NiTextKey* pkTargetKeys = pkTargetTextKeys->m_pKeys;
    // A key not found in the source counts as found: it is not looked for.
    bool bPrevFound = pkPrevKey == nullptr;
    bool bNextFound = pkNextKey == nullptr;
    NiTextKey* pkTargetPrevKey = nullptr;
    NiTextKey* pkTargetNextKey = nullptr;
    for (uint32_t ui = 0; ui < uiTargetNumKeys; ++ui) {
        if (bPrevFound && bNextFound) {
            break;
        }
        NiTextKey* pkKey = &pkTargetKeys[ui];
        if (!bPrevFound && !NiStricmp(pkKey->m_kText.GetHandle(), pkPrevKey->m_kText.GetHandle())) {
            pkTargetPrevKey = pkKey;
            bPrevFound = true;
        }
        if (!bNextFound && !NiStricmp(pkKey->m_kText.GetHandle(), pkNextKey->m_kText.GetHandle())) {
            pkTargetNextKey = pkKey;
            bNextFound = true;
        }
    }

    float fTargetLo = 0.0f;
    float fTargetHi = 0.0f;
    if (pkTargetPrevKey == pkTargetNextKey) {
        fTargetLo = pkTargetPrevKey->GetTime();
        fTargetHi = pkTargetNextKey->GetTime();
        if (bCreateNext) {
            fTargetHi = m_fEndKeyTime - m_fBeginKeyTime + fTargetHi;
        }
        else {
            fTargetLo = fTargetLo - (m_fEndKeyTime - m_fBeginKeyTime);
        }
    }
    else {
        if (pkTargetNextKey) {
            fTargetHi = pkTargetNextKey->GetTime();
        }
        if (pkTargetPrevKey) {
            fTargetLo = pkTargetPrevKey->GetTime();
        }
        if (fTargetHi < fTargetLo) {
            fTargetHi = fTargetHi + (m_fEndKeyTime - m_fBeginKeyTime);
        }
    }

    float fTargetFrame = fTargetLo + (fTargetHi - fTargetLo) * fSourceRatio;
    if (m_fBeginKeyTime > fTargetFrame) {
        return fTargetFrame + (m_fEndKeyTime - m_fBeginKeyTime);
    }
    if (m_fEndKeyTime < fTargetFrame) {
        return fTargetFrame - (m_fEndKeyTime - m_fBeginKeyTime);
    }
    return fTargetFrame;
}

void NiControllerSequence::RemoveInterpolator_0(uint32_t uiIndex) {
    NiPointer<NiInterpolator> spinterpolator;
    RemoveInterpolator(uiIndex, spinterpolator);
}

char* NiControllerSequence::GetViewerString(const char* apPrefix, State aeState) {
    char* pcString = static_cast<char*>(MemoryManager_Singleton.Allocate(strlen(apPrefix) + 32));
    const char* pcState = nullptr;
    switch (aeState) {
        case kInactive: pcState = "INACTIVE"; break;
        case kAnimating: pcState = "ANIMATING"; break;
        case kEaseIn: pcState = "EASEIN"; break;
        case kEaseOut: pcState = "EASEOUT"; break;
        case kTransSource: pcState = "TRANSSOURCE"; break;
        case kTransDest: pcState = "TRANSDEST"; break;
        case kMorphSource: pcState = "MORPHSOURCE"; break;
        default: break;
    }

    std::sprintf(pcString, "%s = %s", apPrefix, pcState);
    return pcString;
}

// Out of range, the result is released and nulled. Otherwise a file before 5.0.0.0 has its NiSequence converted and a
// newer one's object is cast to a controller sequence; the result is assigned and the local reference dropped.
void NiControllerSequence::CreateSequenceFromFile(
    const NiStream& kStream,
    uint32_t uiIndex,
    NiPointer<NiControllerSequence>& aspControllerSequence
) {
    if (uiIndex >= kStream.m_kTopObjects.m_uiSize) {
        aspControllerSequence = nullptr;
        return;
    }

    alignas(NiPointer<NiControllerSequence>) uint8_t aucSequence[sizeof(NiPointer<NiControllerSequence>)];
    NiPointer<NiControllerSequence>* pspSequence =
        static_cast<NiPointer<NiControllerSequence>*>(static_cast<void*>(aucSequence));
    pspSequence->m_pObject = nullptr;
    if (kStream.m_uiNifFileVersion < 0x05000000) {
        alignas(NiPointer<NiSequence>) uint8_t aucOldSequence[sizeof(NiPointer<NiSequence>)];
        NiPointer<NiSequence>* pspOldSequence = static_cast<NiPointer<NiSequence>*>(static_cast<void*>(aucOldSequence));
        pspOldSequence->m_pObject = nullptr;
        NiSequence::CreateFromKFFile(kStream, uiIndex, *pspOldSequence, nullptr);
        ConvertOldSequence(pspOldSequence->m_pObject, *pspSequence);
        NiSequence* pkOldSequence = pspOldSequence->m_pObject;
        if (pkOldSequence) {
            pkOldSequence->DecRefCount();
        }
    }
    else {
        NiObject* pkObject = kStream.m_kTopObjects.m_pBase[uiIndex].m_pObject;
        NiObject* pkCast = pkObject ? pkObject->DynamicCastMember(&ms_RTTI) : nullptr;
        *pspSequence = static_cast<NiControllerSequence*>(pkCast);
    }

    NiControllerSequence* pkSequence = pspSequence->m_pObject;
    aspControllerSequence = pkSequence;
    if (pkSequence) {
        pkSequence->DecRefCount();
    }
}

// Builds a controller sequence from a pre-5.0 NiSequence into aspControllerSequence: its name and size, then each named
// controller, the first one also giving the cycle type (loop unless clamp), frequency and key range, then its text keys
// and an empty accum root name.
void NiControllerSequence::ConvertOldSequence(
    NiSequence* pkOldSequence,
    NiPointer<NiControllerSequence>& aspControllerSequence
) {
    NiPointer<NiControllerSequence> spNewSequence;
    void* pvMemory = NiMemObject::operator new(sizeof(NiControllerSequence));
    if (pvMemory) {
        uint32_t uiCount = pkOldSequence->m_kObjectName.m_usSize;
        const char* pcSequenceName = pkOldSequence->m_pcName;
        spNewSequence = ::new (pvMemory) NiControllerSequence(pcSequenceName, uiCount, uiCount);
    }

    bool bFirstTime = true;
    for (uint32_t ui = 0; ui < pkOldSequence->m_kObjectName.m_usSize; ui++) {
        const char* pcName = pkOldSequence->m_kObjectName.m_pBase[ui];
        NiTimeController* pkController = pkOldSequence->m_kCtrl.m_pBase[ui].m_pObject;
        if (pcName && pkController) {
            if (bFirstTime) {
                // The controller's cycle type (NiTimeController::GetCycleType: flag bits 1-2); a reverse one
                // becomes a loop.
                NiTimeController::CycleType eCycleType =
                    static_cast<NiTimeController::CycleType>((pkController->m_usFlags >> 1) & 3);
                if (eCycleType != NiTimeController::CLAMP && eCycleType != NiTimeController::LOOP) {
                    eCycleType = NiTimeController::LOOP;
                }
                // EXE-INLINED 0x7408D0
                spNewSequence->m_eCycleType = static_cast<NiControllerSequence::CycleType>(eCycleType);
                spNewSequence->m_fFrequency = pkController->m_fFrequency;
                spNewSequence->m_fBeginKeyTime = pkController->m_fLoKeyTime;
                spNewSequence->m_fEndKeyTime = pkController->m_fHiKeyTime;
                bFirstTime = false;
            }
            spNewSequence->AddNameControllerPair(pcName, pkController);
        }
    }

    // SetTextKeys(pkOldSequence->GetTextKeys()). m_spTextKeys is held as NiPointer<NiRefObject> for its stream users
    // rather than as a NiTextKeyExtraDataPtr.
    spNewSequence->m_spTextKeys = static_cast<NiTextKeyExtraData*>(pkOldSequence->m_spTextKeys.m_pObject);

    // EXE-INLINED 0x4EEAE0: SetAccumRootName(EmptyFS)
    const char* pcEmpty = EmptyFS.GetHandle();
    if (spNewSequence->m_kAccumRootName.GetHandle() != pcEmpty) {
        NiGlobalStringTable::AcquireHandle(pcEmpty);
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(spNewSequence->m_kAccumRootName.GetHandle());
        spNewSequence->m_kAccumRootName.SetHandle(pcEmpty);
    }

    aspControllerSequence = spNewSequence;
}

NiObject* NiControllerSequence::CreateObject() {
    void* pvMemory = NiMemObject::operator new(sizeof(NiControllerSequence));
    NiControllerSequence* pSequence = static_cast<NiControllerSequence*>(pvMemory);
    if (!pSequence) {
        return nullptr;
    }
    return ::new (pSequence) NiControllerSequence;
}
