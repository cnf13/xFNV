#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertWeightsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI NiObjectNET::ms_RTTI("NiObjectNET", &NiObject::ms_RTTI);
NiCriticalSection NiObjectNET::ExtraDataCrit;

NiObjectNET::NiObjectNET() : m_ppkExtra(nullptr), m_usExtraDataSize(0), m_usMaxSize(0) {}

NiObjectNET::~NiObjectNET() {
    RemoveAllExtraData();
}

void NiObjectNET::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion < 0x14010001) {
        kStream.LoadCStringAsFixedString(m_kName);
    }
    else {
        kStream.LoadFixedString(m_kName);
    }
    if (kStream.m_uiNifFileVersion < 0x0500000B) {
        kStream.ReadLinkID();
    }
    else {
        kStream.ReadMultipleLinkIDs();
    }
    kStream.ReadLinkID();
}

// Files before 5.0.0.11 hold a single extra-data link, the head of a legacy chain that PostLinkObject unlinks; newer
// ones a counted list.
void NiObjectNET::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    if (kStream.m_uiNifFileVersion < 0x0500000B) {
        m_pkLegacyExtra = static_cast<NiExtraData*>(kStream.GetObjectFromLinkID());
    }
    else {
        uint32_t uiSize = kStream.GetNumberOfLinkIDs();
        SetExtraDataSize(uiSize);
        for (uint32_t i = uiSize; i != 0; --i) {
            AddExtraData(static_cast<NiExtraData*>(kStream.GetObjectFromLinkID()));
        }
    }
    m_spControllers = static_cast<NiTimeController*>(kStream.GetObjectFromLinkID());
}

// Registers the object and its name, then each streamable extra datum and the controller chain.
bool NiObjectNET::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }
    if (!kStream.RegisterFixedString(m_kName)) {
        return false;
    }
    for (uint16_t usIndex = 0; usIndex < m_usExtraDataSize; ++usIndex) {
        NiExtraData* pkExtra = m_ppkExtra[usIndex];
        if (pkExtra && pkExtra->IsStreamable()) {
            pkExtra->RegisterStreamables(kStream);
        }
    }
    if (m_spControllers) {
        m_spControllers->RegisterStreamables(kStream);
    }
    return true;
}

// Saves the object part (the shared empty SaveBinary), the name, the count and link IDs of the streamable extra data,
// then the link ID of the first streamable controller. The controller walk reads m_spNext itself.
void NiObjectNET::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    kStream.SaveFixedString(m_kName);
    uint32_t uiExtraDataSize = 0;
    for (uint16_t i = 0; i < m_usExtraDataSize; ++i) {
        NiExtraData* pkExtra = m_ppkExtra[i];
        if (pkExtra->IsStreamable()) {
            ++uiExtraDataSize;
        }
    }
    NiStreamSaveBinary(kStream, uiExtraDataSize);
    for (uint16_t j = 0; j < m_usExtraDataSize; ++j) {
        NiExtraData* pkExtra = m_ppkExtra[j];
        if (pkExtra->IsStreamable()) {
            kStream.SaveLinkID(pkExtra);
        }
    }
    NiTimeController* pkController = m_spControllers;
    while (pkController && !pkController->IsStreamable()) {
        pkController = pkController->m_spNext.m_pObject;
    }
    kStream.SaveLinkID(pkController);
}

