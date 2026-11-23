#include "Fallout Shared/Region System/TESRegionPoint.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

TESRegionPoint::TESRegionPoint() {
    SetX(0.0f);
    SetY(0.0f);
}

TESRegionPoint::TESRegionPoint(float afX, float afY) {
    SetX(afX);
    SetY(afY);
}

TESRegionPoint::TESRegionPoint(const NiPoint3& aNP3) {
    SetX(aNP3.x);
    SetY(aNP3.y);
}

TESRegionPoint::TESRegionPoint(const NiPoint2& aNP2) : Point(aNP2) {}

// Fetches aRP's coordinate before its own for each FloatEQ test.
bool TESRegionPoint::operator==(const TESRegionPoint& aRP) const {
    float fOtherX = aRP.GetX();
    if (!FloatEQ(GetX(), fOtherX, 0.001f)) {
        return false;
    }
    float fOtherY = aRP.GetY();
    return FloatEQ(GetY(), fOtherY, 0.001f);
}

float TESRegionPoint::GetX() const {
    return Point.x;
}

float TESRegionPoint::GetY() const {
    return Point.y;
}

void TESRegionPoint::SetX(float afX) {
    Point.x = afX;
}

void TESRegionPoint::SetY(float afY) {
    Point.y = afY;
}

float TESRegionPoint::GetDistanceSquared(const TESRegionPoint* apOther) const {
    if (!apOther) {
        return -0x1p+0f;
    }
    double dX = GetX();
    float fDeltaX = static_cast<float>(dX - apOther->GetX());
    double dY = GetY();
    float fDeltaY = static_cast<float>(dY - apOther->GetY());
    return fDeltaX * fDeltaX + fDeltaY * fDeltaY;
}

float TESRegionPoint::GetDistance(const TESRegionPoint* apOther) const {
    if (!apOther) {
        return -0x1p+0f;
    }
    return std::sqrtf(GetDistanceSquared(apOther));
}

TESRegionPoint* TESRegionPoint::Copy(TESRegionPoint* apCopyInto) {
    TESRegionPoint* pCopy;
    if (apCopyInto) {
        pCopy = apCopyInto;
    }
    else {
        pCopy = new TESRegionPoint;
    }
    pCopy->SetX(GetX());
    pCopy->SetY(GetY());
    return pCopy;
}

float TESRegionPoint::XIntercept(TESRegionPoint* apVertex1, TESRegionPoint* apVertex2, float afTestY) {
    if (!apVertex1 || !apVertex2) {
        return 0.0;
    }
    float fX2 = apVertex2->GetX();
    float fY2 = apVertex2->GetY();
    double dDeltaX = apVertex1->GetX() - fX2;
    return static_cast<float>(fX2 - dDeltaX / (apVertex1->GetY() - fY2) * (fY2 - afTestY));
}

bool TESRegionPoint::LineSegmentsIntersect(
    TESRegionPoint* apP1,
    TESRegionPoint* apP2,
    TESRegionPoint* apR1,
    TESRegionPoint* apR2
) {
    float fPMinY;
    float fPMinX;
    float fPMaxX;
    float fPMaxY;
    if (!apP1 || !apP2 || !apR1 || !apR2) {
        return false;
    }
    double dP2X = apP2->GetX();
    float fPDeltaX = static_cast<float>(dP2X - apP1->GetX());
    double dR1X = apR1->GetX();
    float fRDeltaX = static_cast<float>(dR1X - apR2->GetX());
    if (fPDeltaX < 0.0) {
        fPMaxX = apP1->GetX();
        fPMinX = apP2->GetX();
    }
    else {
        fPMaxX = apP2->GetX();
        fPMinX = apP1->GetX();
    }
    if (fRDeltaX > 0.0) {
        if (static_cast<double>(fPMaxX) < apR2->GetX() || static_cast<double>(fPMinX) > apR1->GetX()) {
            return false;
        }
    }
    else if (static_cast<double>(fPMaxX) < apR1->GetX() || static_cast<double>(fPMinX) > apR2->GetX()) {
        return false;
    }
    double dP2Y = apP2->GetY();
    float fPDeltaY = static_cast<float>(dP2Y - apP1->GetY());
    double dR1Y = apR1->GetY();
    float fRDeltaY = static_cast<float>(dR1Y - apR2->GetY());
    if (fPDeltaY < 0.0) {
        fPMaxY = apP1->GetY();
        fPMinY = apP2->GetY();
    }
    else {
        fPMaxY = apP2->GetY();
        fPMinY = apP1->GetY();
    }
    if (fRDeltaY > 0.0) {
        if (static_cast<double>(fPMaxY) < apR2->GetY() || static_cast<double>(fPMinY) > apR1->GetY()) {
            return false;
        }
    }
    else if (static_cast<double>(fPMaxY) < apR1->GetY() || static_cast<double>(fPMinY) > apR2->GetY()) {
        return false;
    }
    double dP1X = apP1->GetX();
    float fOffsetX = static_cast<float>(dP1X - apR1->GetX());
    double dP1Y = apP1->GetY();
    float fOffsetY = static_cast<float>(dP1Y - apR1->GetY());
    float fDenominator = fPDeltaY * fRDeltaX - fPDeltaX * fRDeltaY;
    float fPNumerator = fPDeltaX * fOffsetY - fPDeltaY * fOffsetX;
    float fRNumerator = fRDeltaY * fOffsetX - fRDeltaX * fOffsetY;
    if (fDenominator == 0.0) {
        return true;
    }
    if (fDenominator > 0.0) {
        if (fRNumerator < 0.0 || fDenominator < static_cast<double>(fRNumerator)) {
            return false;
        }
        if (fPNumerator < 0.0 || fDenominator < static_cast<double>(fPNumerator)) {
            return false;
        }
    }
    else {
        if (fRNumerator > 0.0 || fDenominator > static_cast<double>(fRNumerator)) {
            return false;
        }
        if (fPNumerator > 0.0 || fDenominator > static_cast<double>(fPNumerator)) {
            return false;
        }
    }
    return true;
}
