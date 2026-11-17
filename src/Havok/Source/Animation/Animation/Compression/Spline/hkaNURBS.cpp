#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Animation/Internal/Compression/hkaCompression.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>

static int32_t Bin(int32_t a1, int32_t a2);

static int32_t Bin(int32_t a1, int32_t a2) {
    int32_t v2 = hkaNURBS::Factorial(a1 - a2);
    int32_t v3 = hkaNURBS::Factorial(a2) * v2;
    return hkaNURBS::Factorial(a1) / v3;
}

// The hkArray<hkReal> and hkArray<Point> locals the bodies below build and free.
using hkaNURBSFloatArray = hkArray<float, hkContainerHeapAllocator>;
using hkaNURBSPointArray = hkArray<hkaNURBS::Point, hkContainerHeapAllocator>;

hkaNURBS::Point::Point(float x, float y, float z, float w) : point{x, y, z, w} {}

// The componentwise absolute value, built in apResult (the x87 fabs of each component, last one first).
hkaNURBS::Point* hkaNURBS::Point::abs(hkaNURBS::Point* apResult) {
    const float w = std::fabs(point[3]);
    const float z = std::fabs(point[2]);
    const float y = std::fabs(point[1]);
    const float x = std::fabs(point[0]);
    ::new (apResult) hkaNURBS::Point(x, y, z, w);
    return apResult;
}

// The componentwise floor, built in apResult (the CRT's _floor on each component, last one first).
hkaNURBS::Point* hkaNURBS::Point::floor(hkaNURBS::Point* apResult) {
    const float w = std::floor(static_cast<double>(point[3]));
    const float z = std::floor(static_cast<double>(point[2]));
    const float y = std::floor(static_cast<double>(point[1]));
    const float x = std::floor(static_cast<double>(point[0]));
    ::new (apResult) hkaNURBS::Point(x, y, z, w);
    return apResult;
}

// The componentwise ceiling, built in apResult (the CRT's _ceil on each component, last one first).
hkaNURBS::Point* hkaNURBS::Point::ceil(hkaNURBS::Point* apResult) {
    const float w = std::ceil(static_cast<double>(point[3]));
    const float z = std::ceil(static_cast<double>(point[2]));
    const float y = std::ceil(static_cast<double>(point[1]));
    const float x = std::ceil(static_cast<double>(point[0]));
    ::new (apResult) hkaNURBS::Point(x, y, z, w);
    return apResult;
}

// KEEP: the exe's callers keep ecx and edx live across the call, which the exe body leaves intact and clang does
// not guarantee: approximateNURBSCurveImpl calls it again without reloading ecx (0xE58564) and reads [edx] after it
// (0xE5855D); Point::Max reads edx after it (0xE53672), the exe's LeastSquaresFit too (0xE57C4A).
float& hkaNURBS::Point::operator[](int32_t u) {
    return point[u];
}

float hkaNURBS::Point::mag() const {
    return std::sqrt((double)mag2());
}

hkaNURBS::Point* hkaNURBS::Point::normalize(hkaNURBS::Point* apResult) {
    float d = mag();
    Div(apResult, d);
    return apResult;
}

hkaNURBS::Point& hkaNURBS::Point::operator-=(const hkaNURBS::Point& a) {
    point[0] = point[0] - a.point[0];
    point[1] = point[1] - a.point[1];
    point[2] = point[2] - a.point[2];
    point[3] = point[3] - a.point[3];
    return *this;
}

hkaNURBS::Point& hkaNURBS::Point::operator*=(float d) {
    point[0] = point[0] * d;
    point[1] = point[1] * d;
    point[2] = d * point[2];
    point[3] = d * point[3];
    return *this;
}

hkaNURBS::Point& hkaNURBS::Point::operator/=(float d) {
    point[0] = point[0] / d;
    point[1] = point[1] / d;
    point[2] = point[2] / d;
    point[3] = point[3] / d;
    return *this;
}

float hkaNURBS::Point::mag2() const {
    return Point::Dot(*this, *this);
}

hkaNURBS::Point* hkaNURBS::Point::Mul(hkaNURBS::Point* apResult, float d) const {
    hkaNURBS::Point v9;
    float v3 = point[1];
    v9.point[0] = point[0];
    float v4 = point[2];
    v9.point[3] = point[3];
    v9.point[1] = v3;
    v9.point[2] = v4;
    hkaNURBS::Point* v5 = &v9.operator*=(d);
    apResult->point[0] = v5->point[0];
    apResult->point[1] = v5->point[1];
    float v6 = v5->point[2];
    float v7 = v5->point[3];
    apResult->point[2] = v6;
    apResult->point[3] = v7;
    return apResult;
}

hkaNURBS::Point* hkaNURBS::Point::Sub(hkaNURBS::Point* apResult, const hkaNURBS::Point& a) const {
    hkaNURBS::Point v10;
    float v3 = point[1];
    v10.point[0] = point[0];
    float v4 = point[2];
    float v5 = point[3];
    v10.point[1] = v3;
    v10.point[3] = v5;
    v10.point[2] = v4;
    hkaNURBS::Point* v6 = &v10.operator-=(a);
    apResult->point[0] = v6->point[0];
    apResult->point[1] = v6->point[1];
    float v7 = v6->point[2];
    float v8 = v6->point[3];
    apResult->point[2] = v7;
    apResult->point[3] = v8;
    return apResult;
}

hkaNURBS::Point& hkaNURBS::Point::operator+=(const hkaNURBS::Point& a) {
    point[0] = a.point[0] + point[0];
    point[1] = a.point[1] + point[1];
    point[2] = a.point[2] + point[2];
    point[3] = a.point[3] + point[3];
    return *this;
}

// KEEP: the exe's body never writes edx, and its callers keep edx live across the call (0xE54527 `add edx, 0x10`,
// 0xE5535B `mov eax, edx`); clang does not preserve edx.
hkaNURBS::Point& hkaNURBS::Point::operator=(const hkaNURBS::Point& a) {
    point[0] = a.point[0];
    point[1] = a.point[1];
    point[2] = a.point[2];
    point[3] = a.point[3];
    return *this;
}

hkaNURBS::Point* hkaNURBS::Point::Add(hkaNURBS::Point* apResult, const hkaNURBS::Point& a) const {
    hkaNURBS::Point v10;
    float v3 = point[1];
    v10.point[0] = point[0];
    float v4 = point[2];
    float v5 = point[3];
    v10.point[1] = v3;
    v10.point[3] = v5;
    v10.point[2] = v4;
    hkaNURBS::Point* v6 = &v10.operator+=(a);
    apResult->point[0] = v6->point[0];
    apResult->point[1] = v6->point[1];
    float v7 = v6->point[2];
    float v8 = v6->point[3];
    apResult->point[2] = v7;
    apResult->point[3] = v8;
    return apResult;
}

hkaNURBS::Point* hkaNURBS::Point::Div(hkaNURBS::Point* apResult, float d) const {
    hkaNURBS::Point v9;
    float v3 = point[1];
    v9.point[0] = point[0];
    float v4 = point[2];
    v9.point[3] = point[3];
    v9.point[1] = v3;
    v9.point[2] = v4;
    hkaNURBS::Point* v5 = &v9.operator/=(d);
    apResult->point[0] = v5->point[0];
    apResult->point[1] = v5->point[1];
    float v6 = v5->point[2];
    float v7 = v5->point[3];
    apResult->point[2] = v6;
    apResult->point[3] = v7;
    return apResult;
}

// KEEP: the exe's body never writes edx, and its callers keep edx live across the call (hkaNURBS::Dot 0xE535A0
// holds P in edx: 0xE535C0 `mov ecx, edx`); clang does not preserve edx.
float hkaNURBS::Point::operator[](int32_t i) const {
    return point[i];
}

float hkaNURBS::Point::Dot(const hkaNURBS::Point& P, const hkaNURBS::Point& Q) {
    double v2 = P[1];
    double v3 = v2 * Q[1];
    double v5 = P[0];
    double v6 = v3 + v5 * Q[0];
    double v8 = P[2];
    double v9 = v6 + v8 * Q[2];
    double v11 = P[3];
    return v9 + v11 * Q[3];
}

// The componentwise maximum of P and Q, built in apResult. Each component is read through operator[]: P's, then Q's,
// then the larger one's again; an unordered pair takes Q's.
hkaNURBS::Point* hkaNURBS::Point::Max(hkaNURBS::Point* apResult, const hkaNURBS::Point& P, const hkaNURBS::Point& Q) {
    ::new (apResult) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
    for (int32_t i = 0; i < 4; ++i) {
        const float fP = P.operator[](i);
        const float fQ = Q.operator[](i);
        const hkaNURBS::Point& rPick = (fQ < fP) ? P : Q;
        const float fValue = rPick.operator[](i);
        apResult->operator[](i) = fValue;
    }
    return apResult;
}

// The componentwise minimum of P and Q, built in apResult, as Max; an unordered pair takes Q's.
hkaNURBS::Point* hkaNURBS::Point::Min(hkaNURBS::Point* apResult, const hkaNURBS::Point& P, const hkaNURBS::Point& Q) {
    ::new (apResult) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
    for (int32_t i = 0; i < 4; ++i) {
        const float fP = P.operator[](i);
        const float fQ = Q.operator[](i);
        const hkaNURBS::Point& rPick = (fQ > fP) ? P : Q;
        const float fValue = rPick.operator[](i);
        apResult->operator[](i) = fValue;
    }
    return apResult;
}

float hkaNURBS::Point::Dist(const hkaNURBS::Point& a1, const hkaNURBS::Point& a2) {
    hkaNURBS::Point result;
    hkaNURBS::Point* v2 = a1.Sub(&result, a2);
    return v2->mag();
}

// The cosine of the angle between a1 and a2. The dot product and a1's length are kept as doubles and only the quotient
// is rounded to a float.
float hkaNURBS::Point::Cos(const hkaNURBS::Point& a1, const hkaNURBS::Point& a2) {
    double dDot = hkaNURBS::Point::Dot(a1, a2);
    double dMag1 = a1.mag();
    return static_cast<float>(dDot / (a2.mag() * dMag1));
}

int32_t hkaNURBS::MaxIndex(const hkArray<float, hkContainerHeapAllocator>& arr, int32_t len) {
    if (len <= 0) {
        return -1;
    }
    float* m_data = arr.m_data;
    int32_t v5 = 0;
    float lena = *arr.m_data;
    int32_t v6 = 1;
    if (len - 1 >= 4) {
        float* v7 = m_data + 3;
        do {
            if (lena < static_cast<double>(*(v7 - 2))) {
                v5 = v6;
                lena = *(v7 - 2);
            }
            if (lena < static_cast<double>(*(v7 - 1))) {
                v5 = v6 + 1;
                lena = *(v7 - 1);
            }
            if (lena < static_cast<double>(*v7)) {
                v5 = v6 + 2;
                lena = *v7;
            }
            if (lena < static_cast<double>(v7[1])) {
                v5 = v6 + 3;
                lena = v7[1];
            }
            v6 += 4;
            v7 += 4;
        }
        while (v6 < len - 3);
    }
    if (v6 < len) {
        float* v8 = &m_data[v6];
        do {
            if (lena < static_cast<double>(*v8)) {
                v5 = v6;
                lena = *v8;
            }
            ++v6;
            ++v8;
        }
        while (v6 < len);
    }
    return v5;
}

