#pragma once

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
    CIdvBezierSpline(CIdvBezierSpline* pSpline);
    ~CIdvBezierSpline();

    // The raw spline point at fPercent (clamped to [0, 1]): the last control point at the end, else the Bezier
    // segment's interpolation.
    CVec EvaluateRawPoint(float fPercent);
    float Evaluate(float fPercent);
    // A random value within the spline's variance at fPercent.
    float ScaledVariance(float fPercent) const;
    // The Bezier interpolation of the four points at x.
    CVec SplineInterpolate(const CVec& cVec1, const CVec& cVec2, const CVec& cVec3, const CVec& cVec4, float x);
    // Copy assignment: the three floats, then each of the five vectors' operator=.
    CIdvBezierSpline& operator=(const CIdvBezierSpline& arSpline);

protected:
    // Fills m_vEvenlySpacedPoints with nNumPoints points evenly spaced in x, each y interpolated between the raw spline
    // points around it.
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

using g_mSplineCache_t = std::map<std::string, CIdvBezierSpline*>;
extern g_mSplineCache_t g_mSplineCache;
