#pragma once

#include <cstdint>

class CIdvBezierSpline;
class CTreeFileAccess;

// The parameters of one branch level as a tree file stores them; the splines belong to it.
struct SIdvBranchInfo {
    SIdvBranchInfo();
    ~SIdvBranchInfo();

    // Replace a spline, deleting the one held before unless it is the same.
    void SetDisturbance(CIdvBezierSpline* pSpline);
    void SetLength(CIdvBezierSpline* pSpline);
    void SetAngleProfile(CIdvBezierSpline* pSpline);
    // The other spline setters, like SetDisturbance.
    void SetGravity(CIdvBezierSpline* pSpline);
    void SetFlexibility(CIdvBezierSpline* pSpline);
    void SetFlexibilityScale(CIdvBezierSpline* pSpline);
    void SetRadius(CIdvBezierSpline* pSpline);
    void SetRadiusScale(CIdvBezierSpline* pSpline);
    void SetStartAngle(CIdvBezierSpline* pSpline);

    // Reads one branch level of a tree file into a new SIdvBranchInfo.
    static SIdvBranchInfo* Parse(CTreeFileAccess& cFile);

    int32_t m_nCrossSectionSegments;
    int32_t m_nSegments;
    float m_fFirstBranch;
    float m_fLastBranch;
    float m_fFrequency;
    float m_fSTile;
    float m_fTTile;
    bool m_bSTileAbsolute;
    bool m_bTTileAbsolute;
    bool m_bRandomTCoordOffset;
    float m_fTwist;
    float m_fSegmentPackingExponent;
    int32_t m_nNumFlares;
    float m_fFlareBalance;
    float m_fRadialInfluence;
    float m_fRadialInfluenceVariance;
    float m_fRadialExponent;
    float m_fRadialDistance;
    float m_fRadialVariance;
    float m_fLengthDistance;
    float m_fLengthVariance;
    float m_fLengthExponent;
    CIdvBezierSpline* m_pDisturbance;
    CIdvBezierSpline* m_pGravity;
    CIdvBezierSpline* m_pFlexibility;
    CIdvBezierSpline* m_pFlexibilityScale;
    CIdvBezierSpline* m_pLength;
    CIdvBezierSpline* m_pRadius;
    CIdvBezierSpline* m_pRadiusScale;
    CIdvBezierSpline* m_pStartAngle;
    CIdvBezierSpline* m_pAngleProfile;
};