// Equal when the base comparison holds, the names are one handle, the streamable extra data compare equal pairwise and
// the controller chains are as long and compare equal pairwise.
bool NiObjectNET::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }
    NiObjectNET* pkNET = static_cast<NiObjectNET*>(pkObject);
    if (m_kName.GetHandle() != pkNET->m_kName.GetHandle()) {
        return false;
    }

    // The indices of the streamable extra data of this object and of the other one (no slots, a grow-by of one).
    NiTPrimitiveArray<uint16_t> kSelfEntries(0u, 1u);
    NiTPrimitiveArray<uint16_t> kOtherEntries(0u, 1u);

    uint16_t i;
    for (i = 0; i < m_usExtraDataSize; ++i) {
        if (m_ppkExtra[i]->IsStreamable()) {
            kSelfEntries.Add(i);
        }
    }
    for (i = 0; i < pkNET->m_usExtraDataSize; ++i) {
        if (pkNET->m_ppkExtra[i]->IsStreamable()) {
            kOtherEntries.Add(i);
        }
    }

    if (kSelfEntries.m_usSize != kOtherEntries.m_usSize) {
        return false;
    }
    for (i = 0; i < kSelfEntries.m_usSize; ++i) {
        NiExtraData* pkSelfExtra = m_ppkExtra[kSelfEntries.m_pBase[i]];
        if (!pkSelfExtra->IsEqual(pkNET->m_ppkExtra[kOtherEntries.m_pBase[i]])) {
            return false;
        }
    }

    NiTimeController* pkControl0 = m_spControllers;
    NiTimeController* pkControl1 = pkNET->m_spControllers;
    int32_t uiControlCount0 = pkControl0 ? pkControl0->GetLast() : 0;
    int32_t uiControlCount1 = pkControl1 ? pkControl1->GetLast() : 0;
    if (uiControlCount0 != uiControlCount1) {
        return false;
    }
    while (pkControl0 && pkControl1) {
        if (!pkControl0->IsEqual(pkControl1)) {
            return false;
        }
        pkControl0 = pkControl0->m_spNext.m_pObject;
        pkControl1 = pkControl1->m_spNext.m_pObject;
    }
    return true;
}

void NiObjectNET::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiObjectNET::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v10 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v10);
    char* ViewerString_9 = NiGetViewerString("m_kName", m_kName.GetHandle());
    uint32_t v8 = pkStrings->m_usSize;
    uint32_t v9 = pkStrings->m_usMaxSize;
    v10 = ViewerString_9;
    if (v8 >= v9) {
        pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v8, v10);
}

void NiObjectNET::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);
    for (uint16_t i = 0; i < m_usExtraDataSize; ++i) {
        NiExtraData* pkExtra = m_ppkExtra[i];
        if (pkExtra) {
            pkExtra->ProcessClone(kCloning);
        }
    }
    NiTimeController* pkController = m_spControllers;
    if (pkController) {
        pkController->ProcessClone(kCloning);
    }
}

// A legacy chain left by LinkObject is unlinked first: each datum's name field holds the next one, so the name is
// cleared before the datum is added. Every NiVertWeightsExtraData is then removed, the scan restarting after each.
void NiObjectNET::PostLinkObject(NiStream& kStream) {
    // NiObject::PostLinkObject is empty.
    NiObject::PostLinkObject(kStream);
    if (kStream.m_uiNifFileVersion < 0x0500000B) {
        NiExtraData* pkExtra = m_pkLegacyExtra;
        if (pkExtra) {
            m_ppkExtra = nullptr;
            do {
                // The name's handle is read as the next datum's address (raw memory, not a string here).
                NiExtraData* pkNext = reinterpret_cast<NiExtraData*>(const_cast<char*>(pkExtra->GetName().GetHandle()));
                pkExtra->ClearName();
                AddExtraData(pkExtra);
                pkExtra = pkNext;
            }
            while (pkExtra);
        }
    }
    for (int32_t i = 0; static_cast<uint32_t>(i) < m_usExtraDataSize; ++i) {
        NiExtraData* pkExtraData = m_ppkExtra[static_cast<uint16_t>(i)];
        if (pkExtraData && pkExtraData->GetRTTI() == &NiVertWeightsExtraData::ms_RTTI) {
            RemoveExtraDataAt(i);
            i = -1;
        }
    }
}

// Only the low 16 bits of the index are used.
void NiObjectNET::DeleteExtraData(int32_t usExtraDataIndex) {
    ExtraDataCrit.Lock();
    uint16_t usIndex = static_cast<uint16_t>(usExtraDataIndex);
    if (usIndex < m_usExtraDataSize) {
        NiExtraData* pkExtra = m_ppkExtra[usIndex];
        if (pkExtra) {
            pkExtra->DecRefCount();
        }
        for (uint16_t i = usIndex; static_cast<int32_t>(i) < m_usExtraDataSize - 1; ++i) {
            m_ppkExtra[i] = m_ppkExtra[i + 1];
        }
        NiExtraData** ppkExtra = m_ppkExtra;
        --m_usExtraDataSize;
        ppkExtra[m_usExtraDataSize] = nullptr;
    }
    ExtraDataCrit.Unlock();
}

