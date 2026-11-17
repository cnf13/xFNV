// Kept out of FR2MatrixVTC.h: the bodies need the FaceGen assert and the memory manager, which the header's users do
// not all include.
#pragma once

// The generic members, instantiated in FR2MatrixVTC.cpp. The rows are stored one after the other in data.
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/string.h"

#include <new>

template <class T>
T& FR2MatrixVTC<T>::RowT::operator[](uint32_t i) {
    return p[i];
}

template <class T>
const T& FR2MatrixVTC<T>::RowConstT::operator[](uint32_t i) const {
    return p[i];
}

template <class T>
void FR2MatrixVTC<T>::resize(int32_t nr, int32_t nc) {
    nrows = static_cast<uint32_t>(nr);
    ncols = static_cast<uint32_t>(nc);
    data.resize(static_cast<uint32_t>(nc * nr));
}

template <class T>
FR2MatrixVTC<T>::RowT FR2MatrixVTC<T>::operator[](uint32_t i) {
    return RowT(data.data() + ncols * i, ncols);
}

template <class T>
FR2MatrixVTC<T>::RowConstT FR2MatrixVTC<T>::operator[](uint32_t i) const {
    return RowConstT{data.data() + ncols * i, ncols};
}

template <class T>
uint32_t FR2MatrixVTC<T>::numRows() const {
    return nrows;
}

template <class T>
uint32_t FR2MatrixVTC<T>::numCols() const {
    return ncols;
}

// The storage is sized to m's size and the elements copied over.
template <class T>
FR2MatrixVTC<T>& FR2MatrixVTC<T>::operator=(const FR2MatrixVTC& m) {
    if (this != &m) {
        nrows = m.nrows;
        ncols = m.ncols;
        data.resize(ncols * nrows);
        for (uint32_t k = 0; k < ncols * nrows; ++k) {
            data[k] = m.data[k];
        }
    }
    return *this;
}

template <class T>
bool FR2MatrixVTC<T>::operator!=(const FR2MatrixVTC& mm) const {
    return !operator==(mm);
}

// The elements are compared as bytes, with one memcmp.
template <class T>
bool FR2MatrixVTC<T>::operator==(const FR2MatrixVTC& m) const {
    if (m.nrows != nrows || m.ncols != ncols) {
        return false;
    }
    if (nrows && ncols) {
        return std::memcmp(data.data(), m.data.data(), ncols * (sizeof(T) * nrows)) == 0;
    }
    return true;
}

// A size different from m's is reported (matrixVT.hpp line 591); the walk stops at the end of either matrix.
template <class T>
FR2MatrixVTC<T>& FR2MatrixVTC<T>::operator+=(const FR2MatrixVTC& m) {
    if (nrows != m.nrows || ncols != m.ncols) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 591);
    }
    auto it = data.begin();
    auto jt = m.data.begin();
    while (it != data.end() && jt != m.data.end()) {
        *it = *jt + *it;
        ++it;
        ++jt;
    }
    return *this;
}

// A size different from m's is reported (matrixVT.hpp line 670); the walk stops at the end of either matrix.
template <class T>
FR2MatrixVTC<T>& FR2MatrixVTC<T>::operator-=(const FR2MatrixVTC& m) {
    if (nrows != m.nrows || ncols != m.ncols) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 670);
    }
    auto it = data.begin();
    auto jt = m.data.begin();
    while (it != data.end() && jt != m.data.end()) {
        *it = *it - *jt;
        ++it;
        ++jt;
    }
    return *this;
}

// Clears the elements' bytes with one memset.
template <class T>
void FR2MatrixVTC<T>::setZero() {
    std::memset(data.data(), 0, sizeof(T) * (ncols * nrows));
}

