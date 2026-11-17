#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BSMultiBound;
class BSMultiBoundRoom;
class TESFile;
class TESObjectREFR;

// The half extents of a multibound marker reference's box. Only some of the members are declared.
class MultiBoundMarkerData {
public:
    // The default half extents.
    MultiBoundMarkerData();

    const NiPoint3& GetHalfExtents();
    // Copies aExt into the half extents.
    void SetHalfExtents(const NiPoint3& aExt);
    // Reads the XMBO chunk of apFile's current record into the half extents; false when it is not there.
    bool Load(TESFile* apFile);
    void Copy(MultiBoundMarkerData* apCopy);
    void Save();
    bool Compare(MultiBoundMarkerData* apCompare);
    bool IsDefault();

    // The multibound of apRef's multibound marker.
    static BSMultiBound* CreateMultiBoundForRef(TESObjectREFR* apRef);
    // The half extents of a marker without data.
    static const NiPoint3& GetDefaultHalfExtents();
    static BSMultiBoundRoom* CreateRoomForRef(TESObjectREFR* apRef);
    static bool MultiBoundIntersectsCell(TESObjectREFR* apRef, int32_t aiCellX, int32_t aiCellY);

    NiPoint3 HalfExtents;

    static inline NiPoint3 DefHalfExtents{100.0f, 100.0f, 100.0f};
    // The colour the multibound marker is drawn in.
    static inline NiColorA DrawColor{0.0f, 1.0f, 0.0f, 0.5f};
};
