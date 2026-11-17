#include "BSMain/BSFurnitureMarker.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <stdint.h>

NiRTTI BSFurnitureMarker::ms_RTTI("BSFurnitureMarker", &NiExtraData::ms_RTTI);

// Compares x, y, z each for equality (NaN unequal), then the number and heading.
bool FurnitureMark::operator==(const FurnitureMark& arSource) const {
    // EXE-INLINED 0x4390C0
    bool bSamePosition =
        Position.x == arSource.Position.x && Position.y == arSource.Position.y && Position.z == arSource.Position.z;
    return bSamePosition && cNumber == arSource.cNumber && sHeading == arSource.sHeading;
}

void FurnitureMark::SetHeading(float afHeading) {
    while (afHeading < 0.0) {
        afHeading += NI_TWO_PI;
    }
    while (afHeading >= NI_TWO_PI) {
        afHeading -= NI_TWO_PI;
    }
    sHeading = static_cast<uint16_t>(static_cast<int32_t>(afHeading * 1000.0));
}

void FurnitureMark::LoadBinary(NiStream& arStream) {
    Position.LoadBinary(arStream);
    NiStreamLoadBinary(arStream, sHeading);
    // The byte is loaded through the 1-byte transfer NiStreamLoadBinary<bool>, which moves one untyped byte, so the
    // field is passed as a bool reference.
    NiStreamLoadBinary<bool>(arStream, reinterpret_cast<bool&>(cNumber));
    // The file stores the number twice (SaveBinary writes it twice); the second copy is read and dropped.
    bool bUnused;
    NiStreamLoadBinary<bool>(arStream, bUnused);
    // Old files store negative headings: rewrap them into [0, 2 pi).
    int16_t sSignedHeading = static_cast<int16_t>(sHeading);
    if (sSignedHeading < 0) {
        SetHeading(static_cast<float>(sSignedHeading / 1000.0));
    }
}

void FurnitureMark::SaveBinary(NiStream& arStream) {
    Position.SaveBinary(arStream);
    NiStreamSaveBinary(arStream, sHeading);
    // The byte is saved twice, as the game does.
    NiStreamSaveByte(&arStream, &cNumber);
    NiStreamSaveByte(&arStream, &cNumber);
}

float FurnitureMark::GetHeading() {
    return (float)(((double)sHeading) / 1000.0);
}

uint32_t FurnitureMark::GetType() const {
    return cNumber;
}

void FurnitureMark::SetType(uint8_t a2) {
    cNumber = a2;
}

void FurnitureMark::SetPosition(const NiPoint3& arPos) {
    Position = arPos;
}

BSFurnitureMarker::BSFurnitureMarker() : NiExtraData(), aMarks(0, 1) {
    Init();
}

BSFurnitureMarker::~BSFurnitureMarker() {}

NiObject* BSFurnitureMarker::CreateClone(NiCloningProcess& arCloneData) {
    return this;
}

// On the uiNum > 0 path the padding byte of kMark is never written by FurnitureMark::LoadBinary, and Add and SetAt copy
// the whole element into the array, so the elements carry uninitialised bytes, as the game does. SetSize fills cleared
// and new slots from an uninitialised FurnitureMark(0) the same way.
void BSFurnitureMarker::LoadBinary(NiStream& arStream) {
    NiExtraData::LoadBinary(arStream);
    uint32_t uiNum;
    NiStreamLoadBinary(arStream, uiNum);
    if (uiNum) {
        aMarks.SetSize(uiNum);
        FurnitureMark kMark;
        for (uint32_t i = 0; i < uiNum; i++) {
            kMark.LoadBinary(arStream);
            aMarks.Add(kMark);
        }
    }
}

void BSFurnitureMarker::LinkObject(NiStream& arStream) {
    NiExtraData::LinkObject(arStream);
}

bool BSFurnitureMarker::RegisterStreamables(NiStream& arStream) {
    return NiExtraData::RegisterStreamables(arStream);
}