// Vector deleting destructor. With auiFlags & 2, this is the first element of an array new[] made: the element count is
// the array cookie in front of it (the Itanium C++ ABI's size_t, padded to the element's alignment), the elements are
// destroyed last to first and, with auiFlags & 1, the block freed from the cookie; the block is returned. Otherwise
// this one matrix is destroyed and, with auiFlags & 1, freed.
template <class T>
void* FR2MatrixVTC<T>::DeleteArray(uint32_t auiFlags) {
    if ((auiFlags & 2) != 0) {
        // The cookie is raw memory new[] writes in front of the elements, untyped.
        void* pBlock = reinterpret_cast<uint8_t*>(this) -
            (alignof(FR2MatrixVTC) > sizeof(size_t) ? alignof(FR2MatrixVTC) : sizeof(size_t));
        size_t uiCount = *(reinterpret_cast<size_t*>(this) - 1);
        while (uiCount != 0) {
            --uiCount;
            this[uiCount].~FR2MatrixVTC();
        }
        if ((auiFlags & 1) != 0) {
            operator delete[](pBlock);
        }
        return pBlock;
    }
    this->~FR2MatrixVTC();
    if ((auiFlags & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// Each square is added to the sum, kept as a float.
template <class T>
float FR2MatrixVTC<T>::lengthSqr() const {
    float fSum = 0.0f;
    for (uint32_t i = 0; i < nrows; ++i) {
        for (uint32_t j = 0; j < ncols; ++j) {
            const T& rValue = (*this)[i][j];
            fSum = rValue * rValue + fSum;
        }
    }
    return fSum;
}

template <class T>
FR2MatrixVTC<T>& FR2MatrixVTC<T>::operator*=(float d) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        *it = *it * d;
    }
    return *this;
}

// The matrices are freed with ourFree.
template <class T>
void FR2MatrixVTC<T>::operator delete(void* apMemory) {
    ourFree(apMemory);
}

template <class T>
void FR2MatrixVTC<T>::operator delete[](void* apMemory) {
    ourFree(apMemory);
}

// Every element times s, copied into apResult.
template <class T>
FR2MatrixVTC<T>* FR2MatrixVTC<T>::MulScalar(FR2MatrixVTC* apResult, float s) const {
    FR2MatrixVTC kResult(nrows, ncols);
    auto jt = kResult.data.begin();
    for (auto it = data.begin(); it != data.end(); ++it, ++jt) {
        *jt = *it * s;
    }
    ::new (apResult) FR2MatrixVTC(kResult);
    return apResult;
}

// The product with m: element (i, k) is the sum over j of this (i, j) times m (j, k), accumulated in the result's
// element. A column count different from m's row count is reported (matrixVT.hpp line 275) and the product computed
// anyway.
template <class T>
FR2MatrixVTC<T>* FR2MatrixVTC<T>::Mul(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const {
    if (ncols != m.nrows) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 275);
    }
    FR2MatrixVTC kResult(nrows, m.ncols);
    for (uint32_t i = 0; i < nrows; ++i) {
        for (uint32_t k = 0; k < m.ncols; ++k) {
            kResult[i][k] = T();
            for (uint32_t j = 0; j < ncols; ++j) {
                T& rSum = kResult[i][k];
                rSum = (*this)[i][j] * m[j][k] + rSum;
            }
        }
    }
    ::new (apResult) FR2MatrixVTC(kResult);
    return apResult;
}

// The element-wise sum with m. A size different from m's is reported (matrixVT.hpp line 365) and the sum walks this
// matrix's elements anyway.
template <class T>
FR2MatrixVTC<T>* FR2MatrixVTC<T>::Add(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const {
    if (nrows != m.nrows || ncols != m.ncols) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 365);
    }
    FR2MatrixVTC kResult(nrows, ncols);
    auto kt = kResult.data.begin();
    auto jt = m.data.begin();
    for (auto it = data.begin(); it != data.end(); ++it, ++jt, ++kt) {
        *kt = *it + *jt;
    }
    ::new (apResult) FR2MatrixVTC(kResult);
    return apResult;
}

// The ncols x nrows matrix whose element (j, i) is this matrix's (i, j).
template <class T>
FR2MatrixVTC<T>* FR2MatrixVTC<T>::transpose(FR2MatrixVTC* apResult) const {
    FR2MatrixVTC kResult(ncols, nrows);
    for (uint32_t i = 0; i < nrows; ++i) {
        for (uint32_t j = 0; j < ncols; ++j) {
            kResult[j][i] = (*this)[i][j];
        }
    }
    ::new (apResult) FR2MatrixVTC(kResult);
    return apResult;
}

// The matrix times 1/s (the reciprocal rounded to a float) through MulScalar.
template <class T>
FR2MatrixVTC<T>* FR2MatrixVTC<T>::DivScalar(FR2MatrixVTC* apResult, float s) const {
    float fInverse = 1.0 / s;
    MulScalar(apResult, fInverse);
    return apResult;
}

// The element-wise difference with m. A size different from m's is reported (matrixVT.hpp line 458) and the
// difference walks this matrix's elements anyway.
template <class T>
FR2MatrixVTC<T>* FR2MatrixVTC<T>::Sub(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const {
    if (nrows != m.nrows || ncols != m.ncols) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 458);
    }
    FR2MatrixVTC kResult(nrows, ncols);
    auto kt = kResult.data.begin();
    auto jt = m.data.begin();
    for (auto it = data.begin(); it != data.end(); ++it, ++jt, ++kt) {
        *kt = *it - *jt;
    }
    ::new (apResult) FR2MatrixVTC(kResult);
    return apResult;
}

// The nr x nc block starting at row r and column c, copied row by row. A block that does not fit is reported
// (matrixVT.hpp line 1063) and copied anyway.
template <class T>
FR2MatrixVTC<T>*
FR2MatrixVTC<T>::subMatrix(FR2MatrixVTC* apResult, uint32_t r, uint32_t c, uint32_t nr, uint32_t nc) const {
    if (nc + c > ncols || nr + r > nrows) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 1063);
    }
    FR2MatrixVTC retval(nr, nc);
    if (nr && nc) {
        for (uint32_t i = 0; i < retval.nrows; ++i) {
            RowConstT kSrcRow = (*this)[r + i];
            RowT kDstRow = retval[i];
            for (uint32_t j = 0; j < retval.ncols; ++j) {
                kDstRow[j] = kSrcRow[c + j];
            }
        }
    }
    ::new (apResult) FR2MatrixVTC(retval);
    return apResult;
}
