#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include <stdint.h>

// Non-uniform rational B-spline animation compression utilities.
class hkaNURBS {
public:
    struct Point {
        // The default constructor leaves the four floats uninitialised.
        Point() = default;
        Point(float x, float y, float z, float w);

        hkaNURBS::Point abs();
        hkaNURBS::Point floor();
        hkaNURBS::Point ceil();
        // Returns the address of component u.
        float& operator[](int32_t u);
        // The length of the point as a 4-vector.
        float mag() const;
        hkaNURBS::Point normalize();
        hkaNURBS::Point& operator-=(const hkaNURBS::Point& a);
        hkaNURBS::Point& operator*=(float d);
        hkaNURBS::Point& operator/=(float d);
        float mag2() const;
        // A copy of this times d (operator*=).
        hkaNURBS::Point Mul(float d) const;
        // A copy of this minus a (operator-=).
        hkaNURBS::Point Sub(const hkaNURBS::Point& a) const;
        hkaNURBS::Point& operator+=(const hkaNURBS::Point& a);
        // Copies a's four floats.
        hkaNURBS::Point& operator=(const hkaNURBS::Point& a);
        // A copy of this plus a (operator+=).
        hkaNURBS::Point Add(const hkaNURBS::Point& a) const;
        // A copy of this divided by d (as operator/= does).
        hkaNURBS::Point Div(float d) const;
        float operator[](int32_t i) const;

        // The 4-component dot product of P and Q.
        static float Dot(const hkaNURBS::Point& P, const hkaNURBS::Point& Q);
        static hkaNURBS::Point Max(const hkaNURBS::Point& P, const hkaNURBS::Point& Q);
        static hkaNURBS::Point Min(const hkaNURBS::Point& P, const hkaNURBS::Point& Q);
        // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
        static float Dist(const hkaNURBS::Point& a1, const hkaNURBS::Point& a2);
        // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
        static float Cos(const hkaNURBS::Point& a1, const hkaNURBS::Point& a2);

        float point[4];
    };