// Compares the low 16 bits of the index but passes the whole argument on to DeleteExtraData.
bool NiObjectNET::RemoveExtraDataAt(int32_t usExtraDataIndex) {
    ExtraDataCrit.Lock();
    if (static_cast<uint16_t>(usExtraDataIndex) >= m_usExtraDataSize) {
        ExtraDataCrit.Unlock();
        return false;
    }
    DeleteExtraData(usExtraDataIndex);
    ExtraDataCrit.Unlock();
    return true;
}

// Only the low word of the size is read.
bool NiObjectNET::SetExtraDataSize(uint16_t usSize) {
    uint16_t usNewSize = usSize;
    if (!usNewSize) {
        return false;
    }
    ExtraDataCrit.Lock();
    bool bEmpty = m_usMaxSize == 0;
    m_usMaxSize = usNewSize;
    if (bEmpty) {
        m_ppkExtra = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*) * usNewSize));
        m_usExtraDataSize = 0;
    }
    else {
        NiExtraData** ppkExtra = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*) * usNewSize));
        rt::memcpy(ppkExtra, m_ppkExtra, sizeof(NiExtraData*) * m_usExtraDataSize);
        _NiFree(m_ppkExtra);
        m_ppkExtra = ppkExtra;
    }
    ExtraDataCrit.Unlock();
    return true;
}

void NiObjectNET::RemoveAllExtraData() {
    ExtraDataCrit.Lock();
    for (int16_t i = static_cast<int16_t>(m_usExtraDataSize - 1); i >= 0; --i) {
        DeleteExtraData(static_cast<uint16_t>(i));
    }
    _NiFree(m_ppkExtra);
    m_ppkExtra = nullptr;
    m_usMaxSize = 0;
    m_usExtraDataSize = 0;
    ExtraDataCrit.Unlock();
}

bool NiObjectNET::InsertExtraData(NiExtraData* pkExtra) {
    if (!pkExtra) {
        return false;
    }
    NiIncRefCount(pkExtra);
    EnterCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
    uint16_t m_usMaxSize = this->m_usMaxSize;
    if (m_usMaxSize) {
        int32_t i_2;
        if (m_usExtraDataSize == m_usMaxSize) {
            uint16_t v6 = 2 * m_usMaxSize + 1;
            this->m_usMaxSize = v6;
            NiExtraData** ppkNewExtra_2 = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*) * v6));
            rt::memcpy(ppkNewExtra_2, this->m_ppkExtra, sizeof(NiExtraData*) * m_usExtraDataSize);
            _NiFree(this->m_ppkExtra);
            this->m_ppkExtra = ppkNewExtra_2;
        }
        this->m_ppkExtra[m_usExtraDataSize++] = pkExtra;
        for (int32_t i = m_usExtraDataSize; static_cast<uint16_t>(i) < this->m_usMaxSize;
             this->m_ppkExtra[i_2] = nullptr) {
            i_2 = static_cast<uint16_t>(i++);
        }
        uint16_t i_3 = m_usExtraDataSize - 1;
        if (m_usExtraDataSize != 1) {
            while (true) {
                int32_t i_4 = i_3;
                const char* m_kHandle = this->m_ppkExtra[i_4 - 1]->GetName().GetHandle();
                int32_t v13 = m_kHandle - this->m_ppkExtra[i_4]->GetName().GetHandle();
                if (!v13) {
                    DeleteExtraData(i_3);
                    LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
                    return false;
                }
                if (v13 > 0) {
                    NiExtraData** m_ppkExtra = this->m_ppkExtra;
                    NiExtraData* pkTmpED = m_ppkExtra[i_4 - 1];
                    m_ppkExtra[i_3 - 1] = m_ppkExtra[i_3];
                    --i_3;
                    this->m_ppkExtra[i_4] = pkTmpED;
                    if (i_3) {
                        continue;
                    }
                }
                break;
            }
        }
        LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
        return true;
    }

    this->m_usMaxSize = 1;
    m_usExtraDataSize = 1;
    NiExtraData** ppkNewExtra = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*)));
    this->m_ppkExtra = ppkNewExtra;
    *ppkNewExtra = pkExtra;
    LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
    return true;
}

