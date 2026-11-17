#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/ehvec.h"

struct Segment;
struct Sphere;

// The navmesh geometry helpers.
class NavMeshMath {
public:
	// How a segment meets a circle.
	enum CIRCLE_SEGMENT_INTERSECTION : int32_t {
		CSI_NONE = 0,
		CSI_ONEPOINT_LOWER = 1,
		CSI_ONEPOINT_UPPER = 2,
		CSI_TANGENT = 3,
		CSI_TWOPOINTS = 4,
		CSI_CONTAINED = 5,
		CSI_COUNT = 6,
	};

	// The kind of one CSIResult.
	enum CIRCLE_SEGMENT_INTERSECTION_TYPE : int32_t {
		CSIT_NONE = 0,
		CSIT_EDGE = 1,
		CSIT_ENDPOINT = 2,
	};

	// One intersection: its classification, circle parameter and segment parameter.
	struct CSIResult {
		CIRCLE_SEGMENT_INTERSECTION_TYPE eType;
		AngleSinCos CircleParam;
		float fSegmentParam;
	};

	// The constructor initializes the two results. CircleSegmentIntersection writes the classifications and float
	// parameters.
	struct CSIResults {
		CSIResults();

		CIRCLE_SEGMENT_INTERSECTION eType;
		CSIResult Results[2];
	};

	// How many tangents two circles have.
	enum CIRCLE_CIRCLE_TANGENT : int32_t {
		CCT_NONE = 0,
		CCT_ONEPOINT = 1,
		CCT_TWOPOINTS = 2,
		CCT_COUNT = 3,
	};

	// One tangent between two circles: its direction and where it touches each circle. The constructor leaves Direction
	// alone; ComputeTangentPoints writes AD_CCW or AD_CW into it.
	struct CCTResult {
		ANGLE_DIRECTION Direction;
		AngleSinCos Circle1Param;
		AngleSinCos Circle2Param;
	};

	// The tangents between two circles.
	struct CCTResults {
		CCTResults();

		CIRCLE_CIRCLE_TANGENT eType;
		CCTResult Results[2];
	};