void hkaNURBS::RemoveSubtractOne(int32_t i, hkArray<int32_t, hkContainerHeapAllocator>& U, int32_t m) {
    int32_t v3 = 4 * (--U.m_size - i);
    int32_t* v4 = &U.m_data[i];
    if (v3 > 0) {
        uint32_t v5 = (static_cast<uint32_t>(v3 - 1) >> 2) + 1;
        do {
            *v4 = v4[1];
            ++v4;
            --v5;
        }
        while (v5);
    }
    for (int32_t j = i; j < m; ++j) {
        --U.m_data[j];
    }
}

hkBool* hkaNURBS::WithinTolerance(
    hkBool* apResult,
    int32_t n,
    int32_t p,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    const hkArray<float, hkContainerHeapAllocator>& U,
    int32_t m,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
    float E
) {
    hkaNURBS::Point apResult_;
    int32_t v14 = 0;
    if (m < 0) {
    LABEL_5:;
        apResult->m_bool = 1;
        return apResult;
    }
    else {
        int32_t v13 = 0;
        while (true) {
            hkaNURBS::Point v16;
            hkaNURBS::Point* apPoint = &Q.m_data[v13];
            float v9 = apPoint->point[1];
            v16.point[0] = apPoint->point[0];
            float v10 = apPoint->point[2];
            v16.point[1] = v9;
            float v11 = apPoint->point[3];
            v16.point[2] = v10;
            v16.point[3] = v11;
            float u = projectPointToCurve(n, p, &U, &P, apPoint, 100, 10, 0.0001f, 0.0001f);
            hkaNURBS::CurvePoint(&apResult_, n, p, U, P, u);
            if (static_cast<double>(E) < hkaNURBS::Point::Dist(v16, apResult_)) {
                break;
            }
            ++v13;
            if (++v14 > m) {
                goto LABEL_5;
            }
        }
        apResult->m_bool = 0;
        return apResult;
    }
}

// True when every point Q[i] lies within E[i] (plus 0.001) of the curve at its parameter ub[i].
hkBool* hkaNURBS::WithinTolerance(
    hkBool* apResult,
    int32_t n,
    int32_t p,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    const hkArray<float, hkContainerHeapAllocator>& U,
    int32_t m,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
    const hkArray<float, hkContainerHeapAllocator>& ub,
    const hkArray<float, hkContainerHeapAllocator>& E
) {
    if (m >= 0) {
        const float* pE = E.m_data;
        const float* pUb = ub.m_data;
        const hkaNURBS::Point* pQ = Q.m_data;
        for (int32_t i = 0; i <= m; ++i) {
            hkaNURBS::Point kPoint = pQ[i];
            hkaNURBS::Point kCurve;
            hkaNURBS::CurvePoint(&kCurve, n, p, U, P, pUb[i]);
            const double dDist = hkaNURBS::Point::Dist(kPoint, kCurve);
            // Adds the double 0.001 and rounds the sum to a float.
            float fTol = pE[i] + 0.0010000000474974513;
            // A NaN goes on to the next point.
            if (fTol < dDist) {
                apResult->m_bool = 0;
                return apResult;
            }
        }
    }
    apResult->m_bool = 1;
    return apResult;
}

// The overload that computes the parameters itself: hkArray<hkReal> ub(m + 1) filled by ChordLengthParameterization,
// then the fit with bProject set.
void hkaNURBS::GlobalCurveApproxErrBnd(
    int32_t m,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
    int32_t p,
    float E,
    int32_t& n,
    hkArray<float, hkContainerHeapAllocator>& U,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P
) {
    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::hkArray(int)
    FNVD_INLINED_LOCAL(hkaNURBSFloatArray, ub, std::nullopt, std::nullopt);
    ub.m_data = nullptr;
    ub.m_size = 0;
    ub.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    int32_t iCount = m + 1;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    float* pData;
    if (iCount) {
        int32_t iBytes = iCount * static_cast<int32_t>(sizeof(float));
        pData = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iCount = iBytes / static_cast<int32_t>(sizeof(float));
    }
    else {
        pData = nullptr;
    }
    ub.m_capacityAndFlags = iCount ? iCount : static_cast<int32_t>(hkArrayBase<float>::DONT_DEALLOCATE_FLAG);
    ub.m_size = iCount;
    ub.m_data = pData;
    ChordLengthParameterization(m, &Q, &ub);
    // bProject is p with only its low byte set to 1, so p's upper bytes are passed on with it, as the game does.
    const int32_t bProject = static_cast<int32_t>((static_cast<uint32_t>(p) & 0xFFFFFF00u) | 1u);
    GlobalCurveApproxErrBnd(m, Q, ub, bProject, p, E, n, U, P);

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    ub.m_size = 0;
    if ((ub.m_capacityAndFlags & hkArrayBase<float>::DONT_DEALLOCATE_FLAG) == 0) {
        pHeap->bufFree(
            ub.m_data, (ub.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(float))
        );
    }
}