    static int32_t MaxIndex(const hkArray<float, hkContainerHeapAllocator>& arr, int32_t len);
    static void RemoveSubtractOne(int32_t i, hkArray<int32_t, hkContainerHeapAllocator>& U, int32_t m);
    static hkBool WithinTolerance(
        int32_t n,
        int32_t p,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        const hkArray<float, hkContainerHeapAllocator>& U,
        int32_t m,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
        float E
    );
    static hkBool WithinTolerance(
        int32_t n,
        int32_t p,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        const hkArray<float, hkContainerHeapAllocator>& U,
        int32_t m,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
        const hkArray<float, hkContainerHeapAllocator>& ub,
        const hkArray<float, hkContainerHeapAllocator>& E
    );
    // The overload that computes the chord-length parameters itself.
    static void GlobalCurveApproxErrBnd(
        int32_t m,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
        int32_t p,
        float E,
        int32_t& n,
        hkArray<float, hkContainerHeapAllocator>& U,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P
    );
    // The overload that fits to the given parameters ub.
    static void GlobalCurveApproxErrBnd(
        int32_t m,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
        hkArray<float, hkContainerHeapAllocator>& ub,
        int32_t bProject,
        int32_t p,
        float E,
        int32_t& n,
        hkArray<float, hkContainerHeapAllocator>& U,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P
    );
    // The overload taking ub as const: fits through a copy of it.
    static void GlobalCurveApproxErrBnd(
        int32_t m,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
        const hkArray<float, hkContainerHeapAllocator>& ub,
        int32_t p,
        float E,
        int32_t& n,
        hkArray<float, hkContainerHeapAllocator>& U,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P
    );
    static void CurvePoint(
        int32_t n,
        int32_t p,
        const hkArray<float, hkContainerHeapAllocator>& U,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        int32_t mb,
        const hkArray<float, hkContainerHeapAllocator>& ub,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Cb
    );
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static int32_t FindSpan(int32_t a1, int32_t a2, float a3, const hkArray<float, hkContainerHeapAllocator>& a4);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static void BasisFuns(int32_t i, float u, int32_t p, const hkArray<float, hkContainerHeapAllocator>& U, float* N);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static void DersBasisFuns(
        int32_t i,
        float u,
        int32_t p,
        int32_t n,
        const hkArray<float, hkContainerHeapAllocator>& U,
        float (*ders)[4]
    );
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static float
    OneBasisFun(int32_t p, int32_t m, const hkArray<float, hkContainerHeapAllocator>& U, int32_t i, float u);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static void CurveDerivs(
        int32_t n,
        int32_t p,
        const hkArray<float, hkContainerHeapAllocator>& U,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        float u,
        int32_t d,
        hkaNURBS::Point* CK
    );
    // CurvePoint(n, p, U, P, u): the curve's point at u, built in apResult; its body is FindSpan, BasisFuns and the
    // weighted sum of the p + 1 control points.
    static hkaNURBS::Point CurvePoint(
        int32_t n,
        int32_t p,
        const hkArray<float, hkContainerHeapAllocator>& U,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        float u
    );
    // The index of the first of the m + 1 values arr not below val.
    static int32_t Find(float val, const hkArray<float, hkContainerHeapAllocator>& arr, int32_t m);
    // The index of the least of the first len values of arr.
    static int32_t MinIndex(const hkArray<float, hkContainerHeapAllocator>& arr, int32_t len);
    // i!.
    static int32_t Factorial(int32_t i);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static int32_t Multiplicity(int32_t a1, int32_t a2, const hkArray<float, hkContainerHeapAllocator>& a3);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static int32_t UniqueKnotCount(const hkArray<float, hkContainerHeapAllocator>& a1, int32_t a2);
    // Grows the knots by UniqueKnotCount(knots, degree) + 1 and repeats each unique knot once more.
    static int32_t DegreeElevateKnotVector(hkArray<float, hkContainerHeapAllocator>& U, int32_t m);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static float Distance3D(const hkaNURBS::Point& a1, const hkaNURBS::Point& a2);
    // The NURBS Book's GetRemovalBndCurve (A5.8's error test, t = 0): the distance by which removing knot u (index r,
    // multiplicity s) moves the curve. Parameter names are from the book.
    static float GetRemovalBoundCurve(
        int32_t n,
        int32_t p,
        const hkArray<float, hkContainerHeapAllocator>& U,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        float u,
        int32_t r,
        int32_t s
    );
    // The NURBS Book's RemoveCurveKnot (A5.8): removes knot u (index r, multiplicity s) from U and Pw up to num times
    // while the curve moves by at most TOL, and returns how many times it was removed. Public here like its siblings in
    // this class.
    static int32_t RemoveCurveKnot(
        int32_t n,
        int32_t p,
        hkArray<float, hkContainerHeapAllocator>& U,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Pw,
        float u,
        int32_t r,
        int32_t s,
        int32_t num,
        float TOL
    );
    // The NURBS Book's RemoveKnotsBoundCurve (A9.9): removes the knots of the curve (n, p, U, Pw) whose removal keeps
    // the error ek of each parameter ub within E; n drops by one per knot removed, and the count is returned. Public
    // because its callers are free functions. Q is never read.
    static int32_t RemoveKnotsBoundCurve(
        int32_t& n,
        int32_t p,
        hkArray<float, hkContainerHeapAllocator>& U,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        int32_t mb,
        const hkArray<float, hkContainerHeapAllocator>& ub,
        hkArray<float, hkContainerHeapAllocator>& e,
        float E,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q_JUNK
    );
    // Whether every distinct knot span of U (n, p) holds one of the m + 1 parameters ub at least tol inside it.
    static hkBool IsWellPosed(
        int32_t n,
        int32_t p,
        const hkArray<float, hkContainerHeapAllocator>& U,
        int32_t m,
        const hkArray<float, hkContainerHeapAllocator>& ub,
        float tol
    );
    // The NURBS Book's least-squares curve fit (A9.7) of the m + 1 points Q at the parameters ub with n + 1 control
    // points of degree p on the knots U: P[0] = Q[0], P[n] = Q[m], the inner ones solved by conjugate gradients; P
    // takes the result only when it is within E of every Q.
    static hkBool LeastSquaresFit(
        int32_t n,
        int32_t p,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        const hkArray<float, hkContainerHeapAllocator>& U,
        int32_t m,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Q,
        const hkArray<float, hkContainerHeapAllocator>& ub,
        float tol,
        float E
    );
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static void Remove(int32_t i, hkArray<float, hkContainerHeapAllocator>& U, int32_t m);
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static void ComputeBounds(
        int32_t n,
        int32_t p,
        const hkArray<float, hkContainerHeapAllocator>& U,
        const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& P,
        hkArray<float, hkContainerHeapAllocator>& B,
        int32_t lo,
        int32_t hi
    );
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static void DegreeElevateCurve(
        int32_t& n,
        int32_t p,
        hkArray<float, hkContainerHeapAllocator>& U,
        hkArray<hkaNURBS::Point, hkContainerHeapAllocator>& Pw,
        int32_t t
    );
    // Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
    static int32_t DegreeLowerKnotVector(hkArray<float, hkContainerHeapAllocator>& U, int32_t m);
};

