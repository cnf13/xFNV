#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Reconstruction.hpp"

class TESFile;

class TESRegionPointList : public BSSimpleList<TESRegionPoint*> {
public:
	// A cached scale of the outline: the point it was computed for, the distance and the scale.
	struct ScaleResult {
		// The compiler's default constructor: builds point (thiscall, ret 0, 0x4F87E0; IDA names it
		// TESRegionPoint::TESRegionPoint_0, but GetScaleBasedOnLocation calls it on its 16-byte ScaleResult).
		ScaleResult();

		// Whether apPoint and afDistance are the ones cached (FloatEQ within 0.001).
		bool Matches(TESRegionPoint* apPoint, float afDistance);

		TESRegionPoint point;
		float fdist;
		float fscale;
	};

	// The constructor; abOwnsPointMemory is true for a list loaded from a file.
	TESRegionPointList(bool abOwnsPointMemory);
	~TESRegionPointList();

	// Reads the list's RPLD point chunks.
	bool Load(TESFile* apFile);
	// Whether the outline has enough points and does not cross itself.
	bool Validate(bool abMinPointsTest);
	// Stores the list's RPLI chunk dword in the field at +0x20 (iDistanceInsideAtMax) through
	// the exe's shared one-field setter (thiscall, ret 4, 0x50F9C0).
	void SetDistanceInsideAtMax(uint32_t aiDistance);
	// Whether the outline crosses itself; abClosePolygon adds the closing edge.
	bool SelfIntersects(bool abClosePolygon);
	bool Add(TESRegionPoint* apPoint, float afMaxDelta);
	bool IsInsideRegion(TESRegionPoint* apTest);
	void Save();
	// Frees the points and resets the bounds. PDB: void Clear(); the exe leaves this in eax.
	TESRegionPointList* Clear();
	float GetMinX();
	float GetMaxX();
	float GetMinY();
	float GetMaxY();
	TESRegionPointList* Copy(TESRegionPointList* apCopyInto);
	float GetScaleBasedOnLocation(TESRegionPoint* apTest, float afDistInsideAtMax);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	ScaleResult* pLastScaleResult;
	bool bOwnsPointMemory;
	uint8_t padding0D[3];
	NiPoint2 Minimums;
	NiPoint2 Maximums;
	uint32_t iDistanceInsideAtMax;
	uint32_t iCount;
};
