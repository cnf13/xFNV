#include "SDK/FaceGen/MatrixVT.h"

#include "BSCore/MemoryManager.h"
#include "SDK/FaceGen/MatrixVT.inl"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <memory>
#include <vector>

// The standalone build's one instantiation of the matrix's generic members.
template class FR2MatrixVTC<float>;

FR2MatrixVTC<float>*
ConcatRows(FR2MatrixVTC<float>* apResult, FR2MatrixVTC<float>* hProcess, FR2MatrixVTC<float>* apLower) {
    FR2MatrixVTC<float>::RowT kRow;
    FR2MatrixVTC<float> kConcat;
    bool bUpperEmpty = !hProcess->nrows || !hProcess->ncols;
    bool bLowerEmpty = !apLower->nrows || !apLower->ncols;
    if (bUpperEmpty && bLowerEmpty) {
        apResult->data.~vector<float, std::allocator<float>>();
        ::new (&apResult->data) std::vector<float, std::allocator<float>>;
        apResult->nrows = 0;
        apResult->ncols = 0;
        apResult->data.resize(apResult->nrows * apResult->ncols, 0.0f);
        return apResult;
    }

    if (bUpperEmpty) {
        ::new (apResult) FR2MatrixVTC<float>(*apLower);
        return apResult;
    }

    if (bLowerEmpty) {
        ::new (apResult) FR2MatrixVTC<float>(*hProcess);
        return apResult;
    }

    if (hProcess->ncols != apLower->ncols) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 1278);
    }
    uint32_t ncols = hProcess->ncols;
    uint32_t nrows = hProcess->nrows;
    uint32_t uiLowerRows = apLower->nrows;
    kConcat.data.~vector<float, std::allocator<float>>();
    ::new (&kConcat.data) std::vector<float, std::allocator<float>>;
    kConcat.nrows = uiLowerRows + nrows;
    kConcat.ncols = ncols;
    kConcat.data.resize((uiLowerRows + nrows) * ncols, 0.0f);
    nrows = hProcess->nrows;
    ncols = hProcess->ncols;
    float* pSource = &hProcess->data[0];
    float* pDest = &kConcat.data[0];
    rt::memcpy(pDest, pSource, sizeof(float) * (ncols * nrows));
    uiLowerRows = apLower->nrows;
    uint32_t uiLowerCols = apLower->ncols;
    kRow = (*apLower)[0];
    rt::memcpy(kConcat[hProcess->nrows].p, kRow.p, sizeof(float) * (uiLowerCols * uiLowerRows));
    ::new (apResult) FR2MatrixVTC<float>(kConcat);
    return apResult;
}