	// Whether aLocation lies over or under the triangle within the given heights; afZDiff
	// gets its height above the triangle.
	static bool IsPointInsideTriangle(const NiPoint3& aTriVert1Loc, const NiPoint3& aTriVert2Loc,
		const NiPoint3& aTriVert3Loc, const NiPoint3& aLocation, float& afZDiff, float afMaxZdistAbove,
		float afMaxZdistBelow);
	// Whether the two segments cross in x and y; apReturnIntersection gets the crossing point.
	static bool Find2DIntersectionStable(const NiPoint3& aSegment1Start, const NiPoint3& aSegment1End,
		const NiPoint3& aSegment2Start, const NiPoint3& aSegment2End, NiPoint3* apReturnIntersection);
	static int8_t Get2DBaryCentricCoord_0(float* apA, float* apB, float* apC, float* apD, float* apReturn);
	static bool Find2DIntersection(const NiPoint2& a1, const NiPoint2& a2, const NiPoint2& a3, const NiPoint2& a4);
	// Find2DIntersection for NiPoint3: builds NiPoint2s of the four points and calls the NiPoint2 overload.
	static bool Find2DIntersection3(const NiPoint3& aSegment1Start, const NiPoint3& aSegment1End,
		const NiPoint3& aSegment2Start, const NiPoint3& aSegment2End);
	// Whether the two segments cross in x and y; apReturnIntersection gets the crossing point.
	static bool Find2DIntersectionPoint(const NiPoint3& aSegment1Start, const NiPoint3& aSegment1End,
		const NiPoint3& aSegment2Start, const NiPoint3& aSegment2End, NiPoint3* apReturnIntersection);
	// The point of the segment from aLineStart to aLineEnd closest to aPoint.
	static NiPoint3 ProjectPointOntoSegment(const NiPoint3& aPoint, const NiPoint3& aLineStart,
		const NiPoint3& aLineEnd);
	// The barycentric coordinates of aLoc in the triangle in x and y (the NiPoint3 overload).
	static bool Get2DBaryCentricCoord(const NiPoint3& aV0, const NiPoint3& aV1, const NiPoint3& aV2,
		const NiPoint3& aLoc, NiPoint3& aCoord);
	// The point of the segment nearest to the line through aLineStart and aLineEnd (the NiPoint2 overload).
	static NiPoint2 ClosestPointOnSegmentToLine(const NiPoint2& aSegmentStart, const NiPoint2& aSegmentEnd,
		const NiPoint2& aLineStart, const NiPoint2& aLineEnd);
	// The NiPoint3 overload of ClosestPointOnSegmentToLine.
	static NiPoint3 ClosestPointOnSegmentToLine3(const NiPoint3& aSegmentStart, const NiPoint3& aSegmentEnd,
		const NiPoint3& aLineStart, const NiPoint3& aLineEnd);
	// The distance from aPoint to the segment from aLineStart to aLineEnd; apRetInSegment, when given, gets whether the
	// nearest point lies inside the segment.
	static float DistanceFromSegment(const NiPoint3& aPoint, const NiPoint3& aLineStart, const NiPoint3& aLineEnd,
		bool* apRetInSegment);
	// Whether the circle around aCircleCenter meets the segment.
	static bool DoCircleAndSegmentIntersect(const NiPoint3& aCircleCenter, float afCircleRadius,
		const NiPoint3& aSegmentStart, const NiPoint3& aSegmentEnd);
	// Where the segment meets the circle, written to aResults.
	static bool CircleSegmentIntersection(const Sphere& aCircle, const Segment& aSegment, CSIResults& aResults);
	// The tangents between the two circles going around them in aDirection, written to
	// aResults; returns their count, also stored in aResults.eType.
	static CIRCLE_CIRCLE_TANGENT ComputeTangentPoints(const Sphere& aCircle1, const Sphere& aCircle2,
		ANGLE_DIRECTION aDirection, CCTResults& aResults);
	static float DistanceFromLine(const NiPoint3& aPoint, const NiPoint3& aLineStart, const NiPoint3& aLineEnd);
	static int32_t SegmentPlaneIntersection(const NiPoint3& aLineStart, const NiPoint3& aLineEnd,
		const NiPoint3& aPlaneV0, const NiPoint3& aPlaneNormal, NiPoint3* apRet);
	static bool SegmentCircleIntersection(const NiPoint3& aSegmentStart, const NiPoint3& aSegmentEnd,
		const NiPoint3& aCircleCenter, float afCircleRadius);
	static bool CheckArcAgainstCircle(const Sphere& aCircle1, const AngleSinCosArc& aArc, const Sphere& aCircle2);
	static NiPoint3* RandomPointOnTriangle(NiPoint3* apResult, const NiPoint3& aVert0, const NiPoint3& aVert1,
		const NiPoint3& aVert2);
	static bool Find2DIntersection_006B9E90(const NiPoint2& aSegment1Start, const NiPoint2& aSegment1End,
		const NiPoint2& aSegment2Start, const NiPoint2& aSegment2End, NiPoint2* apReturnIntersection);
	// Whether the line between the cover's two vertices meets the line from apTargetLocation to apCoverPoint, and
	// where.
	static bool LineToLineSegment2DIntersection(const NiPoint3* apLeftVertex, const NiPoint3* apRightVertex,
		const NiPoint3* apTargetLocation, const NiPoint3* apCoverPoint, NiPoint3* apIntersection);
	static char LineSegmentToLine2DIntersection(NiPoint3* a1, NiPoint3* a2, NiPoint3* pt, NiPoint3* a4, NiPoint3* a5);
	static char ComputeTangentPointsToCircleFromPoint(NiPoint3* pt, float a2, NiPoint3* a3, float* a4, float* a5);
};