// The NURBS Book's GlobalCurveApproxErrBnd (A9.10) for the parameters ub: starts from the degree-1 curve through Q,
// then raises the degree to p one step at a time, removing knots within E at each degree.
void hkaNURBS::GlobalCurveApproxErrBnd(
    int32_t m,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
    hkArray<float, hkContainerHeapAllocator>& ub,
    int32_t bProject,
    int32_t p,
    float E,
    int32_t& n,
    hkArray<float, hkContainerHeapAllocator>& U,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P
) {
    n = m;
    int32_t iKnots = m + 3;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    U._reserve(*pHeap, iKnots);
    U.m_size = iKnots;

    int32_t iPoints = n + 1;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    P._reserve(*pHeap, iPoints);
    hkaNURBS::Point* pPoint = P.m_data + P.m_size;
    for (int32_t i = iPoints - P.m_size; i > 0; --i, ++pPoint) {
        if (pPoint) {
            ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    P.m_size = iPoints;

    const float* pParams = ub.m_data;
    U.m_data[0] = pParams[0];
    U.m_data[m + 2] = pParams[m];
    for (int32_t i = 0; i <= m; ++i) {
        U.m_data[i + 1] = ub.m_data[i];
        P.m_data[i].operator=(Q.m_data[i]);
    }

    // hkArray<hkReal> ek(m + 1, 0.0f).
    FNVD_INLINED_LOCAL(hkaNURBSFloatArray, ek, std::nullopt, std::nullopt);
    int32_t iErrors = m + 1;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    float* pErrors;
    int32_t iCapacity;
    if (iErrors) {
        int32_t iBytes = iErrors * static_cast<int32_t>(sizeof(float));
        pErrors = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iErrors = iBytes / static_cast<int32_t>(sizeof(float));
        iCapacity = iErrors ? iErrors : static_cast<int32_t>(hkArrayBase<float>::DONT_DEALLOCATE_FLAG);
    }
    else {
        pErrors = nullptr;
        iCapacity = static_cast<int32_t>(hkArrayBase<float>::DONT_DEALLOCATE_FLAG);
    }
    ek.m_data = pErrors;
    ek.m_size = iErrors;
    ek.m_capacityAndFlags = iCapacity;
    for (int32_t i = 0; i < iErrors; ++i) {
        pErrors[i] = 0.0f;
    }

    // computeProjectionErrors only reads Q; its declaration takes a non-const base pointer.
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* pQ =
        const_cast<hkArray<hkaNURBS::Point, hkContainerHeapAllocator>*>(&Q);
    int32_t j = 1;
    if (p >= j) {
        int32_t iRemoved;
        for (;;) {
            iRemoved = RemoveKnotsBoundCurve(n, j, U, P, m, ub, ek, E, Q);
            if (j == p) {
                break;
            }
            n += DegreeElevateKnotVector(U, n + j + 1) + 1;
            iPoints = n + 1;
            pHeap = hkMemoryRouter::getInstance().m_heap;
            P._reserve(*pHeap, iPoints);
            pPoint = P.m_data + P.m_size;
            for (int32_t i = iPoints - P.m_size; i > 0; --i, ++pPoint) {
                if (pPoint) {
                    ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            P.m_size = iPoints;

            int32_t iNext = j + 1;
            hkBool bFit;
            LeastSquaresFit(&bFit, n, iNext, P, U, m, Q, ub, 1e-05f, E);
            if (!bFit.m_bool) {
                n += -1 - DegreeLowerKnotVector(U, n + j + 1);
                iPoints = n + 1;
                pHeap = hkMemoryRouter::getInstance().m_heap;
                P._reserve(*pHeap, iPoints);
                pPoint = P.m_data + P.m_size;
                for (int32_t i = iPoints - P.m_size; i > 0; --i, ++pPoint) {
                    if (pPoint) {
                        ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
                    }
                }
                P.m_size = iPoints;
                DegreeElevateCurve(n, j, U, P, 1);
            }
            computeProjectionErrors(m, pQ, n, iNext, &U, &P, &ek, &ub, 100, 10, 1e-06f, 1e-06f, bProject);
            j = iNext;
            if (j > p) {
                break;
            }
        }
        if (iRemoved) {
            hkBool bFit;
            LeastSquaresFit(&bFit, n, p, P, U, m, Q, ub, 1e-05f, E);
            computeProjectionErrors(m, pQ, n, p, &U, &P, &ek, &ub, 100, 10, 1e-06f, 1e-06f, bProject);
            RemoveKnotsBoundCurve(n, p, U, P, m, ub, ek, E, Q);
        }
    }

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (ek.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            ek.m_data, (ek.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(float))
        );
    }
}

// Fits through a non-const copy ub_nonconst of ub, without projecting.
// KEEP: the exe builds the hkBool bProject argument in a `push ecx` slot holding &U and clears only its low byte
// (0xE583F1, 0xE583F9), so the dword the 0xE57D60 overload passes on whole to computeProjectionErrors carries U's
// address in its upper three bytes; src cannot reproduce them (it passes 0).
void hkaNURBS::GlobalCurveApproxErrBnd(
    int32_t m,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
    const hkArray<float, hkContainerHeapAllocator>& ub,
    int32_t p,
    float E,
    int32_t& n,
    hkArray<float, hkContainerHeapAllocator>& U,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P
) {
    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::hkArray(const hkArray&)
    FNVD_INLINED_LOCAL(hkaNURBSFloatArray, ub_nonconst, std::nullopt, std::nullopt);
    ub_nonconst.m_data = nullptr;
    ub_nonconst.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<float>::DONT_DEALLOCATE_FLAG);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    if (static_cast<int32_t>(ub.m_size) > 0) {
        int32_t iBytes = static_cast<int32_t>(ub.m_size) * static_cast<int32_t>(sizeof(float));
        ub_nonconst.m_data = static_cast<float*>(pHeap->bufAlloc(iBytes));
        ub_nonconst.m_capacityAndFlags = iBytes / static_cast<int32_t>(sizeof(float));
    }
    int32_t iSize = static_cast<int32_t>(ub.m_size);
    ub_nonconst.m_size = iSize;
    for (int32_t i = 0; i < iSize; ++i) {
        ub_nonconst.m_data[i] = ub.m_data[i];
    }
    GlobalCurveApproxErrBnd(m, Q, ub_nonconst, 0, p, E, n, U, P);
    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (ub_nonconst.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            ub_nonconst.m_data,
            (ub_nonconst.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(float))
        );
    }
}

void hkaNURBS::CurvePoint(
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>& U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    int32_t mb,
    const hkArray<float, hkContainerHeapAllocator>& ub,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Cb
) {
    hkaNURBS::Point apResult_;
    int32_t mb_1 = 0;
    if (mb >= 0) {
        int32_t v8 = 0;
        do {
            hkaNURBS::Point* v9 = hkaNURBS::CurvePoint(&apResult_, n, p, U, P, ub.m_data[mb_1]);
            Cb.m_data[v8].operator=(*v9);
            ++mb_1;
            ++v8;
        }
        while (mb_1 <= mb);
    }
}

int32_t hkaNURBS::FindSpan(int32_t a1, int32_t a2, float a3, const hkArray<float, hkContainerHeapAllocator>& a4) {
    double v4 = a3;
    int32_t v5 = a1;
    float* m_data = a4.m_data;
    if (a4.m_data[a1 + 1] != a3) {
        int32_t v7 = a1 + 1;
        int32_t v8 = a2;
        int32_t v9 = a1 + 1 + a2;
        while (true) {
            v5 = v9 / 2;
            if (!(m_data[v9 / 2] > v4) && !(m_data[v5 + 1] <= v4)) {
                break;
            }
            if (!(m_data[v5] > v4)) {
                v8 = v9 / 2;
                v9 = v7 + v5;
            }
            else {
                v7 = v9 / 2;
                v9 = v5 + v8;
            }
        }
    }
    return v5;
}

// The NURBS Book's BasisFuns (A2.2): the p + 1 nonzero basis functions of degree p at u (knot span i) into N. left and
// right hold four floats each, so p is at most 3.
void hkaNURBS::BasisFuns(int32_t i, float u, int32_t p, const hkArray<float, hkContainerHeapAllocator>& U, float* N) {
    float left[4];
    float right[4];
    N[0] = 1.0f;
    for (int32_t j = 1; j <= p; ++j) {
        left[j] = u - U.m_data[i + 1 - j];
        right[j] = U.m_data[i + j] - u;
        float saved = 0.0f;
        for (int32_t r = 0; r < j; ++r) {
            float temp = N[r] / (right[r + 1] + left[j - r]);
            N[r] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        N[j] = saved;
    }
}

// The NURBS Book's DersBasisFuns (A2.3): the nonzero basis functions of degree p at u (knot span i) and their
// derivatives up to the n-th, ders[k][j] the k-th derivative of N(i - p + j). ndu holds the basis functions (upper
// triangle) and the knot differences (lower); a the two most recent rows of coefficients. left and right hold four
// floats each, so p is at most 3; every intermediate is a float.
void hkaNURBS::DersBasisFuns(
    int32_t i,
    float u,
    int32_t p,
    int32_t n,
    const hkArray<float, hkContainerHeapAllocator>& U,
    float (*ders)[4]
) {
    float left[4];
    float right[4];
    float ndu[4][4];
    float a[2][4];
    ndu[0][0] = 1.0f;
    for (int32_t j = 1; j <= p; ++j) {
        left[j] = u - U.m_data[i + 1 - j];
        right[j] = U.m_data[i + j] - u;
        float saved = 0.0f;
        for (int32_t r = 0; r < j; ++r) {
            ndu[j][r] = right[r + 1] + left[j - r];
            float temp = ndu[r][j - 1] / ndu[j][r];
            ndu[r][j] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        ndu[j][j] = saved;
    }
    for (int32_t j = 0; j <= p; ++j) {
        ders[0][j] = ndu[j][p];
    }
    for (int32_t r = 0; r <= p; ++r) {
        int32_t s1 = 0;
        int32_t s2 = 1;
        a[0][0] = 1.0f;
        for (int32_t k = 1; k <= n; ++k) {
            float d = 0.0f;
            int32_t rk = r - k;
            int32_t pk = p - k;
            if (r >= k) {
                a[s2][0] = a[s1][0] / ndu[pk + 1][rk];
                d = a[s2][0] * ndu[rk][pk];
            }
            int32_t j1 = rk >= -1 ? 1 : -rk;
            int32_t j2 = r - 1 <= pk ? k - 1 : p - r;
            for (int32_t j = j1; j <= j2; ++j) {
                a[s2][j] = (a[s1][j] - a[s1][j - 1]) / ndu[pk + 1][rk + j];
                d += a[s2][j] * ndu[rk + j][pk];
            }
            if (r <= pk) {
                a[s2][k] = -a[s1][k - 1] / ndu[pk + 1][r];
                d += a[s2][k] * ndu[r][pk];
            }
            ders[k][r] = d;
            int32_t j = s1;
            s1 = s2;
            s2 = j;
        }
    }
    // The factor p! / (p - k)! is an int, converted to a float local for each k.
    int32_t r = p;
    for (int32_t k = 1; k <= n; ++k) {
        float fR = static_cast<float>(r);
        for (int32_t j = 0; j <= p; ++j) {
            ders[k][j] *= fR;
        }
        r *= p - k;
    }
}

// The NURBS Book's OneBasisFun (A2.4): the basis function N(i, p) of the m + 1 knots U at u. N holds four floats (p at
// most 3).
// KEEP: with p < 0 and no early return (u NaN, or knots out of order) the exe returns N[0], which nothing wrote.
float hkaNURBS::OneBasisFun(
    int32_t p,
    int32_t m,
    const hkArray<float, hkContainerHeapAllocator>& U,
    int32_t i,
    float u
) {
    float N[4];
    if ((i == 0 && u == U.m_data[0]) || (i == m - p - 1 && u == U.m_data[m])) {
        return 1.0f;
    }
    if (u < U.m_data[i] || u >= U.m_data[i + p + 1]) {
        return 0.0f;
    }
    for (int32_t j = 0; j <= p; ++j) {
        if (u >= U.m_data[i + j] && u < U.m_data[i + j + 1]) {
            N[j] = 1.0f;
        }
        else {
            N[j] = 0.0f;
        }
    }
    for (int32_t k = 1; k <= p; ++k) {
        float saved;
        if (N[0] == 0.0f) {
            saved = 0.0f;
        }
        else {
            saved = ((u - U.m_data[i]) * N[0]) / (U.m_data[i + k] - U.m_data[i]);
        }
        for (int32_t j = 0; j < p - k + 1; ++j) {
            float Uleft = U.m_data[i + j + 1];
            float Uright = U.m_data[i + j + k + 1];
            if (N[j + 1] == 0.0f) {
                N[j] = saved;
                saved = 0.0f;
            }
            else {
                float temp = N[j + 1] / (Uright - Uleft);
                N[j] = saved + (Uright - u) * temp;
                saved = (u - Uleft) * temp;
            }
        }
    }
    return N[0];
}

// The NURBS Book's CurveDerivs (A3.2): CK[k], the k-th derivative of the curve (n, p, U, P) at u for k up to d; those
// above p are zero. Each zero point is a Point::Construct temporary copied in with operator=.
void hkaNURBS::CurveDerivs(
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>& U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    float u,
    int32_t d,
    hkaNURBS::Point* CK
) {
    hkaNURBS::Point kZero;
    hkaNURBS::Point kTerm;
    float nders[4][4];
    int32_t du = d < p ? d : p;
    for (int32_t k = p + 1; k <= d; ++k) {
        CK[k] = *FNVD_RECONSTRUCT(hkaNURBS::Point)(&kZero, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    int32_t span = hkaNURBS::FindSpan(n, p, u, U);
    hkaNURBS::DersBasisFuns(span, u, p, du, U, nders);
    for (int32_t k = 0; k <= du; ++k) {
        CK[k] = *FNVD_RECONSTRUCT(hkaNURBS::Point)(&kZero, 0.0f, 0.0f, 0.0f, 0.0f);
        for (int32_t j = 0; j <= p; ++j) {
            CK[k] += *mulScalarPoint(&kTerm, nders[k][j], &P.m_data[span - p + j]);
        }
    }
}

hkaNURBS::Point* hkaNURBS::CurvePoint(
    hkaNURBS::Point* apResult,
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>& U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    float u
) {
    float N[4];
    hkaNURBS::Point x0;
    int32_t Span = hkaNURBS::FindSpan(n, p, u, U);
    hkaNURBS::BasisFuns(Span, u, p, U, N);
    int32_t v7 = Span - p;
    mulScalarPoint(apResult, N[0], &P.m_data[v7]);
    for (int32_t i = 1; i <= p; ++i) {
        hkaNURBS::Point* pTerm = mulScalarPoint(&x0, N[i], &P.m_data[v7 + i]);
        apResult->operator+=(*pTerm);
    }
    return apResult;
}

int32_t hkaNURBS::Find(float a1, const hkArray<float, hkContainerHeapAllocator>& arr, int32_t a3) {
    int32_t v3 = a3;
    float* v4 = arr.m_data;
    double v5 = arr.m_data[a3];
    int32_t v6 = 0;
    int32_t v7 = a3 / 2;
    bool v8 = a1 < v5;
    bool v9 = a1 == v5;
    double v10 = a1;
    if (!v8 && !v9) {
        return a3 + 1;
    }
    if (*v4 >= v10) {
        return 0;
    }
    if (v7) {
        do {
            if (!(v4[v7] >= v10)) {
                v6 = v7;
            }
            else {
                v3 = v7;
            }
            v7 = (v3 + v6) / 2;
        }
        while (v6 != v7);
    }
    return v3;
}

int32_t hkaNURBS::MinIndex(const hkArray<float, hkContainerHeapAllocator>& arr, int32_t a2) {
    int32_t a2a;
    if (a2 <= 0) {
        return -1;
    }
    float* v4 = arr.m_data;
    int32_t v5 = 0;
    a2a = *reinterpret_cast<int32_t*>(arr.m_data);
    int32_t v6 = 1;
    if (a2 - 1 >= 4) {
        float* v7 = v4 + 3;
        do {
            if (*reinterpret_cast<float*>(&a2a) > static_cast<double>(*(v7 - 2))) {
                v5 = v6;
                a2a = *(reinterpret_cast<int32_t*>(v7) - 2);
            }
            if (*reinterpret_cast<float*>(&a2a) > static_cast<double>(*(v7 - 1))) {
                v5 = v6 + 1;
                a2a = *(reinterpret_cast<int32_t*>(v7) - 1);
            }
            if (*reinterpret_cast<float*>(&a2a) > static_cast<double>(*v7)) {
                v5 = v6 + 2;
                a2a = *reinterpret_cast<int32_t*>(v7);
            }
            if (*reinterpret_cast<float*>(&a2a) > static_cast<double>(v7[1])) {
                v5 = v6 + 3;
                a2a = *(reinterpret_cast<int32_t*>(v7) + 1);
            }
            v6 += 4;
            v7 += 4;
        }
        while (v6 < a2 - 3);
    }
    if (v6 < a2) {
        float* v8 = &v4[v6];
        do {
            if (*reinterpret_cast<float*>(&a2a) > static_cast<double>(*v8)) {
                v5 = v6;
                a2a = *reinterpret_cast<int32_t*>(v8);
            }
            ++v6;
            ++v8;
        }
        while (v6 < a2);
    }
    return v5;
}

int32_t hkaNURBS::Factorial(int32_t a1) {
    uint32_t v4[11];
    int32_t result = 0x375F00;
    v4[0] = 1;
    v4[1] = 1;
    v4[2] = 2;
    v4[3] = 6;
    v4[4] = 24;
    v4[5] = 120;
    v4[6] = 720;
    v4[7] = 0x13B0;
    v4[8] = 0x9D80;
    v4[9] = 0x58980;
    v4[10] = 0x375F00;
    if (a1 <= 0) {
        return 1;
    }
    if (a1 <= 10) {
        return v4[a1];
    }
    for (int32_t i = 11; i <= a1; ++i) {
        int32_t v3 = i;
        result *= v3;
    }
    return result;
}

int32_t hkaNURBS::Multiplicity(int32_t a1, int32_t a2, const hkArray<float, hkContainerHeapAllocator>& a3) {
    int32_t v3 = 0;
    if (a1 <= a2) {
        int32_t v4 = a1;
        float* v5 = &a3.m_data[a1];
        do {
            if (*v5 != a3.m_data[a1]) {
                break;
            }
            ++v4;
            ++v3;
            ++v5;
        }
        while (v4 <= a2);
    }
    int32_t v6 = a1 - 1;
    int32_t v7 = 1;
    if (a1 - 1 >= 0) {
        int32_t v8 = a1 - 1;
        do {
            if (a3.m_data[v6] != a3.m_data[a1]) {
                break;
            }
            ++v7;
            v6 = --v8;
        }
        while (v8 >= 0);
    }
    return v7 + v3 - 1;
}

// The number of distinct values among the first a2 + 1 knots of a1: one plus the number of steps between neighbours.
int32_t hkaNURBS::UniqueKnotCount(const hkArray<float, hkContainerHeapAllocator>& a1, int32_t a2) {
    int32_t iCount = 0;
    for (int32_t i = 0; i < a2; ++i) {
        if (a1.m_data[i + 1] != a1.m_data[i]) {
            ++iCount;
        }
    }
    return iCount + 1;
}

// The knot vector grows by one knot per unique knot: each unique knot is repeated once more, filled from the back.
int32_t hkaNURBS::DegreeElevateKnotVector(hkArray<float, hkContainerHeapAllocator>& U, int32_t m) {
    int32_t s = hkaNURBS::UniqueKnotCount(U, m);
    int32_t n = s + m;
    U.setSize(n + 1);
    int32_t j = n;
    for (int32_t i = m; i > 0; --i) {
        if (U.m_data[i - 1] != U.m_data[i]) {
            U.m_data[j--] = U.m_data[i];
        }
        U.m_data[j--] = U.m_data[i];
    }
    return s - 2;
}

float hkaNURBS::Distance3D(const hkaNURBS::Point& a1, const hkaNURBS::Point& a2) {
    hkaNURBS::Point result;
    hkaNURBS::Point* v2 = a2.Sub(&result, a1);
    return v2->mag();
}

// The NURBS Book's GetRemovalBndCurve: A5.8's knot removal test for one knot (t = 0), returning the distance instead
// of comparing it with a tolerance. temp holds the control points solved from both ends.
float hkaNURBS::GetRemovalBoundCurve(
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>& U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    float u,
    int32_t r,
    int32_t s
) {
    hkaNURBS::Point temp[7];
    hkaNURBS::Point mulI;
    hkaNURBS::Point subI;
    hkaNURBS::Point divI;
    hkaNURBS::Point mulJ;
    hkaNURBS::Point subJ;
    hkaNURBS::Point divJ;
    hkaNURBS::Point* pTemp = temp;
    int32_t iLeft = 6;
    do {
        ::new (pTemp) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        ++pTemp;
    }
    while (--iLeft >= 0);
    const hkaNURBS::Point* pPoints = P.m_data;
    int32_t first = r - p;
    int32_t last = r - s;
    int32_t off = first - 1;
    temp[0] = pPoints[off];
    temp[last + 1 - off] = pPoints[last + 1];
    int32_t i = first;
    int32_t j = last;
    int32_t ii = 1;
    int32_t jj = last - off;
    if (j - i > 0) {
        // Loads U's data once, before the loop.
        const float* pLoopKnots = U.m_data;
        do {
            float alfi = (u - pLoopKnots[i]) / (pLoopKnots[i + p + 1] - pLoopKnots[i]);
            float alfj = (u - pLoopKnots[j]) / (pLoopKnots[j + p + 1] - pLoopKnots[j]);
            float oneMinusAlfi = 1.0f - alfi;
            mulScalarPoint(&mulI, oneMinusAlfi, &temp[ii - 1]);
            temp[ii] = *pPoints[i].Sub(&subI, mulI)->Div(&divI, alfi);
            float oneMinusAlfj = 1.0f - alfj;
            mulScalarPoint(&mulJ, alfj, &temp[jj + 1]);
            temp[jj] = *pPoints[j].Sub(&subJ, mulJ)->Div(&divJ, oneMinusAlfj);
            ++i;
            ++ii;
            --j;
            --jj;
        }
        while (j - i > 0);
    }
    if (j - i < 0) {
        return hkaNURBS::Distance3D(temp[ii - 1], temp[jj + 1]);
    }
    const float* pKnots = U.m_data;
    float alfi = (u - pKnots[i]) / (pKnots[i + p + 1] - pKnots[i]);
    float oneMinusAlfi = 1.0f - alfi;
    mulScalarPoint(&mulI, oneMinusAlfi, &temp[ii - 1]);
    return hkaNURBS::Distance3D(pPoints[i], *mulScalarPoint(&mulJ, alfi, &temp[ii + 1])->Add(&subI, mulI));
}

// The NURBS Book's RemoveCurveKnot (A5.8). temp holds the control points solved from both ends; first and last move
// outward by one per removal. Returns 0 at once when num is not positive, and shrinks Pw and U by num, not t, as the
// game does.
int32_t hkaNURBS::RemoveCurveKnot(
    int32_t n,
    int32_t p,
    hkArray<float, hkContainerHeapAllocator>& U,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Pw,
    float u,
    int32_t r,
    int32_t s,
    int32_t num,
    float TOL
) {
    hkaNURBS::Point temp[7];
    hkaNURBS::Point mulI;
    hkaNURBS::Point subI;
    hkaNURBS::Point divI;
    hkaNURBS::Point mulJ;
    hkaNURBS::Point subJ;
    hkaNURBS::Point divJ;
    hkaNURBS::Point sum;
    int32_t m = n + p + 1;
    int32_t ord = p + 1;
    int32_t fout = (2 * r - p - s) / 2;
    int32_t last = r - s;
    int32_t first = r - p;
    hkaNURBS::Point* pTemp = temp;
    int32_t iLeft = 6;
    do {
        ::new (pTemp) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        ++pTemp;
    }
    while (--iLeft >= 0);
    int32_t t = 0;
    for (; t < num; ++t) {
        int32_t off = first - 1;
        // Pw's data is loaded once per removal, U's once before the solving loop.
        hkaNURBS::Point* pPoints = Pw.m_data;
        temp[0] = pPoints[off];
        temp[last + 1 - off] = pPoints[last + 1];
        int32_t i = first;
        int32_t j = last;
        int32_t ii = 1;
        int32_t jj = last - off;
        if (j - i > t) {
            const float* pLoopKnots = U.m_data;
            do {
                float alfi = (u - pLoopKnots[i]) / (pLoopKnots[i + ord + t] - pLoopKnots[i]);
                float alfj = (u - pLoopKnots[j - t]) / (pLoopKnots[j + ord] - pLoopKnots[j - t]);
                float oneMinusAlfi = 1.0f - alfi;
                mulScalarPoint(&mulI, oneMinusAlfi, &temp[ii - 1]);
                temp[ii] = *pPoints[i].Sub(&subI, mulI)->Div(&divI, alfi);
                float oneMinusAlfj = 1.0f - alfj;
                mulScalarPoint(&mulJ, alfj, &temp[jj + 1]);
                temp[jj] = *pPoints[j].Sub(&subJ, mulJ)->Div(&divJ, oneMinusAlfj);
                ++i;
                ++ii;
                --j;
                --jj;
            }
            while (j - i > t);
        }
        if (j - i < t) {
            if (!(TOL >= hkaNURBS::Distance3D(temp[ii - 1], temp[jj + 1]))) {
                break;
            }
        }
        else {
            const float* pKnots = U.m_data;
            float alfi = (u - pKnots[i]) / (pKnots[i + ord + t] - pKnots[i]);
            float oneMinusAlfi = 1.0f - alfi;
            mulScalarPoint(&mulI, oneMinusAlfi, &temp[ii - 1]);
            if (!(TOL >= hkaNURBS::Distance3D(
                             Pw.m_data[i], *mulScalarPoint(&mulJ, alfi, &temp[ii + t + 1])->Add(&sum, mulI)
                         ))) {
                break;
            }
        }
        i = first;
        j = last;
        while (j - i > t) {
            Pw.m_data[i] = temp[i - off];
            Pw.m_data[j] = temp[j - off];
            ++i;
            --j;
        }
        --first;
        ++last;
    }
    if (t == 0) {
        return 0;
    }
    for (int32_t k = r + 1; k <= m; ++k) {
        U.m_data[k - t] = U.m_data[k];
    }
    int32_t j = fout;
    int32_t i = j;
    for (int32_t k = 1; k < t; ++k) {
        if (k % 2 == 1) {
            ++i;
        }
        else {
            --j;
        }
    }
    for (int32_t k = i + 1; k <= n; ++k) {
        Pw.m_data[j] = Pw.m_data[k];
        ++j;
    }
    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::setSize
    int32_t pointCount = Pw.m_size - num;
    hkMemoryAllocator* pPointHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t pointCapacity = Pw.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK;
    if (pointCapacity < pointCount) {
        int32_t newCapacity = 2 * pointCapacity;
        if (pointCount >= newCapacity) {
            newCapacity = pointCount;
        }
        hkArrayUtil::_reserve(*pPointHeap, &Pw, newCapacity, sizeof(hkaNURBS::Point));
    }
    hkaNURBS::Point* pNew = Pw.m_data + Pw.m_size;
    for (int32_t iNew = pointCount - Pw.m_size; iNew > 0; --iNew) {
        if (pNew) {
            ::new (pNew) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        }
        ++pNew;
    }
    Pw.m_size = pointCount;
    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::setSize
    int32_t knotCount = U.m_size - num;
    hkMemoryAllocator* pKnotHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t knotCapacity = U.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (knotCapacity < knotCount) {
        int32_t newCapacity = 2 * knotCapacity;
        if (knotCount >= newCapacity) {
            newCapacity = knotCount;
        }
        hkArrayUtil::_reserve(*pKnotHeap, &U, newCapacity, sizeof(float));
    }
    U.m_size = knotCount;
    return t;
}

// The NURBS Book's RemoveKnotsBoundCurve (A9.9). Br holds the removal bound of each knot (the float 3.40282e+38f,
// 0x7F7FFFEE, where there is none); the knot of least bound is removed while the errors its removal adds to ek stay
// within E, else its bound is set to that value.
int32_t hkaNURBS::RemoveKnotsBoundCurve(
    int32_t& n,
    int32_t p,
    hkArray<float, hkContainerHeapAllocator>& U,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    int32_t mb,
    const hkArray<float, hkContainerHeapAllocator>& ub,
    hkArray<float, hkContainerHeapAllocator>& e,
    float E,
    [[maybe_unused]] const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q_JUNK
) {
    const int32_t iElemSize = static_cast<int32_t>(sizeof(float));
    int32_t nOrig = n;
    int32_t mKnots = nOrig + p + 1;
    int32_t iBounds = mKnots + 1;
    int32_t iBytes;

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::hkArray(int)
    using Br_t = hkArray<float, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(Br_t, Br, std::nullopt, std::nullopt);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iSize = iBounds;
    float* pBrData = nullptr;
    if (iSize != 0) {
        iBytes = iSize * iElemSize;
        pBrData = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / iElemSize;
    }
    Br.m_data = pBrData;
    Br.m_size = iSize;
    Br.m_capacityAndFlags = iSize;
    if (iSize == 0) {
        Br.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }
    for (int32_t i = 0; i < iSize; ++i) {
        pBrData[i] = 3.40282e+38f;
    }

    // The error each removal adds at a parameter, cleared.
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iSize = mb + 1;
    float* pNewErr = nullptr;
    if (iSize != 0) {
        iBytes = iSize * iElemSize;
        pNewErr = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / iElemSize;
    }
    int32_t iNewErrCapacity = iSize;
    if (iSize == 0) {
        iNewErrCapacity = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }
    for (int32_t i = 0; i < iSize; ++i) {
        pNewErr[i] = 0.0f;
    }

    // That error plus ek, cleared.
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iSize = mb + 1;
    float* pTotalErr = nullptr;
    if (iSize != 0) {
        iBytes = iSize * iElemSize;
        pTotalErr = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / iElemSize;
    }
    int32_t iTotalErrCapacity = iSize;
    if (iSize == 0) {
        iTotalErrCapacity = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }
    for (int32_t i = 0; i < iSize; ++i) {
        pTotalErr[i] = 0.0f;
    }

    // Two arrays of m + 1 are allocated, left uninitialised and freed unread, as the game does.
    pHeap = hkMemoryRouter::getInstance().m_heap;
    float* pSpare1 = nullptr;
    int32_t iSpare1Capacity = 0;
    if (mb + 1 != 0) {
        iBytes = (mb + 1) * iElemSize;
        pSpare1 = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSpare1Capacity = iBytes / iElemSize;
    }
    if (iSpare1Capacity == 0) {
        iSpare1Capacity = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }
    pHeap = hkMemoryRouter::getInstance().m_heap;
    float* pSpare2 = nullptr;
    int32_t iSpare2Capacity = 0;
    if (mb + 1 != 0) {
        iBytes = (mb + 1) * iElemSize;
        pSpare2 = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSpare2Capacity = iBytes / iElemSize;
    }
    if (iSpare2Capacity == 0) {
        iSpare2Capacity = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }

    ComputeBounds(n, p, U, P, Br, p + 1, mKnots - p - 1);
    int32_t iLeft = iBounds;
    int32_t r = MinIndex(Br, iLeft);
    int32_t s = Multiplicity(r, mKnots, U);
    float* pBr = &Br.m_data[r];
    while (*pBr != 3.402820018375656e+38) {
        float fAlpha;
        int32_t iFirst;
        if ((s + p) % 2 != 0) {
            int32_t k = r - (p + s + 1) / 2;
            iFirst = k + 1;
            fAlpha = (U.m_data[r] - U.m_data[k + 1]) / (U.m_data[k + p + 2] - U.m_data[k + 1]);
        }
        else {
            fAlpha = 0.0f;
            iFirst = r - (s + p) / 2;
        }
        const float* pKnots = U.m_data;
        int32_t iLo = Find(pKnots[iFirst], ub, mb);
        int32_t iHi = Find(pKnots[iFirst + p + 1], ub, mb) - 1;
        float fMaxErr = 0.0f;
        if (iLo <= iHi) {
            double dOneMinusAlpha = 1.0 - fAlpha;
            for (int32_t j = iLo; j <= iHi; ++j) {
                float fErr = OneBasisFun(p, mKnots, U, iFirst, ub.m_data[j]) * (*pBr * dOneMinusAlpha);
                pNewErr[j] = fErr;
                fErr = fErr + e.m_data[j];
                pTotalErr[j] = fErr;
                if (!(fErr < fMaxErr)) {
                    fMaxErr = fErr;
                }
            }
        }
        if (E >= fMaxErr) {
            // A copy of ek, made as its copy constructor does; its size is deliberately left unwritten, as the game
            // does.
            float* pEkCopy = nullptr;
            int32_t iEkCopyCapacity = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
            pHeap = hkMemoryRouter::getInstance().m_heap;
            if (e.m_size > 0) {
                iBytes = e.m_size * iElemSize;
                pEkCopy = static_cast<float*>(pHeap->bufAlloc(iBytes));
                iEkCopyCapacity = iBytes / iElemSize;
            }
            const float* pEk = e.m_data;
            for (int32_t j = 0; j < e.m_size; ++j) {
                pEkCopy[j] = pEk[j];
            }
            for (int32_t j = iLo; j <= iHi; ++j) {
                e.m_data[j] = pTotalErr[j];
            }
            RemoveCurveKnot(n, p, U, P, U.m_data[r], r, s, 1, 3.40282e+38f);
            --mKnots;
            --n;
            --iLeft;
            if (iLeft <= 2 * p + 2) {
                pHeap = hkMemoryRouter::getInstance().m_heap;
                if (iEkCopyCapacity >= 0) {
                    pHeap->bufFree(
                        pEkCopy, static_cast<int32_t>(iEkCopyCapacity & hkArrayBase<float>::CAPACITY_MASK) * iElemSize
                    );
                }
                break;
            }
            Remove(r, Br, iLeft);
            int32_t iLoKnot = r - p;
            if (iLoKnot <= p + 1) {
                iLoKnot = p + 1;
            }
            int32_t nNow = n;
            int32_t iHiKnot = nNow;
            if (r - s + p < nNow) {
                iHiKnot = r - s + p;
            }
            int32_t iHiUnique = UniqueKnot(iHiKnot, &U, mKnots);
            int32_t iLoUnique = UniqueKnot(iLoKnot, &U, mKnots);
            ComputeBounds(nNow, p, U, P, Br, iLoUnique, iHiUnique);
            pHeap = hkMemoryRouter::getInstance().m_heap;
            if (iEkCopyCapacity >= 0) {
                pHeap->bufFree(
                    pEkCopy, static_cast<int32_t>(iEkCopyCapacity & hkArrayBase<float>::CAPACITY_MASK) * iElemSize
                );
            }
        }
        else {
            *pBr = 3.40282e+38f;
        }
        r = MinIndex(Br, iLeft);
        s = Multiplicity(r, mKnots, U);
        pBr = &Br.m_data[r];
    }

    int32_t iRemoved = nOrig - n;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iSpare2Capacity >= 0) {
        pHeap->bufFree(pSpare2, static_cast<int32_t>(iSpare2Capacity & hkArrayBase<float>::CAPACITY_MASK) * iElemSize);
    }
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iSpare1Capacity >= 0) {
        pHeap->bufFree(pSpare1, static_cast<int32_t>(iSpare1Capacity & hkArrayBase<float>::CAPACITY_MASK) * iElemSize);
    }
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iTotalErrCapacity >= 0) {
        pHeap->bufFree(
            pTotalErr, static_cast<int32_t>(iTotalErrCapacity & hkArrayBase<float>::CAPACITY_MASK) * iElemSize
        );
    }
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iNewErrCapacity >= 0) {
        pHeap->bufFree(pNewErr, static_cast<int32_t>(iNewErrCapacity & hkArrayBase<float>::CAPACITY_MASK) * iElemSize);
    }
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (Br.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            Br.m_data, static_cast<int32_t>(Br.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * iElemSize
        );
    }
    return iRemoved;
}

// Of the parameters ub, a knot span holding one at least tol inside it gets its flag set; the knot vector is well posed
// when the flag of every distinct span is set. The flags are n + 1 cleared elements.
hkBool* hkaNURBS::IsWellPosed(
    hkBool* apResult,
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>& U,
    int32_t m,
    const hkArray<float, hkContainerHeapAllocator>& ub,
    float tol
) {
    int32_t iLast = n + p + 1;
    int32_t iFlagBytes = n + 1;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    hkBool* pFlags = nullptr;
    if (iFlagBytes != 0) {
        pFlags = static_cast<hkBool*>(pHeap->bufAlloc(iFlagBytes));
    }
    int32_t iFlagCapacity = iFlagBytes;
    if (iFlagBytes == 0) {
        iFlagCapacity = hkArrayBase<hkBool>::DONT_DEALLOCATE_FLAG;
    }
    if (iFlagBytes > 0) {
        std::memset(pFlags, 0, static_cast<size_t>(iFlagBytes));
    }
    for (int32_t i = 0; i <= m; ++i) {
        const float* pU = &ub.m_data[i];
        int32_t iSpan = FindSpan(n, p, *pU, U);
        const float* pKnots = U.m_data;
        float fLo = pKnots[iSpan] + tol;
        int32_t iNext = iLast;
        if (iSpan != iLast) {
            iNext = iSpan + 1;
        }
        if (fLo <= *pU) {
            float fHi = pKnots[iNext] - tol;
            if (fHi >= *pU) {
                pFlags[iSpan - p].m_bool = 1;
            }
        }
    }
    char cWellPosed = 1;
    for (int32_t i = 0; i <= n; ++i) {
        // isKnotDistinct's hkBool result is built in m's storage, which is dead after the first loop, so the callee
        // sees m's bytes beside the one it writes. This is deliberate, as the game does.
        if (*isKnotDistinct(reinterpret_cast<uint8_t*>(&m), p + i, &U, iLast) && !pFlags[i].m_bool) {
            cWellPosed = 0;
            break;
        }
    }
    apResult->m_bool = cWellPosed;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iFlagCapacity >= 0) {
        pHeap->bufFree(pFlags, static_cast<int32_t>(iFlagCapacity & hkArrayBase<hkBool>::CAPACITY_MASK));
    }
    return apResult;
}

