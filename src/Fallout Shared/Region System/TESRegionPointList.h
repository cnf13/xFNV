#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

class TESFile;

class TESRegionPointList : public BSSimpleList<TESRegionPoint*> {
public:
	// A cached scale of the outline: the point it was computed for, the distance and the scale.
	struct ScaleResult {
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
	// Stores the list's RPLI chunk value in iDistanceInsideAtMax.
	void SetDistanceInsideAtMax(uint32_t aiDistance);
	// Whether the outline crosses itself; abClosePolygon adds the closing edge.
	bool SelfIntersects(bool abClosePolygon);
	bool Add(TESRegionPoint* apPoint, float afMaxDelta);
	bool IsInsideRegion(TESRegionPoint* apTest);
	void Save();
	// Frees the points and resets the bounds. Returns this.
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
