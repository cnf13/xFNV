#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class TESForm;
class TESObjectCELL;
class TESWorldSpace;

// A point in a space: an interior cell or a world space.
class BGSWorldLocation {
public:
    BGSWorldLocation();
    BGSWorldLocation(const NiPoint3& arPoint, TESForm* apSpace);

    // The cell of the location.
    TESObjectCELL* GetCell() const;
    // Writes the position and the space's form ID.
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    // Reads the position and the space, and clears the space when it is not a cell or world space.
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    float GetDistanceSquared(const BGSWorldLocation& arWorldLocation) const;
    bool IsLocationLoaded() const;
    BGSWorldLocation* Clear();
    void SetLocation(const NiPoint3& arPoint, TESForm* apSpace);
    void SetPoint(const NiPoint3& arPoint);
    TESForm* GetSpace();
    NiPoint3* GetPoint();
    // Stores apSpace in pSpace.
    void SetSpace(TESForm* apSpace) {
        pSpace = apSpace;
    }
    float GetDistance(const BGSWorldLocation& arWorldLocation) const;
    float GetModifiedDistanceSquared(const BGSWorldLocation& arWorldLocation, const NiPoint3& arCoordMult) const;
    bool GetCellOrWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld);

    static char CheckSpace(TESObjectCELL* a1);

    NiPoint3 Point;
    // An interior cell or a world space: the form's type is tested to treat it as one or the other.
    TESForm* pSpace;
};