const NiFixedString& NiObjectNET::GetName() const {
    return m_kName;
}

NiTimeController* NiObjectNET::GetControllers() const {
    return m_spControllers;
}

void NiObjectNET::SetControllers(NiTimeController* pkControl) {
    m_spControllers = pkControl;
}

void NiObjectNET::PrependController(NiTimeController* pkControl) {
    if (!pkControl) {
        return;
    }

    pkControl->m_spNext = m_spControllers.m_pObject;
    m_spControllers = pkControl;
}

NiTimeController* NiObjectNET::GetController(const NiRTTI* pkRTTI) const {
    for (NiTimeController* pController = m_spControllers; pController; pController = pController->m_spNext) {
        const NiRTTI* pRTTI = pController->GetRTTI();
        for (; pRTTI; pRTTI = pRTTI->m_pkBaseRTTI) {
            if (pRTTI == pkRTTI) {
                return pController;
            }
        }
    }
    return nullptr;
}

// The removed controller is held across the unlink and released afterwards.
void NiObjectNET::RemoveController(NiTimeController* pkControl) {
    if (!pkControl || !m_spControllers) {
        return;
    }
    if (m_spControllers == pkControl) {
        pkControl->IncRefCount();
        m_spControllers = pkControl->m_spNext.m_pObject;
        pkControl->m_spNext = nullptr;
        pkControl->DecRefCount();
        return;
    }
    NiTimeController* pkPrev = m_spControllers;
    NiTimeController* pkCurr = pkPrev->m_spNext.m_pObject;
    while (pkCurr && pkCurr != pkControl) {
        pkPrev = pkCurr;
        pkCurr = pkCurr->m_spNext.m_pObject;
    }
    if (!pkCurr) {
        return;
    }
    pkControl->IncRefCount();
    pkPrev->SetNext(pkControl->m_spNext.m_pObject);
    pkControl->m_spNext = nullptr;
    // Releases the reference taken above, as NiPointer's destructor would on the parameter.
    if (pkControl) {
        pkControl->DecRefCount();
    }
}

void NiObjectNET::RemoveAllControllers() {
    m_spControllers = nullptr;
}

// Copies the name (shared, or with the append character for a unique copy), clones of the cloneable extra data, and a
// clone of the controller chain.
void NiObjectNET::CopyMembers(NiObjectNET* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    if (kCloning.m_eCopyType == COPY_EXACT) {
        pkDest->m_kName = m_kName;
    }
    else if ((kCloning.m_eCopyType == COPY_UNIQUE) && m_kName.GetHandle()) {
        size_t stQuantity = strlen(m_kName.GetHandle()) + 2;
        char* pcNamePlus = static_cast<char*>(_NiMalloc(stQuantity));
        strcpy_s(pcNamePlus, stQuantity, m_kName.GetHandle());
        pcNamePlus[stQuantity - 2] = kCloning.m_cAppendChar;
        pcNamePlus[stQuantity - 1] = 0;
        pkDest->SetName(NiFixedString(pcNamePlus));
        _NiFree(pcNamePlus);
    }

    for (uint16_t i = 0; i < m_usExtraDataSize; ++i) {
        NiExtraData* pkExtra = m_ppkExtra[i];
        if (pkExtra && pkExtra->IsCloneable()) {
            pkDest->AddExtraData(static_cast<NiExtraData*>(pkExtra->CreateClone(kCloning)));
        }
    }
    if (m_spControllers) {
        pkDest->m_spControllers = static_cast<NiTimeController*>(m_spControllers->CreateClone(kCloning));
    }
}