// The free operator*(float, const hkaNURBS::Point&): d times a (Point::Mul).
hkaNURBS::Point mulScalarPoint(float d, const hkaNURBS::Point* a);
// a divided by d (Point::Div).
hkaNURBS::Point divScalarPoint(float d, const hkaNURBS::Point* a);
// Whether a3's float element a2 is the last one (a2 == a4) or differs from the next; the result byte is written through
// a1. Used by UniqueKnot, ComputeBounds and IsWellPosed. a3 is the knot vector, indexed as floats.
uint8_t* isKnotDistinct(uint8_t* a1, int32_t a2, const hkArray<float, hkContainerHeapAllocator>* a3, int32_t a4);
// Point inversion: returns the parameter of the curve (n, p, U, P) nearest apPoint.
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
);
int32_t UniqueKnot(int32_t a1, const hkArray<float, hkContainerHeapAllocator>* a2, int32_t a3);
// The parameter and the distance to the curve (n, p, U, P) of each of the m + 1 data points Q. U and P go on to
// projectPointToCurve; pErrors and pParams receive the results as floats. bProject is tested as a byte, but it is
// int32_t because its caller (the GlobalCurveApproxErrBnd overload) passes on its own caller's whole 32-bit value.
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
);
// Whether the curve (a2 = n, a3 = p, a5 = U, a4 = P) lies within 1.1 * a9 of each of the a6 + 1 points a7 (Q) at the
// parameters a8 (ub). a4 holds the control points and a5 the knots.
hkBool isWithinErrorBound(
    int32_t a2,
    int32_t a3,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* a4,
    const hkArray<float, hkContainerHeapAllocator>* a5,
    int32_t a6,
    const hkArray<hkaNURBS::Point, hkContainerHeapAllocator>* a7,
    const hkArray<float, hkContainerHeapAllocator>* a8,
    float a9
);
// a1 is signed. arg4 holds the points and is only read. a3 is the parameter array it fills with floats.
int32_t ChordLengthParameterization(
    int32_t a1,
    const hkArrayBase<hkaNURBS::Point>* arg4,
    hkArray<float, hkContainerHeapAllocator>* a3
);
// The body of ApproximateNURBSCurve, which passes it its own nine arguments.
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
);

// A dense m x n matrix of hkaNURBS.cpp's least-squares fit, its m * n elements in m_data.
template <typename T>
class hkaMatrix {
public:
    // Leaves m_data empty, for a by-value result that a call fills in.
    hkaMatrix() = default;
    hkaMatrix(int32_t m, int32_t n) : m_data(), m_m(m), m_n(n) {
        m_data.setSize(m * n);
    }

