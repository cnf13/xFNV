#pragma once

#include "SDK/MSVC/ehvec.h"

#include <memory>
#include <stdint.h>
#include <vector>

// The float vector under one name (a macro argument can't hold the template's commas): FR2MatrixVTC<float>'s storage
// and FaceGen's coordinate data.
using FloatVector = std::vector<float, std::allocator<float>>;

// The members of std::vector<float>, _Vector_const_iterator<float> and _Vector_iterator<float> that the matrix
// arithmetic calls, kept as free functions per element type. vector::begin and vector::end on a const vector are
// FloatVector's begin() const and end() const.
bool VectorIteratorNotEqual(const FloatVector::const_iterator* apThis, const FloatVector::const_iterator& _Right);
// const_iterator::operator*, checked.
const float& VectorIteratorDeref(const FloatVector::const_iterator* apThis);
// iterator::operator*, through the const one.
float& VectorIteratorDeref(const FloatVector::iterator* apThis);
// const_iterator::operator++(int): the iterator before the step, built in apResult and returned.
FloatVector::const_iterator* VectorIteratorPostIncrement(
    FloatVector::const_iterator* apThis,
    FloatVector::const_iterator* apResult,
    int32_t _Postfix
);
// iterator::operator++(int): the iterator before the step, built in apResult and returned.
FloatVector::iterator*
VectorIteratorPostIncrement(FloatVector::iterator* apThis, FloatVector::iterator* apResult, int32_t _Postfix);
// The float vector's helpers that FloatVector::_Insert_n (FR2MatrixVTC.cpp) calls, declared here so that other files'
// bodies can call them and their own bodies can be defined with qualified names. VectorUmove is _Umove(_First, _Last,
// _Ptr).
float* VectorUmove(FloatVector* _This, float* _First, float* _Last, float* _Ptr);
// fill(_First, _Last, _Val).
void Fill(float* _First, float* _Last, const float& _Val);
// _Umove_backward(_First, _Last, _Dest).
void UncheckedMoveBackward(float* _First, float* _Last, float* _Dest);
void FillUnchecked(float* _First, float* _Last, const float* _Val);
// _Move_backward_opt(_First, _Last, _Dest, _IterCat, _MoveCat, _Checked) of floats: takes six arguments and reads only
// the first four.
float* MoveOpt(float* _First, float* _Last, float* _Dest, uint8_t _IterCat, uint8_t _MoveCat, uint8_t _Checked);

// A dense matrix of T, stored row by row in one vector (FaceGen's coordinate arrays).
template <class T>
class FR2MatrixVTC {
public:
    // One row of a matrix: the address of its first element and the row length.
    class RowT {
    public:
        RowT() = default;
        RowT(T* ap, uint32_t an) : p(ap), n(an) {}

        // The address of element i.
        T& operator[](uint32_t i);

        T* p;
        uint32_t n;
    };

    // One row of a const matrix, read-only: the address of its first element and the row length.
    class RowConstT {
    public:
        // The element i, read-only.
        const T& operator[](uint32_t i) const;

        const T* p;
        uint32_t n;
    };

    FR2MatrixVTC() : nrows(0), ncols(0) {
        data.clear();
    }
    FR2MatrixVTC(uint32_t nr, uint32_t nc) : nrows(nr), ncols(nc) {
        data.resize(nrows * ncols);
    }
    FR2MatrixVTC(const FR2MatrixVTC& m) : data(m.data), nrows(m.nrows), ncols(m.ncols) {}
    ~FR2MatrixVTC() {}

    // Sets the size to nr x nc and sizes the storage to fit.
    void resize(int32_t nr, int32_t nc);
    RowT operator[](uint32_t i);
    // Row i of a const matrix.
    RowConstT operator[](uint32_t i) const;
    uint32_t numRows() const;
    uint32_t numCols() const;
    // Copies the size and elements of m; self-assignment does nothing.
    FR2MatrixVTC& operator=(const FR2MatrixVTC& m);
    // True when the size or an element differs from mm's.
    bool operator!=(const FR2MatrixVTC& mm) const;
    // True when the size and the elements (one memcmp) equal mm's; two empty matrices of one
    // size are equal.
    bool operator==(const FR2MatrixVTC& m) const;
    // Adds m element by element; the sizes must match (fr2Assert).
    FR2MatrixVTC& operator+=(const FR2MatrixVTC& m);
    // Subtracts m element by element; the sizes must match (fr2Assert).
    FR2MatrixVTC& operator-=(const FR2MatrixVTC& m);
    // Zeroes every element.
    void setZero();
    // The sum of the squared elements.
    float lengthSqr() const;
    // Multiplies every element by s.
    FR2MatrixVTC& operator*=(float d);
    // operator*(float): the matrix times s, built in apResult.
    FR2MatrixVTC* MulScalar(FR2MatrixVTC* apResult, float s) const;
    // The matrix product with m, built in apResult.
    FR2MatrixVTC* Mul(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const;
    // operator+: the element-wise sum with m, built in apResult.
    FR2MatrixVTC* Add(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const;
    // The transpose, built in apResult.
    FR2MatrixVTC* transpose(FR2MatrixVTC* apResult) const;
    // Divides the matrix by s as the matrix times 1/s through MulScalar, built in apResult.
    FR2MatrixVTC* DivScalar(FR2MatrixVTC* apResult, float s) const;
    // operator-(const FR2MatrixVTC&): the element-wise difference with m, built in apResult.
    FR2MatrixVTC* Sub(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const;
    // The nr x nc block starting at row r and column c, built in apResult; the block must fit (fr2Assert).
    FR2MatrixVTC* subMatrix(FR2MatrixVTC* apResult, uint32_t r, uint32_t c, uint32_t nr, uint32_t nc) const;

    // The array delete frees with ourFree.
    static void operator delete(void* apMemory);
    static void operator delete[](void* apMemory);

    // The elements, row by row.
    std::vector<T, std::allocator<T>> data;
    uint32_t nrows;
    uint32_t ncols;
};
// The generic members are defined in MatrixVT.inl and instantiated in MatrixVT.cpp only.
extern template class FR2MatrixVTC<float>;

FR2MatrixVTC<float>*
ConcatRows(FR2MatrixVTC<float>* apResult, FR2MatrixVTC<float>* hProcess, FR2MatrixVTC<float>* apLower);
