#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Reconstruction.hpp"

class NiPoint3;

// One corner of a region's outline, a point on the map.
class TESRegionPoint {
public:
	// The default constructor.
	TESRegionPoint();
	TESRegionPoint(float a1, float a2);
	// The constructor from a world position: its x and y.
	TESRegionPoint(const NiPoint3& aNP3);
	TESRegionPoint(const NiPoint2& aNP2);

	// True when both coordinates match aRP's within 0.001 (FloatEQ).
	bool operator==(const TESRegionPoint& aRP) const;
	// Point.x; the exe's copy is folded with DecalData::GetDecalMinWidth (0x6A7F50; thiscall,
	// ST0 float).
	float GetX() const;
	// Point.y; the exe's copy is folded with DecalData::GetDecalMaxWidth (0x6B9130; thiscall,
	// ST0 float).
	float GetY() const;
	// Sets Point.x; the exe's copy is folded with NiFloatKey's time setter (thiscall, ret 4,
	// 0x52CE20).
	void SetX(float afX);
	// Sets Point.y; the exe's copy is folded with NiFloatKey::SetValue (thiscall, ret 4,
	// 0x4F5D90).
	void SetY(float afY);
	float GetDistanceSquared(const TESRegionPoint* apOther) const;
	float GetDistance(const TESRegionPoint* apOther) const;
	TESRegionPoint* Copy(TESRegionPoint* apCopyInto);

	static float XIntercept(TESRegionPoint* apVertex1, TESRegionPoint* apVertex2, float afTestY);
	static bool LineSegmentsIntersect(TESRegionPoint* apP1, TESRegionPoint* apP2, TESRegionPoint* apR1,
		TESRegionPoint* apR2);

	NiPoint2 Point;
};
