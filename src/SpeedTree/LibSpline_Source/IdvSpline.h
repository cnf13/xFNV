#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/iosfwd.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"

#include <functional>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class CIdvBezierSpline;

const char* NextToken(const char* pInput, char* pToken);

// A Bezier spline of a tree parameter: the range its values are scaled to, a variance, and its control points and
// cached evaluations in five STL vectors.
class CIdvBezierSpline {
public:
	CIdvBezierSpline();
	CIdvBezierSpline(const std::string& strInput);
	CIdvBezierSpline(CIdvBezierSpline* a1);
	~CIdvBezierSpline();

	// The raw spline point at fPercent (clamped to [0, 1]): the last control point at the end, else the Bezier
	// segment's interpolation (thiscall, ret 8: hidden result and fPercent; 0xB38B50).
	CVec EvaluateRawPoint(float fPercent);
	// (KEEP in IdvSpline.cpp gives the reason). The spline's value at fPercent (thiscall,
	// ret 4, result in ST(0), 0xB37AD0).
	float Evaluate(float fPercent);
	// (KEEP in IdvSpline.cpp gives the reason). A random value within the spline's variance
	// at fPercent (thiscall, ret 4, result in ST(0), 0xB37FA0).
	float ScaledVariance(float fPercent) const;
	// The Bezier interpolation of the four points at x (thiscall, ret 0x18: hidden result,
	// the four points by address and x; 0xB38F40).
	CVec SplineInterpolate(const CVec& cVec1, const CVec& cVec2, const CVec& cVec3, const CVec& cVec4, float x);
	// The exe's copy assignment: the three floats, then each of the five vectors' out-of-line operator= (thiscall,
	// ret 4, returns this; 0xB1C7A0).
	CIdvBezierSpline& operator=(const CIdvBezierSpline& arSpline);

protected:
	// Private in the PDB; protected for the registration. Fills m_vEvenlySpacedPoints with nNumPoints points
	// evenly spaced in x, each y interpolated between the raw spline points around it (thiscall, ret 4).
	void CreateEvenlySpacedPoints(uint32_t nNumPoints);

public:
	void Parse(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& strInput);
	void AddControlPoint(float* afPoint, float* afTangent, float fTangentLength);

	// Deletes every cached spline and empties the spline cache.
	static void ClearCache();

	float m_fMin;
	float m_fMax;
	float m_fVariance;
	// The control points, their tangents and tangent lengths, the evenly spaced points and the spline points: PC
	// STL vectors of 0x18 bytes each.
	std::vector<CVec, std::allocator<CVec>> m_vControlPoints;
	std::vector<CVec, std::allocator<CVec>> m_vControlPointTangents;
	std::vector<float, std::allocator<float>> m_vControlPointTangentLengths;
	std::vector<CVec, std::allocator<CVec>> m_vEvenlySpacedPoints;
	std::vector<CVec, std::allocator<CVec>> m_vSplinePoints;
};

// The exe's std::map<std::string, CIdvBezierSpline*>.
using g_mSplineCache_t = std::map<std::string, CIdvBezierSpline*>;
extern g_mSplineCache_t g_mSplineCache;