    hkaMatrix(int32_t m, int32_t n, T v) : m_data(), m_m(m), m_n(n) {
        m_data.setSize(m * n);
        for (int32_t i = 0; i < m * n; ++i) {
            m_data.m_data[i] = v;
        }
    }

    hkaMatrix(const hkaMatrix& other) : m_data(), m_m(other.m_m), m_n(other.m_n) {
        m_data.setSize(m_m * m_n);
        for (int32_t i = 0; i < m_m * m_n; ++i) {
            m_data.m_data[i] = other.m_data.m_data[i];
        }
    }

    // m_data's storage goes with its member.
    ~hkaMatrix() {}

    // Element i of m_data.
    T& operator()(int32_t i);
    // Element i of m_data, by value.
    T operator()(int32_t i) const;
    // m * n.
    int32_t Size() const;
    // Builds in apResult a copy of the matrix minus b, element by element, and returns apResult.
    hkaMatrix* Sub(hkaMatrix* apResult, const hkaMatrix& b) const;
    // b subtracted from the matrix, element by element.
    hkaMatrix& operator-=(const hkaMatrix& A);
    // Element (i, j): m_data[i * m_n + j].
    T& operator()(int32_t i, int32_t j);
    // Element (i, j), by value.
    T operator()(int32_t i, int32_t j) const;
    // Every element set to v.
    void SetAll(const T& v);

    hkArray<T, hkContainerHeapAllocator> m_data;
    int32_t m_m;
    int32_t m_n;
};

// A sparse m x n matrix of hkaNURBS.cpp's least-squares fit: entry k is val[k] at (row[k], col[k]).
template <typename T>
class hkaSparseLSQMatrix {
public:
    hkaSparseLSQMatrix(int32_t m, int32_t n) : val(), row(), col(), m_m(m), m_n(n) {}
    // The three arrays' storage goes with their members.
    ~hkaSparseLSQMatrix() {}

    // Appends the entry v at (i, j); a zero v is not stored.
    void Set(int32_t i, int32_t j, T v);
    // result += A^T A x, A being this matrix: tmp is zeroed and set to A x on the way.
    void MultiplyAdd(hkaMatrix<T>& result, const hkaMatrix<T>& x, hkaMatrix<T>& scratch) const;
    // result = A^T A x: result zeroed, then MultiplyAdd.
    void Multiply(hkaMatrix<T>& result, const hkaMatrix<T>& x, hkaMatrix<T>& scratch) const;
    // Computes A^T A x, built in apResult.
    hkaMatrix<T>* Mul(hkaMatrix<T>* apResult, const hkaMatrix<T>& x) const;

    hkArray<T, hkContainerHeapAllocator> val;
    hkArray<int32_t, hkContainerHeapAllocator> row;
    hkArray<int32_t, hkContainerHeapAllocator> col;
    int32_t m_m;
    int32_t m_n;
};

// Conjugate gradient solver for hkaNURBS.cpp's least-squares fit: only static members, used as hkaCGSolver<float>.
template <typename T>
class hkaCGSolver {
public:
    // Solves A^T A x = b column by column: each column of x and b copied into column matrices, SolveOneColumn run on
    // them with maxIter and tol, the result copied back into x; false as soon as one column fails, built in apResult.
    static hkBool Solve(const hkaSparseLSQMatrix<T>& A, hkaMatrix<T>& x, const hkaMatrix<T>& b, int32_t maxIter, T tol);
    // Conjugate gradients on A^T A x = b from the start x, at most maxIter steps, until the squared residual falls
    // below tol squared: whether it did is written to apResult.
    static hkBool
    SolveOneColumn(const hkaSparseLSQMatrix<T>& A, hkaMatrix<T>& x, const hkaMatrix<T>& b, int32_t maxIter, T tol);
    // The sum of a(i) * b(i) over a's elements.
    static T InnerProduct(const hkaMatrix<T>& a, const hkaMatrix<T>& b);
    // x = a + s * b, element by element.
    static void ScaleAdd(hkaMatrix<T>& x, const hkaMatrix<T>& a, const T& s, const hkaMatrix<T>& b);
    // x += s * b, element by element.
    static void ScaleAdd(hkaMatrix<T>& x, const T& s, const hkaMatrix<T>& b);
    // x = -y, element by element.
    static void CopyNegative(hkaMatrix<T>& a, const hkaMatrix<T>& b);
    // x = -x, element by element.
    static void Negate(hkaMatrix<T>& a);
};
template <typename T>
T& hkaMatrix<T>::operator()(int32_t i) {
    return m_data.m_data[i];
}