// N holds the basis functions N[i][j] = N(j, p)(ub[i]) of the inner points and control points, R the points less their
// first and last control points' share, Rm the right-hand side N^T R, solved for X by conjugate gradients
// (hkaCGSolver<float>::Solve) starting from the inner points.
hkBool* hkaNURBS::LeastSquaresFit(
    hkBool* apResult,
    int32_t n,
    int32_t p,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    const hkArray<float, hkContainerHeapAllocator>& U,
    int32_t m,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
    const hkArray<float, hkContainerHeapAllocator>& ub,
    float tol,
    float E
) {
    const int32_t iElemSize = static_cast<int32_t>(sizeof(hkaNURBS::Point));
    if (n > m) {
        apResult->m_bool = 0;
        return apResult;
    }
    hkaSparseLSQMatrix<float> N(m - 1, n - 1);
    int32_t iLast = n + p + 1;
    for (int32_t i = 1; i < m; ++i) {
        for (int32_t j = 1; j < n; ++j) {
            float fBasis = OneBasisFun(p, iLast, U, j, ub.m_data[i]);
            N.Set(i - 1, j - 1, fBasis);
        }
    }
    hkaMatrix<float> Rm(n - 1, 4, 0.0f);

    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iCount = m + 1;
    hkaNURBS::Point* pR = nullptr;
    if (iCount != 0) {
        int32_t iBytes = iCount * iElemSize;
        pR = static_cast<hkaNURBS::Point*>(pHeap->bufAlloc(iBytes));
        iCount = iBytes / iElemSize;
    }
    int32_t iRCapacity = iCount;
    if (iCount == 0) {
        iRCapacity = hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG;
    }
    for (hkaNURBS::Point* pPoint = pR; iCount > 0; ++pPoint, --iCount) {
        if (pPoint) {
            ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // R[0] = R[m] = 0; R[k] = Q[k] - N(0, p)(ub[k]) Q[0] - N(n, p)(ub[k]) Q[m].
    hkaNURBS::Point kSum;
    hkaNURBS::Point* pZero = FNVD_RECONSTRUCT(hkaNURBS::Point)(&kSum, 0.0f, 0.0f, 0.0f, 0.0f);
    hkaNURBS::Point& rLastR = pR[m] = *pZero;
    pR[0] = rLastR;
    for (int32_t k = 1; k < m; ++k) {
        hkaNURBS::Point kFirst;
        hkaNURBS::Point kDiff;
        hkaNURBS::Point kRk;
        const hkaNURBS::Point* pQ = Q.m_data;
        const float* pu = &ub.m_data[k];
        float fLastBasis = OneBasisFun(p, iLast, U, n, *pu);
        hkaNURBS::Point* pLast = mulScalarPoint(&kSum, fLastBasis, &pQ[m]);
        float fFirstBasis = OneBasisFun(p, iLast, U, 0, *pu);
        hkaNURBS::Point* pFirst = mulScalarPoint(&kFirst, fFirstBasis, pQ);
        hkaNURBS::Point* pDiff = pQ[k].Sub(&kDiff, *pFirst);
        hkaNURBS::Point* pRk = pDiff->Sub(&kRk, *pLast);
        pR[k] = *pRk;
    }

    // Row j - 1 of Rm: the sum over k of N(j, p)(ub[k]) R[k].
    for (int32_t j = 1; j < n; ++j) {
        kSum.~Point();
        ::new (&kSum) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        if (m > 1) {
            const float* pu = ub.m_data + 1;
            const hkaNURBS::Point* pRk = pR + 1;
            for (int32_t k = m - 1; k != 0; --k) {
                hkaNURBS::Point kTerm;
                float fBasis = OneBasisFun(p, iLast, U, j, *pu);
                hkaNURBS::Point* pTerm = mulScalarPoint(&kTerm, fBasis, pRk);
                kSum += *pTerm;
                ++pRk;
                ++pu;
            }
        }
        for (int32_t i = 0; i < 4; ++i) {
            float* pComponent = &kSum.operator[](i);
            float& rElement = Rm(j - 1, i);
            rElement = *pComponent;
        }
    }

    hkBool bWellPosed;
    if (!*IsWellPosed(&bWellPosed, n, p, U, ub.m_size - 1, ub, tol)) {
        apResult->m_bool = 0;
        pHeap = hkMemoryRouter::getInstance().m_heap;
        if (iRCapacity >= 0) {
            pHeap->bufFree(pR, (iRCapacity & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK) * iElemSize);
        }
        return apResult;
    }

    {
        // X starts as the inner points Q[1..n-1].
        hkaMatrix<float> X(Rm.m_m, Rm.m_n);
        for (int32_t i = 0; i < X.m_m; ++i) {
            // Q[i + 1]'s address is taken once per row.
            const hkaNURBS::Point& rQ = Q.m_data[i + 1];
            for (int32_t j = 0; j < X.m_n; ++j) {
                float fValue = rQ[j];
                X(i, j) = fValue;
            }
        }
        hkBool bSolved;
        hkaCGSolver<float>::Solve(&bSolved, N, X, Rm, 50, tol);
        if (!bSolved.m_bool) {
            apResult->m_bool = 0;
        }
        else {
            // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::hkArray()
            using Pn_t = hkArray<hkaNURBS::Point, hkContainerHeapAllocator>;
            FNVD_INLINED_LOCAL(Pn_t, Pn, std::nullopt, std::nullopt);
            Pn.m_data = nullptr;
            Pn.m_size = 0;
            Pn.m_capacityAndFlags = hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG;
            int32_t iSize = n + 1;
            pHeap = hkMemoryRouter::getInstance().m_heap;
            int32_t iCapacity = Pn.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK;
            if (iCapacity < iSize) {
                int32_t iNewCapacity = 2 * iCapacity;
                if (iSize >= iNewCapacity) {
                    iNewCapacity = iSize;
                }
                hkArrayUtil::_reserve(pHeap, &Pn, iNewCapacity, iElemSize);
            }
            int32_t iNew = iSize - Pn.m_size;
            for (hkaNURBS::Point* pPoint = &Pn.m_data[Pn.m_size]; iNew > 0; ++pPoint, --iNew) {
                if (pPoint) {
                    ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            Pn.m_size = iSize;
            Pn.m_data[0] = Q.m_data[0];
            Pn.m_data[n] = Q.m_data[m];
            for (int32_t j = 1; j < n; ++j) {
                for (int32_t i = 0; i < 4; ++i) {
                    float& rElement = X(j - 1, i);
                    float* pComponent = &Pn.m_data[j].operator[](i);
                    *pComponent = rElement;
                }
            }
            if (*isWithinErrorBound(&bSolved, n, p, &Pn, &U, m, &Q, &ub, E)) {
                P.swap(Pn);
                apResult->m_bool = 1;
            }
            else {
                apResult->m_bool = 0;
            }
            // Pn.clearAndDeallocate().
            pHeap = hkMemoryRouter::getInstance().m_heap;
            Pn.m_size = 0;
            if (Pn.m_capacityAndFlags >= 0) {
                pHeap->bufFree(
                    Pn.m_data, (Pn.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK) * iElemSize
                );
            }
            Pn.m_data = nullptr;
            Pn.m_capacityAndFlags = hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG;
        }
    }
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (iRCapacity >= 0) {
        pHeap->bufFree(pR, (iRCapacity & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK) * iElemSize);
    }
    return apResult;
}

// Removes knot i from U, copying the later knots down.
void hkaNURBS::Remove(int32_t i, hkArray<float, hkContainerHeapAllocator>& U, int32_t m) {
    U.removeAtAndCopy(i);
}

// B[lo..hi] set to HK_REAL_MAX, then each interior knot's removal bound stored at its index.
void hkaNURBS::ComputeBounds(
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>& U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
    hkArray<float, hkContainerHeapAllocator>& B,
    int32_t lo,
    int32_t hi
) {
    int32_t m = n + p + 1;
    for (int32_t j = lo; j <= hi; ++j) {
        B.m_data[j] = 3.40282e+38f;
    }
    for (int32_t i = lo; i <= hi; ++i) {
        uint8_t ucInterior;
        if (*isKnotDistinct(&ucInterior, i, &U, m)) {
            float* pU = &U.m_data[i];
            float* pB = &B.m_data[i];
            int32_t s = hkaNURBS::Multiplicity(i, m, U);
            *pB = hkaNURBS::GetRemovalBoundCurve(n, p, U, P, *pU, i, s);
        }
    }
}

// The NURBS Book's DegreeElevateCurve (A5.9): raises the degree p of the curve (n, U, Pw) by t, U and Pw taking the new
// knots Uh and control points Qw. Returns at once when t is 0, sizes Uh and Qw up front from the count of unique knots
// (nh = n + t * (unique - 1), stored to n at the end) and keeps no mh. Its Bezier work arrays hold four points each and
// bezalfs four rows of four, so p + t is at most 3. Each coefficient (inv, numer, den, bet, alf, gam) is rounded to a
// float.
void hkaNURBS::DegreeElevateCurve(
    int32_t& n,
    int32_t p,
    hkArray<float, hkContainerHeapAllocator>& U,
    hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Pw,
    int32_t t
) {
    const int32_t iKnotSize = static_cast<int32_t>(sizeof(float));
    const int32_t iPointSize = static_cast<int32_t>(sizeof(hkaNURBS::Point));
    int32_t nOld = n;
    int32_t m = nOld + p + 1;
    if (t == 0) {
        return;
    }
    hkaNURBS::Point bpts[4];
    hkaNURBS::Point ebpts[4];
    hkaNURBS::Point Nextbpts[4];
    hkaNURBS::Point kZero;
    hkaNURBS::Point kMul;
    hkaNURBS::Point kMulPrev;
    hkaNURBS::Point kSum;
    float bezalfs[4][4];
    float alfs[4];
    for (int32_t i = 0; i < 4; ++i) {
        bpts[i].~Point();
        ::new (&bpts[i]) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
    }
    for (int32_t i = 0; i < 4; ++i) {
        ebpts[i].~Point();
        ::new (&ebpts[i]) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
    }
    for (int32_t i = 0; i < 4; ++i) {
        Nextbpts[i].~Point();
        ::new (&Nextbpts[i]) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
    }
    int32_t nh = nOld + t * (UniqueKnotCount(U, m) - 1);

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::hkArray(int)
    using Uh_t = hkArray<float, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(Uh_t, Uh, std::nullopt, std::nullopt);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iSize = nh + p + t + 2;
    float* pKnots = nullptr;
    if (iSize != 0) {
        int32_t iBytes = iSize * iKnotSize;
        pKnots = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / iKnotSize;
    }
    Uh.m_data = pKnots;
    Uh.m_size = iSize;
    Uh.m_capacityAndFlags = iSize;
    if (iSize == 0) {
        Uh.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }

    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::hkArray(int)
    using Qw_t = hkArray<hkaNURBS::Point, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(Qw_t, Qw, std::nullopt, std::nullopt);
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iSize = nh + 1;
    hkaNURBS::Point* pPoints = nullptr;
    if (iSize != 0) {
        int32_t iBytes = iSize * iPointSize;
        pPoints = static_cast<hkaNURBS::Point*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / iPointSize;
    }
    Qw.m_data = pPoints;
    Qw.m_size = iSize;
    Qw.m_capacityAndFlags = iSize;
    if (iSize == 0) {
        Qw.m_capacityAndFlags = hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG;
    }
    for (hkaNURBS::Point* pPoint = pPoints; iSize > 0; ++pPoint, --iSize) {
        if (pPoint) {
            ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // The Bezier degree elevation coefficients.
    int32_t ph = t + p;
    int32_t ph2 = ph / 2;
    bezalfs[ph][p] = 1.0f;
    bezalfs[0][0] = 1.0f;
    for (int32_t i = 1; i <= ph2; ++i) {
        float inv = 1.0 / static_cast<double>(Bin(ph, i));
        int32_t mpi = p < i ? p : i;
        for (int32_t j = i - t < 0 ? 0 : i - t; j <= mpi; ++j) {
            double fProduct = static_cast<double>(Bin(p, j)) * inv;
            bezalfs[i][j] = static_cast<float>(fProduct * Bin(t, i - j));
        }
    }
    for (int32_t i = ph2 + 1; i <= ph - 1; ++i) {
        int32_t mpi = p < i ? p : i;
        for (int32_t j = i - t < 0 ? 0 : i - t; j <= mpi; ++j) {
            bezalfs[i][j] = bezalfs[ph - i][p - j];
        }
    }

    int32_t kind = ph + 1;
    int32_t r = -1;
    int32_t a = p;
    int32_t b = p + 1;
    int32_t cind = 1;
    float ua = U.m_data[0];
    float ub;
    Qw.m_data[0] = Pw.m_data[0];
    for (int32_t i = 0; i <= ph; ++i) {
        Uh.m_data[i] = ua;
    }
    // The first Bezier segment.
    for (int32_t i = 0; i <= p; ++i) {
        bpts[i] = Pw.m_data[i];
    }
    while (b < m) {
        int32_t i = b;
        while (b < m && U.m_data[b] == U.m_data[b + 1]) {
            ++b;
        }
        int32_t mul = b - i + 1;
        ub = U.m_data[b];
        int32_t oldr = r;
        r = p - mul;
        int32_t lbz = oldr > 0 ? (oldr + 2) / 2 : 1;
        int32_t rbz = r > 0 ? ph - (r + 1) / 2 : ph;
        // Insert knot ub r times to get the Bezier segment.
        if (r > 0) {
            float numer = ub - ua;
            for (int32_t k = p; k > mul; --k) {
                alfs[k - mul - 1] = numer / (U.m_data[a + k] - ua);
            }
            for (int32_t j = 1; j <= r; ++j) {
                int32_t save = r - j;
                int32_t s = mul + j;
                for (int32_t k = p; k >= s; --k) {
                    mulScalarPoint(&kMulPrev, 1.0f - alfs[k - s], &bpts[k - 1]);
                    bpts[k] = *mulScalarPoint(&kMul, alfs[k - s], &bpts[k])->Add(&kSum, kMulPrev);
                }
                Nextbpts[save] = bpts[p];
            }
        }
        // Degree elevate the Bezier segment; only points lbz..ph are used below.
        for (int32_t iE = lbz; iE <= ph; ++iE) {
            ebpts[iE] = *FNVD_RECONSTRUCT(hkaNURBS::Point)(&kZero, 0.0f, 0.0f, 0.0f, 0.0f);
            int32_t mpi = p < iE ? p : iE;
            for (int32_t j = iE - t < 0 ? 0 : iE - t; j <= mpi; ++j) {
                ebpts[iE] = *ebpts[iE].Add(&kSum, *mulScalarPoint(&kMul, bezalfs[iE][j], &bpts[j]));
            }
        }
        // Remove knot ua oldr times.
        if (oldr > 1) {
            int32_t first = kind - 2;
            int32_t last = kind;
            float den = ub - ua;
            float bet = (ub - Uh.m_data[kind - 1]) / den;
            for (int32_t tr = 1; tr < oldr; ++tr) {
                int32_t iR = first;
                int32_t j = last;
                int32_t kj = j - kind + 1;
                while (j - iR > tr) {
                    if (iR < cind) {
                        float alf = (ub - Uh.m_data[iR]) / (ua - Uh.m_data[iR]);
                        mulScalarPoint(&kMulPrev, 1.0f - alf, &Qw.m_data[iR - 1]);
                        Qw.m_data[iR] = *mulScalarPoint(&kMul, alf, &Qw.m_data[iR])->Add(&kSum, kMulPrev);
                    }
                    if (j >= lbz) {
                        if (j - tr <= kind - ph + oldr) {
                            float gam = (ub - Uh.m_data[j - tr]) / den;
                            mulScalarPoint(&kMulPrev, 1.0f - gam, &ebpts[kj + 1]);
                            ebpts[kj] = *mulScalarPoint(&kMul, gam, &ebpts[kj])->Add(&kSum, kMulPrev);
                        }
                        else {
                            mulScalarPoint(&kMulPrev, 1.0f - bet, &ebpts[kj + 1]);
                            ebpts[kj] = *mulScalarPoint(&kMul, bet, &ebpts[kj])->Add(&kSum, kMulPrev);
                        }
                    }
                    ++iR;
                    --j;
                    --kj;
                }
                --first;
                ++last;
            }
        }
        // Load the knot ua.
        if (a != p) {
            for (int32_t iK = 0; iK < ph - oldr; ++iK) {
                Uh.m_data[kind] = ua;
                ++kind;
            }
        }
        // Load the control points into Qw.
        for (int32_t j = lbz; j <= rbz; ++j) {
            Qw.m_data[cind] = ebpts[j];
            ++cind;
        }
        if (b < m) {
            // Set up for the next pass.
            for (int32_t j = 0; j < r; ++j) {
                bpts[j] = Nextbpts[j];
            }
            for (int32_t j = r; j <= p; ++j) {
                bpts[j] = Pw.m_data[b - p + j];
            }
            a = b;
            ++b;
            ua = ub;
        }
        else {
            // The end knot.
            for (int32_t iK = 0; iK <= ph; ++iK) {
                Uh.m_data[kind + iK] = ub;
            }
        }
    }
    Pw.swap(Qw);
    Uh.swap(U);
    n = nh;
    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (Qw.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            Qw.m_data,
            static_cast<int32_t>(Qw.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK) * iPointSize
        );
    }
    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if (Uh.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            Uh.m_data, static_cast<int32_t>(Uh.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * iKnotSize
        );
    }
}

// Keeps one copy of each repeated knot of U[0..m) and U[m] last; returns how many knots were dropped.
int32_t hkaNURBS::DegreeLowerKnotVector(hkArray<float, hkContainerHeapAllocator>& U, int32_t m) {
    int32_t j = 0;
    for (int32_t i = 0; i < m; ++i) {
        if (U.m_data[i + 1] == U.m_data[i]) {
            U.m_data[j++] = U.m_data[i];
        }
    }
    U.m_data[j] = U.m_data[m];
    U.setSize(j + 1);
    return m - j - 1;
}

hkaNURBS::Point* mulScalarPoint(hkaNURBS::Point* apResult, float d, const hkaNURBS::Point* a) {
    a->Mul(apResult, d);
    return apResult;
}

hkaNURBS::Point* divScalarPoint(hkaNURBS::Point* apResult, float d, const hkaNURBS::Point* a) {
    a->Div(apResult, d);
    return apResult;
}

uint8_t* isKnotDistinct(uint8_t* a1, int32_t a2, const hkArray<float, hkContainerHeapAllocator>* a3, int32_t a4) {
    // Unordered knots count as different.
    if (a2 == a4 || a3->m_data[a2 + 1] != a3->m_data[a2]) {
        *a1 = 1;
        return a1;
    }
    else {
        *a1 = 0;
        return a1;
    }
}

// Point inversion: the parameter of the curve (n, p, U, P) nearest apPoint. iSamples + 1 evenly spaced samples pick the
// start, then up to iMaxIterations Newton steps refine it, clamped to the knot range; it stops when the point is within
// fEps1, the cosine within fEps2, or the step too small.
float projectPointToCurve(
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>* U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* P,
    const hkaNURBS::Point* apPoint,
    int32_t iSamples,
    int32_t iMaxIterations,
    float fEps1,
    float fEps2
) {
    const float* pKnots = U->m_data;
    float u = *pKnots;
    float fMinDist = 3.40282e+38f;
    int32_t iLast = n + p + 1;
    if (iSamples >= 0) {
        double dRange = pKnots[iLast] - *pKnots;
        float fSamples = static_cast<float>(iSamples);
        int32_t i = 0;
        do {
            float t = static_cast<double>(i) / fSamples * dRange + *pKnots;
            hkaNURBS::Point kPoint;
            hkaNURBS::CurvePoint(&kPoint, n, p, *U, *P, t);
            float fDist = hkaNURBS::Point::Dist(kPoint, *apPoint);
            if (fMinDist > fDist) {
                fMinDist = fDist;
                u = t;
            }
            ++i;
        }
        while (i <= iSamples);
    }
    for (int32_t iIteration = 0; iIteration < iMaxIterations; ++iIteration) {
        hkaNURBS::Point aDerivs[3];
        for (int32_t k = 0; k < 3; ++k) {
            aDerivs[k].~Point();
            ::new (&aDerivs[k]) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        }
        hkaNURBS::CurveDerivs(n, p, *U, *P, u, 2, aDerivs);
        aDerivs[0] -= *apPoint;
        bool bPointClose = fEps1 >= aDerivs[0].mag();
        bool bZeroCos = fEps2 >= hkaNURBS::Point::Cos(aDerivs[1], aDerivs[0]);
        if (bPointClose && bZeroCos) {
            return u;
        }
        float uPrev = u;
        double dNum = hkaNURBS::Point::Dot(aDerivs[1], aDerivs[0]);
        float fDenom = hkaNURBS::Point::Dot(aDerivs[2], aDerivs[0]) + aDerivs[1].mag2();
        u = u - dNum / fDenom;
        float fLow = *pKnots;
        if (!(fLow < u)) {
            u = fLow;
        }
        float fHigh = pKnots[iLast];
        if (!(fHigh > u)) {
            u = fHigh;
        }
        double dSpeed = aDerivs[1].mag();
        float fStep = u - uPrev;
        float fAbsStep = std::fabs(fStep);
        bool bNoChange = fEps1 >= dSpeed * fAbsStep;
        if (bPointClose || bZeroCos || bNoChange) {
            return u;
        }
    }
    return u;
}

int32_t UniqueKnot(int32_t a1, const hkArray<float, hkContainerHeapAllocator>* a2, int32_t a3) {
    const hkArray<float, hkContainerHeapAllocator>* v3 = a2;
    int32_t v4 = a1;
    int32_t v5 = a3;
    if (!(*::isKnotDistinct(static_cast<uint8_t*>(static_cast<void*>(&a1)), a1, a2, a3))) {
        do {
            ++v4;
        }
        while (!(*::isKnotDistinct(static_cast<uint8_t*>(static_cast<void*>(&a1)), v4, v3, v5)));
    }
    return v4;
}

// Projects each of the m + 1 data points Q onto the curve (n, p, U, P) through projectPointToCurve when bProject is
// set, else takes its parameter from pParams; stores the parameter in pParams and the point's distance to the curve in
// pErrors.
void computeProjectionErrors(
    int32_t m,
    hkArrayBase<hkaNURBS::Point>* Q,
    int32_t n,
    int32_t p,
    const hkArray<float, hkContainerHeapAllocator>* U,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* P,
    hkArray<float, hkContainerHeapAllocator>* pErrors,
    hkArray<float, hkContainerHeapAllocator>* pParams,
    int32_t iSamples,
    int32_t iMaxIterations,
    float fEps1,
    float fEps2,
    int32_t bProject
) {
    for (int32_t i = 0; i <= m; ++i) {
        float u;
        if (static_cast<uint8_t>(bProject)) {
            u = projectPointToCurve(n, p, U, P, &Q->m_data[i], iSamples, iMaxIterations, fEps1, fEps2);
        }
        else {
            u = pParams->m_data[i];
        }
        hkaNURBS::Point kPoint;
        hkaNURBS::CurvePoint(&kPoint, n, p, *U, *P, u);
        pParams->m_data[i] = u;
        hkaNURBS::Point kDifference;
        pErrors->m_data[i] = Q->m_data[i].Sub(&kDifference, kPoint)->mag();
    }
}

hkBool* isWithinErrorBound(
    hkBool* a1,
    int32_t a2,
    int32_t a3,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* a4,
    const hkArray<float, hkContainerHeapAllocator>* a5,
    int32_t a6,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* a7,
    const hkArray<float, hkContainerHeapAllocator>* a8,
    float a9
) {
    a1->m_bool = 1;
    if (a6 < 0) {
        return a1;
    }
    const hkaNURBS::Point* v9 = a7->m_data;
    const float* v10 = a8->m_data;
    int32_t v11 = a6 + 1;
    do {
        hkaNURBS::Point v17;
        hkaNURBS::Point v18;
        float v12 = v9->point[0];
        float v13 = v9->point[2];
        float v16 = *v10;
        v17.point[1] = v9->point[1];
        v17.point[0] = v12;
        float v14 = v9->point[3];
        v17.point[2] = v13;
        v17.point[3] = v14;
        hkaNURBS::CurvePoint(&v18, a2, a3, *a5, *a4, v16);
        if (hkaNURBS::Point::Dist(v17, v18) > a9 * 1.100000023841858) {
            a1->m_bool = 0;
        }
        ++v9;
        ++v10;
        --v11;
    }
    while (v11);
    return a1;
}

// The chord-length parameters of the points arg4 into a3 (a1 + 1 floats from 0 to 1).
int32_t ChordLengthParameterization(
    int32_t a1,
    const hkArrayBase<hkaNURBS::Point>* arg4,
    hkArray<float, hkContainerHeapAllocator>* a3
) {
    double v15;
    hkaNURBS::Point result;
    int32_t v4 = a1 + 1;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v7 = a3->m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (v7 < a1 + 1) {
        int32_t v8 = 2 * v7;
        if (v4 >= v8) {
            v8 = a1 + 1;
        }
        hkArrayUtil::_reserve(*m_heap, a3, v8, static_cast<int32_t>(sizeof(float)));
    }
    float* m_data = a3->begin();
    a3->m_size = v4;
    m_data[0] = 0.0f;
    int32_t v10 = 1;
    if (a1 >= 1) {
        const hkaNURBS::Point* a1a = arg4->begin() + 1;
        const hkaNURBS::Point* a2 = arg4->begin();
        do {
            float* v11 = &a3->m_data[v10];
            hkaNURBS::Point* v12 = a1a++->Sub(&result, *a2++);
            *v11 = v12->mag() + *(v11 - 1);
            ++v10;
        }
        while (v10 <= a1);
    }
    int32_t v13 = 1;
    if (a1 - 1 >= 4) {
        do {
            double v14 = a3->m_data[v13];
            v13 += 4;
            a3->m_data[v13 - 4] = v14 / a3->m_data[a1];
            a3->m_data[v13 - 3] = a3->m_data[v13 - 3] / a3->m_data[a1];
            a3->m_data[v13 - 2] = a3->m_data[v13 - 2] / a3->m_data[a1];
            a3->m_data[v13 - 1] = a3->m_data[v13 - 1] / a3->m_data[a1];
        }
        while (v13 < a1 - 3);
    }
    for (; v13 < a1; a3->m_data[v13 - 1] = v15 / a3->m_data[a1]) {
        v15 = a3->m_data[v13++];
    }
    a3->m_data[a1] = 1.0f;
    return v13;
}

// The body of ApproximateNURBSCurve (hkaCompression.h): the samples are copied into Q and their times into ub,
// GlobalCurveApproxErrBnd fits the curve of the given degree within tolerance (n, the knots U and the control points
// P), and the knots and control points are copied out. Each element count is the one bufAlloc gives back; the copy
// loops run numSamples times.
void approximateNURBSCurveImpl(
    int32_t numSamples,
    const float* sampleTimes,
    const hkVector4* sampleData,
    int32_t degree,
    float tolerance,
    int32_t* numKnotsOut,
    float* knotsOut,
    int32_t* numPointsOut,
    hkVector4* pointsOut
) {
    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::hkArray(int)
    using Q_t = hkArray<hkaNURBS::Point, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(Q_t, Q, std::nullopt, std::nullopt);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iSize = numSamples;
    hkaNURBS::Point* pPoints = nullptr;
    if (iSize != 0) {
        int32_t iBytes = iSize * static_cast<int32_t>(sizeof(hkaNURBS::Point));
        pPoints = static_cast<hkaNURBS::Point*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / static_cast<int32_t>(sizeof(hkaNURBS::Point));
    }
    Q.m_data = pPoints;
    Q.m_size = iSize;
    Q.m_capacityAndFlags = iSize;
    if (iSize == 0) {
        Q.m_capacityAndFlags = hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG;
    }
    for (hkaNURBS::Point* pPoint = pPoints; iSize > 0; ++pPoint, --iSize) {
        if (pPoint) {
            ::new (pPoint) hkaNURBS::Point(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::hkArray(int)
    using ub_t = hkArray<float, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(ub_t, ub, std::nullopt, std::nullopt);
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iSize = numSamples;
    float* pTimes = nullptr;
    if (iSize != 0) {
        int32_t iBytes = iSize * static_cast<int32_t>(sizeof(float));
        pTimes = static_cast<float*>(pHeap->bufAlloc(iBytes));
        iSize = iBytes / static_cast<int32_t>(sizeof(float));
    }
    ub.m_data = pTimes;
    ub.m_size = iSize;
    ub.m_capacityAndFlags = iSize;
    if (iSize == 0) {
        ub.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    }

    for (int32_t i = 0; i < numSamples; ++i) {
        ub.m_data[i] = sampleTimes[i];
        hkaNURBS::Point& rPoint = Q.m_data[i];
        rPoint.operator[](0) = sampleData[i](0);
        rPoint.operator[](1) = sampleData[i](1);
        rPoint.operator[](2) = sampleData[i](2);
        rPoint.operator[](3) = sampleData[i](3);
    }

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::hkArray()
    using U_t = hkArray<float, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(U_t, U, std::nullopt, std::nullopt);
    U.m_data = nullptr;
    U.m_size = 0;
    U.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::hkArray()
    using P_t = hkArray<hkaNURBS::Point, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(P_t, P, std::nullopt, std::nullopt);
    P.m_data = nullptr;
    P.m_size = 0;
    P.m_capacityAndFlags = hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG;
    int32_t n;
    hkaNURBS::GlobalCurveApproxErrBnd(numSamples - 1, Q, ub, degree, tolerance, n, U, P);

    *numPointsOut = n + 1;
    *numKnotsOut = n + degree + 2;
    for (int32_t i = 0; i < *numPointsOut; ++i) {
        hkaNURBS::Point& rPoint = P.m_data[i];
        pointsOut[i](0) = rPoint.operator[](0);
        pointsOut[i](1) = rPoint.operator[](1);
        pointsOut[i](2) = rPoint.operator[](2);
        pointsOut[i](3) = rPoint.operator[](3);
    }
    for (int32_t i = 0; i < *numKnotsOut; ++i) {
        knotsOut[i] = U.m_data[i];
    }

    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if ((U.m_capacityAndFlags & hkArrayBase<float>::DONT_DEALLOCATE_FLAG) == 0) {
        pHeap->bufFree(
            U.m_data, (U.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(float))
        );
    }
    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if ((P.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG) == 0) {
        pHeap->bufFree(
            P.m_data,
            (P.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkaNURBS::Point))
        );
    }
    // EXE-INLINED none: hkArray<float, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if ((ub.m_capacityAndFlags & hkArrayBase<float>::DONT_DEALLOCATE_FLAG) == 0) {
        pHeap->bufFree(
            ub.m_data, (ub.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(float))
        );
    }
    // EXE-INLINED none: hkArray<hkaNURBS::Point, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    if ((Q.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::DONT_DEALLOCATE_FLAG) == 0) {
        pHeap->bufFree(
            Q.m_data,
            (Q.m_capacityAndFlags & hkArrayBase<hkaNURBS::Point>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkaNURBS::Point))
        );
    }
}

void ApproximateNURBSCurve(
    int32_t numSamples,
    const float* sampleTimes,
    const hkVector4* sampleData,
    int32_t degree,
    float tolerance,
    int32_t* numKnotsOut,
    float* knotsOut,
    int32_t* numPointsOut,
    hkVector4* pointsOut
) {
    approximateNURBSCurveImpl(
        numSamples, sampleTimes, sampleData, degree, tolerance, numKnotsOut, knotsOut, numPointsOut, pointsOut
    );
}
