#include "Fallout Shared/Region System/TESRegionPoint.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

TESRegionPoint::TESRegionPoint() {
    SetX(0.0f);
    SetY(0.0f);
}

TESRegionPoint::TESRegionPoint(float a1, float a2) {
    SetX(a1);
    SetY(a2);
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
    if (FloatEQ(GetY(), fOtherY, 0.001f)) {
        return true;
    }
    return false;
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
        return (-0x1p+0f);
    }
    double v5 = (double)GetX();
    float v7 = (float)(v5 - apOther->GetX());
    double flt004 = (double)GetY();
    float v8 = (float)(flt004 - apOther->GetY());
    return ((v7 * v7) + (v8 * v8));
}

float TESRegionPoint::GetDistance(const TESRegionPoint* apOther) const {
    if (!apOther) {
        return (-0x1p+0f);
    }
    float x = GetDistanceSquared(apOther);
    return std::sqrtf(x);
}

TESRegionPoint* TESRegionPoint::Copy(TESRegionPoint* apCopyInto) {
    TESRegionPoint* v5;
    TESRegionPoint* v6;
    void* apMemory;
    if (apCopyInto) {
        v6 = apCopyInto;
    }
    else {
        apMemory = ourMalloc(sizeof(TESRegionPoint));
        if (apMemory) {
            v5 = ::new (apMemory) TESRegionPoint;
        }
        else {
            v5 = nullptr;
        }
        v6 = v5;
    }
    float a2 = GetX();
    v6->SetX(a2);
    float a2a = GetY();
    v6->SetY(a2a);
    return v6;
}

float TESRegionPoint::XIntercept(TESRegionPoint* apVertex1, TESRegionPoint* apVertex2, float afTestY) {
    if ((!apVertex1) || (!apVertex2)) {
        return 0.0;
    }
    float v6 = apVertex2->GetX();
    float flt004 = apVertex2->GetY();
    double v5 = (double)(apVertex1->GetX() - v6);
    return (float)(v6 - ((v5 / (apVertex1->GetY() - flt004)) * (flt004 - afTestY)));
}

bool TESRegionPoint::LineSegmentsIntersect(
    TESRegionPoint* apP1,
    TESRegionPoint* apP2,
    TESRegionPoint* apR1,
    TESRegionPoint* apR2
) {
    float v12;
    float v14;
    float v18;
    float v23;
    if ((!apP1) || ((!apP2) || ((!apR1) || (!apR2)))) {
        return false;
    }
    double v10 = (double)apP2->GetX();
    float v15 = (float)(v10 - apP1->GetX());
    double v9 = (double)apR1->GetX();
    float v22 = (float)(v9 - apR2->GetX());
    if (!(v15 < 0.0)) {
        v18 = apP2->GetX();
        v14 = apP1->GetX();
    }
    else {
        v18 = apP1->GetX();
        v14 = apP2->GetX();
    }
    if (!(v22 > 0.0)) {
        if ((((double)v18) < apR1->GetX()) || (((double)v14) > apR2->GetX())) {
            return false;
        }
    }
    else {
        if ((((double)v18) < apR2->GetX()) || (((double)v14) > apR1->GetX())) {
            return false;
        }
    }
    double flt004 = (double)apP2->GetY();
    float v17 = (float)(flt004 - apP1->GetY());
    double v7 = (double)apR1->GetY();
    float v20 = (float)(v7 - apR2->GetY());
    if (!(v17 < 0.0)) {
        v23 = apP2->GetY();
        v12 = apP1->GetY();
    }
    else {
        v23 = apP1->GetY();
        v12 = apP2->GetY();
    }
    if (!(v20 > 0.0)) {
        if ((((double)v23) < apR1->GetY()) || (((double)v12) > apR2->GetY())) {
            return false;
        }
    }
    else {
        if ((((double)v23) < apR2->GetY()) || (((double)v12) > apR1->GetY())) {
            return false;
        }
    }
    double v6 = (double)apP1->GetX();
    float v21 = (float)(v6 - apR1->GetX());
    double v5 = (double)apP1->GetY();
    float v11 = (float)(v5 - apR1->GetY());
    float v19 = (v17 * v22) - (v15 * v20);
    float v13 = (v15 * v11) - (v17 * v21);
    float v16 = (v20 * v21) - (v22 * v11);
    if (v19 == 0.0) {
        return true;
    }
    if (!(v19 > 0.0)) {
        if ((v16 > 0.0) || (v19 > ((double)v16))) {
            return false;
        }
        if ((v13 > 0.0) || (v19 > ((double)v13))) {
            return false;
        }
    }
    else {
        if ((v16 < 0.0) || (v19 < ((double)v16))) {
            return false;
        }
        if ((v13 < 0.0) || (v19 < ((double)v13))) {
            return false;
        }
    }
    return true;
}