template <typename T>
T hkaMatrix<T>::operator()(int32_t i) const {
    return m_data.m_data[i];
}

template <typename T>
int32_t hkaMatrix<T>::Size() const {
    return m_m * m_n;
}

// A copy of the matrix (the copy constructor in apResult's storage), then operator-=.
template <typename T>
hkaMatrix<T>* hkaMatrix<T>::Sub(hkaMatrix<T>* apResult, const hkaMatrix<T>& b) const {
    new (apResult) hkaMatrix<T>(*this);
    *apResult -= b;
    return apResult;
}

template <typename T>
hkaMatrix<T>& hkaMatrix<T>::operator-=(const hkaMatrix<T>& A) {
    for (int32_t i = 0; i < m_m * m_n; ++i) {
        m_data.m_data[i] -= A.m_data.m_data[i];
    }
    return *this;
}

template <typename T>
T& hkaMatrix<T>::operator()(int32_t i, int32_t j) {
    return m_data.m_data[i * m_n + j];
}

template <typename T>
T hkaMatrix<T>::operator()(int32_t i, int32_t j) const {
    return m_data.m_data[i * m_n + j];
}

template <typename T>
void hkaMatrix<T>::SetAll(const T& v) {
    for (int32_t i = 0; i < m_m * m_n; ++i) {
        m_data.m_data[i] = v;
    }
}

template <typename T>
void hkaSparseLSQMatrix<T>::Set(int32_t i, int32_t j, T v) {
    if (T(0) != v) {
        row.pushBack(i);
        col.pushBack(j);
        val.pushBack(v);
    }
}

template <typename T>
void hkaSparseLSQMatrix<T>::MultiplyAdd(hkaMatrix<T>& result, const hkaMatrix<T>& x, hkaMatrix<T>& scratch) const {
    scratch.SetAll(T(0));
    // tmp = A x.
    for (int32_t k = 0; k < result.m_n; ++k) {
        for (int32_t i = 0; i < val.m_size; ++i) {
            T& rTmp = scratch(row.m_data[i], k);
            rTmp = x(col.m_data[i], k) * val.m_data[i] + rTmp;
        }
    }
    // result += A^T tmp.
    for (int32_t k = 0; k < result.m_n; ++k) {
        for (int32_t i = 0; i < val.m_size; ++i) {
            T& rResult = result(col.m_data[i], k);
            rResult = val.m_data[i] * scratch(row.m_data[i], k) + rResult;
        }
    }
}

template <typename T>
void hkaSparseLSQMatrix<T>::Multiply(hkaMatrix<T>& result, const hkaMatrix<T>& x, hkaMatrix<T>& scratch) const {
    result.SetAll(T(0));
    MultiplyAdd(result, x, scratch);
}

// The result is built in apResult's storage (the zero x.m_m x x.m_n matrix), then MultiplyAdd runs through a zero m_m x
// x.m_n tmp.
template <typename T>
hkaMatrix<T>* hkaSparseLSQMatrix<T>::Mul(hkaMatrix<T>* apResult, const hkaMatrix<T>& x) const {
    new (apResult) hkaMatrix<T>(x.m_m, x.m_n, T(0));
    hkaMatrix<T> tmp(m_m, x.m_n, T(0));
    MultiplyAdd(*apResult, x, tmp);
    return apResult;
}

