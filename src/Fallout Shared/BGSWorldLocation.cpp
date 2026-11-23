#include "Fallout Shared/BGSWorldLocation.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

BGSWorldLocation::BGSWorldLocation() : Point(NiPoint3::ZERO), pSpace(nullptr) {}

BGSWorldLocation::BGSWorldLocation(const NiPoint3& arPoint, TESForm* apSpace) : Point(arPoint), pSpace(apSpace) {}

TESObjectCELL* BGSWorldLocation::GetCell() const {
    if (!pSpace) {
        return nullptr;
    }
    int32_t iFormType = pSpace->TESForm::GetFormType();
    if (iFormType == 57) {
        return static_cast<TESObjectCELL*>(pSpace);
    }
    if (iFormType != 65) {
        return nullptr;
    }
    TESWorldSpace* pWorld = static_cast<TESWorldSpace*>(pSpace);
    int32_t iCellX = FloatToInt(Point.x) >> 12;
    int32_t iCellY = FloatToInt(Point.y) >> 12;
    return pWorld->GetCellFromCellCoord(iCellX, iCellY);
}

void BGSWorldLocation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Point), sizeof(Point), 0);
    apSaveGameBuffer->SaveFormID(pSpace, 0);
}

void BGSWorldLocation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&Point, sizeof(Point));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    pSpace = TESForm::GetFormByNumericID(uiFormID);
    if (!BGSWorldLocation::CheckSpace(static_cast<TESObjectCELL*>(pSpace))) {
        pSpace = nullptr;
    }
}

float BGSWorldLocation::GetDistanceSquared(const BGSWorldLocation& arWorldLocation) const {
    if (pSpace != arWorldLocation.pSpace) {
        return 0x1.fffffe0000000p+127f;
    }
    return (arWorldLocation.Point - Point).SqrLength();
}

bool BGSWorldLocation::IsLocationLoaded() const {
    TESObjectCELL* pCell = GetCell();
    return pCell && TES::pInstance->IsCellLoaded(pCell, false);
}

BGSWorldLocation* BGSWorldLocation::Clear() {
    Point = NiPoint3::ZERO;
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
    if (pSpace == arWorldLocation.pSpace) {
        return (arWorldLocation.Point - Point).Length();
    }

    return 0x1.fffffe0000000p+127f;
}

float BGSWorldLocation::GetModifiedDistanceSquared(
    const BGSWorldLocation& arWorldLocation,
    const NiPoint3& arCoordMult
) const {
    if (pSpace != arWorldLocation.pSpace) {
        return 0x1.fffffe0000000p+127f;
    }
    NiPoint3 kDiff = arWorldLocation.Point - Point;
    kDiff.x = kDiff.x * arCoordMult.x;
    kDiff.y = kDiff.y * arCoordMult.y;
    kDiff.z = kDiff.z * arCoordMult.z;
    return kDiff.SqrLength();
}

bool BGSWorldLocation::GetCellOrWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld) {
    if (!pSpace) {
        return false;
    }
    int32_t iFormType = pSpace->GetFormType();
    if (iFormType == 65) {
        arpWorld = static_cast<TESWorldSpace*>(pSpace);
        arpCell = nullptr;
        return true;
    }

    if (iFormType == 57) {
        arpCell = static_cast<TESObjectCELL*>(pSpace);
        arpWorld = nullptr;
        return true;
    }

    return false;
}

char BGSWorldLocation::CheckSpace(TESObjectCELL* apSpace) {
    if (!apSpace) {
        return 1;
    }
    uint32_t uiFormType = apSpace->GetFormType();
    if (uiFormType == 57) {
        if (apSpace->GetInterior()) {
            return 1;
        }
    }
    else if (uiFormType == 65) {
        return 1;
    }
    return 0;
}