void BSFurnitureMarker::SaveBinary(NiStream& arStream) {
    NiExtraData::SaveBinary(arStream);
    uint32_t uiNum = aMarks.GetEffectiveSize();
    NiStreamSaveBinary(arStream, uiNum);
    for (uint32_t i = 0; i < uiNum; i++) {
        aMarks[i].SaveBinary(arStream);
    }
}

bool BSFurnitureMarker::IsEqual(NiObject* apObject) {
    return NiExtraData::IsEqual(apObject);
}

void BSFurnitureMarker::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiExtraData::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSFurnitureMarker::ms_RTTI.GetName()));
    uint32_t uiNum = aMarks.GetEffectiveSize();
    apStrings->Add(NiGetViewerString("iMarks", uiNum));
    for (uint32_t i = 0; i < uiNum; i++) {
        FurnitureMark& rMark = aMarks[i];
        apStrings->Add(NiGetViewerString("Mark", i));
        apStrings->Add(NiGetViewerString(" Number", static_cast<uint32_t>(rMark.cNumber)));
        apStrings->Add(rMark.Position.GetViewerString(" Pos"));
        apStrings->Add(NiGetViewerString(" Heading", static_cast<float>(rMark.sHeading / 1000.0)));
    }
}

// Gamebryo's NiTArray has only the const GetAt, and the mark is handed out writable.
FurnitureMark& BSFurnitureMarker::QMark(uint32_t aiMark) {
    return const_cast<FurnitureMark&>(aMarks.GetAt(aiMark));
}

uint32_t BSFurnitureMarker::QNumMarks() {
    return (uint32_t)((int32_t)(&aMarks)->NiTArray<FurnitureMark, NiTNewInterface<FurnitureMark>>::GetEffectiveSize());
}

void BSFurnitureMarker::Init() {
    SetName(*BSFurnitureMarker::pTag);
}

void BSFurnitureMarker::Add(const FurnitureMark& arMark) {
    uint32_t I;
    uint32_t imarks = aMarks.m_usESize;
    for (I = 0; I < imarks && arMark.cNumber >= static_cast<int32_t>(aMarks.m_pBase[I].cNumber); ++I) {
        ;
    }
    aMarks.Add(arMark);
    if (I < imarks) {
        while (imarks > I) {
            aMarks.SetAt(imarks, aMarks.m_pBase[imarks - 1]);
            --imarks;
        }
        aMarks.SetAt(I, arMark);
    }
}

// Allocates the one "FRN" tag and interns the string.
void BSFurnitureMarker::InitSDM() {
    NiFixedString* pTag = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pTag) {
        pTag->SetHandle(NiGlobalStringTable::AddString("FRN"));
    }
    BSFurnitureMarker::pTag = pTag;
}

// Returns the "FRN" extra data of apObject: the one whose RTTI chain reaches ms_RTTI. The chain is walked from the
// extra data's own GetRTTI, not from the object's.
BSFurnitureMarker* BSFurnitureMarker::Find(NiObjectNET* apObject) {
    if (!apObject) {
        return nullptr;
    }

    NiExtraData* pData = apObject->GetExtraData(*BSFurnitureMarker::pTag);
    if (!pData) {
        return nullptr;
    }

    bool bFound = false;
    for (const NiRTTI* pRTTI = pData->GetRTTI(); pRTTI; pRTTI = pRTTI->m_pkBaseRTTI) {
        if (pRTTI == &BSFurnitureMarker::ms_RTTI) {
            bFound = true;
            break;
        }
    }
    return bFound ? static_cast<BSFurnitureMarker*>(pData) : nullptr;
}

NiObject* BSFurnitureMarker::CreateObject() {
    BSFurnitureMarker* v1;
    void* v2 = NiMemObject::operator new(sizeof(BSFurnitureMarker));
    if (v2) {
        v1 = ::new (v2) BSFurnitureMarker;
    }
    else {
        v1 = nullptr;
    }
    v1->SetName(EmptyFS);
    return v1;
}

void BSFurnitureMarker::KillSDM() {
    NiFixedString* Block = BSFurnitureMarker::pTag;
    if (BSFurnitureMarker::pTag) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSFurnitureMarker::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    BSFurnitureMarker::pTag = 0;
}