template <typename T>
T hkaCGSolver<T>::InnerProduct(const hkaMatrix<T>& a, const hkaMatrix<T>& b) {
    const int32_t iSize = a.Size();
    T sum = T(0);
    for (int32_t i = 0; i < iSize; ++i) {
        sum = b(i) * a(i) + sum;
    }
    return sum;
}

template <typename T>
void hkaCGSolver<T>::ScaleAdd(hkaMatrix<T>& x, const hkaMatrix<T>& a, const T& s, const hkaMatrix<T>& b) {
    const int32_t iSize = a.Size();
    for (int32_t i = 0; i < iSize; ++i) {
        x(i) = a(i) + b(i) * s;
    }
}

template <typename T>
void hkaCGSolver<T>::ScaleAdd(hkaMatrix<T>& x, const T& s, const hkaMatrix<T>& b) {
    const int32_t iSize = x.Size();
    for (int32_t i = 0; i < iSize; ++i) {
        x(i) = b(i) * s + x(i);
    }
}

template <typename T>
void hkaCGSolver<T>::CopyNegative(hkaMatrix<T>& a, const hkaMatrix<T>& b) {
    const int32_t iSize = a.Size();
    for (int32_t i = 0; i < iSize; ++i) {
        a(i) = -b(i);
    }
}

template <typename T>
void hkaCGSolver<T>::Negate(hkaMatrix<T>& a) {
    const int32_t iSize = a.Size();
    for (int32_t i = 0; i < iSize; ++i) {
        a(i) = -a(i);
    }
}

// r = b - A^T A x, recomputed from x every 50 steps; r and Ax are by-value results the calls build in their storage
// (the default-constructed locals hold no storage yet).
template <typename T>
hkBool hkaCGSolver<
    T>::SolveOneColumn(const hkaSparseLSQMatrix<T>& A, hkaMatrix<T>& x, const hkaMatrix<T>& b, int32_t maxIter, T tol) {
    const int32_t iRows = x.m_m;
    hkaMatrix<T> r;
    {
        hkaMatrix<T> Ax;
        b.Sub(&r, *A.Mul(&Ax, x));
    }
    bool bConverged = false;
    {
        hkaMatrix<T> p(r);
        hkaMatrix<T> Ap(iRows, 1);
        hkaMatrix<T> tmp(A.m_m, 1);
        T rr = InnerProduct(r, r);
        const T tol2 = tol * tol;
        for (int32_t i = 0; i < maxIter;) {
            if (tol2 > rr) {
                bConverged = true;
                break;
            }
            A.Multiply(Ap, p, tmp);
            T alpha = rr / InnerProduct(p, Ap);
            ScaleAdd(x, alpha, p);
            if (++i % 50 == 0) {
                CopyNegative(r, b);
                A.MultiplyAdd(r, x, tmp);
                Negate(r);
            }
            else {
                alpha = -alpha;
                ScaleAdd(r, alpha, Ap);
            }
            const T rrOld = rr;
            rr = InnerProduct(r, r);
            const T beta = rr / rrOld;
            ScaleAdd(p, r, beta, p);
        }
    }
    return bConverged;
}

// Each column of x and b is copied into a column matrix and solved; the solved column goes back into x.
template <typename T>
hkBool
hkaCGSolver<T>::Solve(const hkaSparseLSQMatrix<T>& A, hkaMatrix<T>& x, const hkaMatrix<T>& b, int32_t maxIter, T tol) {
    hkaMatrix<T> xCol(x.m_m, 1);
    hkaMatrix<T> bCol(b.m_m, 1);
    bool bSolved = true;
    for (int32_t j = 0; j < x.m_n; ++j) {
        for (int32_t i = 0; i < x.m_m; ++i) {
            xCol(i) = x(i, j);
            bCol(i) = b(i, j);
        }
        if (!SolveOneColumn(A, xCol, bCol, maxIter, tol)) {
            bSolved = false;
            break;
        }
        for (int32_t i = 0; i < x.m_m; ++i) {
            x(i, j) = xCol(i);
        }
    }
    return bSolved;
}
