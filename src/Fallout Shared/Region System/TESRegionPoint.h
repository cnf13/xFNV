#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

class NiPoint3;

// One corner of a region's outline, a point on the map.
class TESRegionPoint {
public:
	TESRegionPoint();
	TESRegionPoint(float a1, float a2);
	// The constructor from a world position: its x and y.
	TESRegionPoint(const NiPoint3& aNP3);
	TESRegionPoint(const NiPoint2& aNP2);

	// True when both coordinates match aRP's within 0.001 (FloatEQ).
	bool operator==(const TESRegionPoint& aRP) const;
	// Returns Point.x.
	float GetX() const;
	// Returns Point.y.
	float GetY() const;
	// Sets Point.x.
	void SetX(float afX);
	void SetY(float afY);
	float GetDistanceSquared(const TESRegionPoint* apOther) const;
	float GetDistance(const TESRegionPoint* apOther) const;
	TESRegionPoint* Copy(TESRegionPoint* apCopyInto);

	static float XIntercept(TESRegionPoint* apVertex1, TESRegionPoint* apVertex2, float afTestY);
	static bool LineSegmentsIntersect(TESRegionPoint* apP1, TESRegionPoint* apP2, TESRegionPoint* apR1,
		TESRegionPoint* apR2);

	NiPoint2 Point;
};
