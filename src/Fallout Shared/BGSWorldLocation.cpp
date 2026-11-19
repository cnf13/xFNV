#include "Fallout Shared/BGSWorldLocation.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

BGSWorldLocation::BGSWorldLocation() : Point(NiPoint3_ZERO), pSpace(nullptr) {}

BGSWorldLocation::BGSWorldLocation(const NiPoint3& arPoint, TESForm* apSpace) : Point(arPoint), pSpace(apSpace) {}

TESObjectCELL* BGSWorldLocation::GetCell() const {
    if (!pSpace) {
        return nullptr;
    }
    int32_t formType = (int32_t)pSpace->TESForm::GetFormType();
    if (formType == 57) {
        return static_cast<TESObjectCELL*>(pSpace);
    }
    if (formType != 65) {
        return nullptr;
    }
    TESWorldSpace* pWorld = static_cast<TESWorldSpace*>(pSpace);
    int32_t v3 = FloatToInt(Point.x) >> 12;
    int32_t v4 = FloatToInt(Point.y) >> 12;
    return pWorld->GetCellFromCellCoord(v3, v4);
}

void BGSWorldLocation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Point), 0xC, 0);
    apSaveGameBuffer->SaveFormID(pSpace, 0);
}

void BGSWorldLocation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 12u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    pSpace = TESForm::GetFormByNumericID(FormID_0);
    if (!BGSWorldLocation::CheckSpace(static_cast<TESObjectCELL*>(pSpace))) {
        pSpace = nullptr;
    }
}

float BGSWorldLocation::GetDistanceSquared(const BGSWorldLocation& arWorldLocation) const {
    NiPoint3 v4;
    if (pSpace != (&arWorldLocation)->pSpace) {
        return 0x1.fffffe0000000p+127f;
    }
    NiPoint3* v3 = &(v4 = (&arWorldLocation)->Point - Point);
    return v3->SqrLength();
}

bool BGSWorldLocation::IsLocationLoaded() const {
    TESObjectCELL* a1 = GetCell();
    return (bool)((uint8_t)(a1 && TES::pInstance->IsCellLoaded(a1, false)));
}

BGSWorldLocation* BGSWorldLocation::Clear() {
    Point = NiPoint3_ZERO;
    pSpace = nullptr;
    return this;
}

void BGSWorldLocation::SetLocation(const NiPoint3& arPoint, TESForm* apSpace) {
    Point = arPoint;
    pSpace = apSpace;
}

void BGSWorldLocation::SetPoint(const NiPoint3& arPoint) {
    Point = arPoint;
}

float BGSWorldLocation::GetDistance(const BGSWorldLocation& arWorldLocation) const {
    NiPoint3* v3;
    NiPoint3 v5;
    if (pSpace == (&arWorldLocation)->pSpace) {
        v3 = &(v5 = (&arWorldLocation)->Point - Point);
        return v3->Length();
    }

    return 0x1.fffffe0000000p+127f;
}

float BGSWorldLocation::GetModifiedDistanceSquared(
    const BGSWorldLocation& arWorldLocation,
    const NiPoint3& arCoordMult
) const {
    NiPoint3 v4;
    if (pSpace != (&arWorldLocation)->pSpace) {
        return 0x1.fffffe0000000p+127f;
    }
    (v4 = (&arWorldLocation)->Point - Point);
    v4.x = (v4.x * (&arCoordMult)->x);
    v4.y = (v4.y * (&arCoordMult)->y);
    v4.z = (v4.z * (&arCoordMult)->z);
    return (&v4)->SqrLength();
}

bool BGSWorldLocation::GetCellOrWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld) {
    if (!pSpace) {
        return false;
    }
    int32_t FormType = pSpace->GetFormType();
    if (FormType == 65) {
        arpWorld = static_cast<TESWorldSpace*>(pSpace);
        arpCell = nullptr;
        return true;
    }

    if (FormType == 57) {
        arpCell = static_cast<TESObjectCELL*>(pSpace);
        arpWorld = nullptr;
        return true;
    }

    return false;
}

char BGSWorldLocation::CheckSpace(TESObjectCELL* a1) {
    if (!a1) {
        return 1;
    }
    uint32_t FormType = a1->GetFormType();
    if (FormType == 57) {
        if (a1->GetInterior()) {
            return 1;
        }
    }
    else {
        if (FormType == 65) {
            return 1;
        }
    }
    return 0;
}