NiExtraData* NiObjectNET::GetExtraData(const NiFixedString& kKey) const {
    if (kKey.GetHandle() == NiGlobalStringTable::NULL_STRING) {
        return nullptr;
    }
    ExtraDataCrit.Lock();
    int16_t iHigh = static_cast<int16_t>(m_usExtraDataSize - 1);
    int16_t iLow = 0;
    while (iLow <= iHigh) {
        int16_t iMiddle = static_cast<int16_t>((iLow + iHigh) >> 1);
        const char* pcKey = kKey.GetHandle();
        const char* pcName = m_ppkExtra[iMiddle]->GetName().GetHandle();
        // Subtracts untyped handle addresses, not character positions within one string.
        int32_t iDifference = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(pcKey) - reinterpret_cast<uintptr_t>(pcName)
        );
        if (!iDifference) {
            ExtraDataCrit.Unlock();
            return m_ppkExtra[iMiddle];
        }
        if (iDifference > 0) {
            iLow = static_cast<int16_t>(iMiddle + 1);
        }
        else {
            iHigh = static_cast<int16_t>(iMiddle - 1);
        }
    }
    ExtraDataCrit.Unlock();
    return nullptr;
}

bool NiObjectNET::RemoveExtraData(const NiFixedString& kKey) {
    if (!m_usExtraDataSize) {
        return false;
    }
    EnterCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
    if (kKey.GetHandle() == NiGlobalStringTable::NULL_STRING) {
        LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
        return false;
    }
    int16_t sTop = m_usExtraDataSize - 1;
    int16_t sBottom = 0;
    if (sTop >= 0) {
        while (true) {
            const char* m_kHandle = kKey.GetHandle();
            int16_t sMiddle = (sBottom + sTop) >> 1;
            int32_t v8 = m_kHandle - this->m_ppkExtra[sMiddle]->GetName().GetHandle();
            if (!v8) {
                DeleteExtraData(static_cast<uint16_t>(sMiddle));
                LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
                return true;
            }
            if (v8 <= 0) {
                sTop = sMiddle - 1;
            }
            else {
                sBottom = sMiddle + 1;
            }
            if (sBottom > sTop) {
                break;
            }
        }
    }
    LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
    return false;
}

// Names an unnamed pkExtra "<RTTI name up to 'ExtraData'>ED<index>" before inserting it.
bool NiObjectNET::AddExtraData(NiExtraData* pkExtra) {
    if (!pkExtra) {
        return false;
    }
    if (!pkExtra->GetName().GetHandle()) {
        const char* pcRTTI = pkExtra->GetRTTI()->m_pcName;
        if (pcRTTI && strlen(pcRTTI) > 0) {
            char pcSuffix[6];
            NiSprintf(pcSuffix, 6, "ED%03d", m_usExtraDataSize);
            const char* pcSubstr = std::strstr(pcRTTI, "ExtraData");
            uint32_t uiStrLength = 0;
            if (pcSubstr > pcRTTI) {
                uiStrLength = static_cast<uint32_t>(pcSubstr - pcRTTI);
            }
            uint32_t uiLen = static_cast<uint32_t>(strlen(pcSuffix)) + uiStrLength + 1;
            char* pcGeneratedKey = static_cast<char*>(_NiMalloc(uiLen));
            NiStrncpy(pcGeneratedKey, uiLen, pcRTTI, uiStrLength);
            strcat_s(pcGeneratedKey, uiLen, pcSuffix);
            pkExtra->SetName(NiFixedString(pcGeneratedKey));
            _NiFree(pcGeneratedKey);
        }
    }
    return InsertExtraData(pkExtra);
}

bool NiObjectNET::AddExtraData(const NiFixedString& kKey, NiExtraData* pkExtra) {
    if (kKey.GetHandle() == NiGlobalStringTable::NULL_STRING) {
        return false;
    }
    if (pkExtra) {
        if (pkExtra->GetName().GetHandle() == NiGlobalStringTable::NULL_STRING) {
            pkExtra->SetName(kKey);
            return InsertExtraData(pkExtra);
        }
        if (kKey.GetHandle() == pkExtra->GetName().GetHandle()) {
            return InsertExtraData(pkExtra);
        }
    }
    return false;
}

void NiObjectNET::SetName(const NiFixedString& strName) {
    m_kName.Assign(strName);
}

void NiObjectNET::SetDefaultCopyType(CopyType eCopyType) {
    ms_eCopyType = eCopyType;
}
