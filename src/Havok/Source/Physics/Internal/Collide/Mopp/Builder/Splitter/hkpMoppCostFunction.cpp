#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppCostFunction.h"

#include <cmath>

hkpMoppCostFunction::hkpMoppSplitCostParams::hkpMoppSplitCostParams(int32_t aiUnused)
  : m_weightPrimitiveSplit(1.0f), m_weightPlaneDistance(1.0f), m_weightNumUnbalancedTriangles(1.0f),
    m_weightPlanePosition(1.0f), m_weightPrimitiveIdSpread(1.0f), m_queryErrorTolerance(0.5f) {}

hkpMoppCostFunction::hkpMoppCostFunction(const hkpMoppSplitCostParams& params) : m_userScaling(0) {
    updateParams(params);
}

// The scaling weights are plain floats: nothing to release.
hkpMoppCostFunction::~hkpMoppCostFunction() {}

void hkpMoppCostFunction::updateParams(const hkpMoppCostFunction::hkpMoppSplitCostParams& bp) {
    m_userScaling = bp;
}

// The fourth power of the unbalance between the primitives right and left of the planes (scaled by the inverse
// primitive count, squared at or past the optimal depth, else shifted by 0.75 and scaled by 6 with 0 below that), times
// its weight. The difference is converted as a signed int and the depths are compared signed.
float hkpMoppCostFunction::costNumUnBalanced(const hkpPlanesParams& p) {
    const int32_t iDifference = static_cast<int32_t>(
        static_cast<uint32_t>(p.m_currentNumPrimitivesRight) - static_cast<uint32_t>(p.m_currentNumPrimitivesLeft)
    );
    float fUnbalance = std::fabs(static_cast<double>(iDifference));
    fUnbalance = fUnbalance * p.m_numPrimitivesInv;
    double dBase;
    if (p.m_currentDepth >= p.m_optimalDepth) {
        const float fSquare = fUnbalance * fUnbalance;
        dBase = fSquare;
    }
    else {
        fUnbalance = fUnbalance - 0.75;
        fUnbalance = fUnbalance * 6.0;
        if (fUnbalance < 0.0f) {
            return 0.0;
        }
        dBase = fUnbalance;
    }
    const float fPower = dBase * dBase * dBase * dBase;
    return fPower * m_userScaling.m_weightNumUnbalancedTriangles;
}

float hkpMoppCostFunction::costUnbalancedPlaneRight(const hkpMoppCostFunction::hkpPlaneRightParams& p) {
    double m_extentsInv;
    double m_planeRightPosition = p.m_planeRightPosition;
    bool v4 = p.m_currentDepth < p.m_optimalDepth;
    float a1b = (p.m_absoluteMax + p.m_absoluteMin) * 0.5;
    float v14 = m_planeRightPosition - a1b;
    if (v4) {
        double m_queryErrorTolerance_3;
        float m_queryErrorTolerance_2 = p.m_absoluteMax - p.m_absoluteMin;
        float m_queryErrorTolerance = m_userScaling.m_queryErrorTolerance;
        double m_queryErrorTolerance_1 = m_queryErrorTolerance_2;
        if (!(m_queryErrorTolerance_2 > static_cast<double>(m_queryErrorTolerance))) {
            m_queryErrorTolerance_3 = m_queryErrorTolerance_2;
        }
        else {
            m_queryErrorTolerance_1 = m_queryErrorTolerance;
            m_queryErrorTolerance_3 = m_queryErrorTolerance_2;
        }
        float a1c = m_queryErrorTolerance_1;
        double v7 = m_queryErrorTolerance_3 + a1c;
        if (!(v7 > 0.0)) {
            m_extentsInv = 3.4028202e+37f;
        }
        else {
            m_extentsInv = 1.0 / v7;
        }
    }
    else {
        m_extentsInv = p.m_extentsInv;
    }
    float a1d = m_extentsInv;
    float a1a = a1d * v14;
    double v9 = 0.0;
    if (!(a1a < 0.0f)) {
        float a1e = a1a * a1a;
        double a1e_1 = a1e;
        float a1f = a1e * a1e;
        double v11 = a1f * 3.0 + a1e_1 * 2.9000000953674316;
        float a1g = a1f * a1f;
        float a1h = v11 + a1g * 1500.0;
        return a1h * m_userScaling.m_weightPlanePosition;
    }
    return v9;
}

float hkpMoppCostFunction::costUnbalancedPlaneLeft(const hkpMoppCostFunction::hkpPlanesParams& p) {
    double m_extentsInv;
    double m_planeLeftPosition = p.m_planeLeftPosition;
    bool v4 = p.m_currentDepth < p.m_optimalDepth;
    float a1b = (p.m_absoluteMax + p.m_absoluteMin) * 0.5;
    float v14 = m_planeLeftPosition - a1b;
    if (v4) {
        double m_queryErrorTolerance_3;
        float m_queryErrorTolerance_2 = p.m_absoluteMax - p.m_absoluteMin;
        float m_queryErrorTolerance = m_userScaling.m_queryErrorTolerance;
        double m_queryErrorTolerance_1 = m_queryErrorTolerance_2;
        if (!(m_queryErrorTolerance_2 > static_cast<double>(m_queryErrorTolerance))) {
            m_queryErrorTolerance_3 = m_queryErrorTolerance_2;
        }
        else {
            m_queryErrorTolerance_1 = m_queryErrorTolerance;
            m_queryErrorTolerance_3 = m_queryErrorTolerance_2;
        }
        float a1c = m_queryErrorTolerance_1;
        double v7 = m_queryErrorTolerance_3 + a1c;
        if (!(v7 > 0.0)) {
            m_extentsInv = 3.4028202e+37f;
        }
        else {
            m_extentsInv = 1.0 / v7;
        }
    }
    else {
        m_extentsInv = p.m_extentsInv;
    }
    float a1d = m_extentsInv;
    float a1a = a1d * v14;
    double v9 = 0.0;
    if (!(a1a > 0.0f)) {
        float a1e = a1a * a1a;
        double a1e_1 = a1e;
        float a1f = a1e * a1e;
        double v11 = a1f * 3.0 + a1e_1 * 2.9000000953674316;
        float a1g = a1f * a1f;
        float a1h = v11 + a1g * 1500.0;
        return a1h * m_userScaling.m_weightPlanePosition;
    }
    return v9;
}
