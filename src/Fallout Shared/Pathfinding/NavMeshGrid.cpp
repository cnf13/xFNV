#include "Fallout Shared/Pathfinding/NavMeshGrid.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdint>
#include <limits>

NavMeshGrid::NavMeshGrid() : iGridSize(0), GridData(nullptr) {
    GridBoundsMin.x = std::numeric_limits<float>::max();
}

BSSimpleArray<uint16_t, 1024>* NavMeshGrid::GetTrianglesForGridCell(uint32_t aiIndex) {
    if (aiIndex < iGridSize * iGridSize) {
        return &GridData[aiIndex];
    }

    return nullptr;
}

bool NavMeshGrid::IsTriangleInGridCell(uint16_t aiTriangle, uint32_t aiIndex) {
    return GridData[aiIndex].IsInArray(aiTriangle);
}

bool NavMeshGrid::IsTriangleInGridCell(uint16_t aiTriangle, uint32_t aiColumn, uint32_t aiRow) {
    return IsTriangleInGridCell(aiTriangle, aiColumn + iGridSize * aiRow);
}

// Takes z first, then computes y and x (each rounded to float) in that order.
NiPoint3 NavMeshGrid::GetGridPosition(uint32_t aiColumn, uint32_t aiRow) {
    float fZ = GridBoundsMin.z;
    float fY = static_cast<float>(static_cast<double>(aiRow) * fRowSectionLen + GridBoundsMin.y);
    float fX = static_cast<float>(static_cast<double>(aiColumn) * fColumnSectionLen + GridBoundsMin.x);
    return NiPoint3(fX, fY, fZ);
}

void NavMeshGrid::GetTriangleData(const NiPoint3& aLoc, const BSSimpleArray<uint16_t, 1024>*& apArray) {
    if (!(GridBoundsMin.x > static_cast<double>(aLoc.x)) &&
        !(GridBoundsMax.x < static_cast<double>(aLoc.x)) &&
        !(GridBoundsMin.y > static_cast<double>(aLoc.y)) &&
        !(GridBoundsMax.y < static_cast<double>(aLoc.y))) {
        float fLocX = aLoc.x - GridBoundsMin.x;
        float fLocY = aLoc.y - GridBoundsMin.y;
        float fScaleX = fLocX / fColumnSectionLen;
        float fScaleY = fLocY / fRowSectionLen;
        uint32_t uiColumn = static_cast<int64_t>(std::floorf(fScaleX));
        uint32_t uiRow = static_cast<int64_t>(std::floorf(fScaleY));
        if (uiColumn >= iGridSize) {
            uiColumn = iGridSize - 1;
        }
        if (uiRow >= iGridSize) {
            uiRow = iGridSize - 1;
        }
        apArray = GetTriangleArray(uiColumn, uiRow);
    }
    else {
        apArray = nullptr;
    }
}

// The goal cell is computed from aLocStart too: aLocEnd is never read.
void NavMeshGrid::GetTriangleDataForAABB(
    const NiPoint3& aLocStart,
    const NiPoint3& aLocEnd,
    BSSimpleArray<uint16_t, 1024>* apReturnArray
) {
    using indicesToRemove_t = BSSimpleArray<uint32_t, 1024>;
    float fLocX = aLocStart.x - GridBoundsMin.x;
    float fLocY = aLocStart.y - GridBoundsMin.y;
    float fScaleX = fLocX / fColumnSectionLen;
    float fScaleY = fLocY / fRowSectionLen;
    int32_t iStartColumn = static_cast<int32_t>(std::floorf(fScaleX));
    int32_t iStartRow = static_cast<int32_t>(std::floorf(fScaleY));
    BSClamp<int32_t>(iStartRow, 0, static_cast<int32_t>(iGridSize - 1));
    BSClamp<int32_t>(iStartColumn, 0, static_cast<int32_t>(iGridSize - 1));
    fLocX = aLocStart.x - GridBoundsMin.x;
    fLocY = aLocStart.y - GridBoundsMin.y;
    fScaleX = fLocX / fColumnSectionLen;
    fScaleY = fLocY / fRowSectionLen;
    int32_t iGoalColumn = static_cast<int32_t>(std::floorf(fScaleX));
    int32_t iGoalRow = static_cast<int32_t>(std::floorf(fScaleY));
    BSClamp<int32_t>(iGoalRow, 0, static_cast<int32_t>(iGridSize - 1));
    BSClamp<int32_t>(iGoalColumn, 0, static_cast<int32_t>(iGridSize - 1));
    for (int32_t i = iStartRow; i <= iGoalRow; ++i) {
        for (int32_t j = iStartColumn; j <= iGoalColumn; ++j) {
            BSSimpleArray<uint16_t, 1024>* pArray = GetTriangleArray(static_cast<uint32_t>(j), i);
            for (uint16_t usTriangle : *pArray) {
                apReturnArray->Add(usTriangle);
            }
        }
    }
    if (!apReturnArray->IsEmpty()) {
        apReturnArray->Sort([](const void* apOne, const void* apTwo) {
            return CompareUShort(static_cast<const uint16_t*>(apOne), static_cast<const uint16_t*>(apTwo));
        });
        indicesToRemove_t indicesToRemove;
        for (uint32_t i = 0; i < apReturnArray->iSize - 1; ++i) {
            int32_t iNextTriangle = apReturnArray->GetAt(i + 1);
            if (iNextTriangle == apReturnArray->GetAt(i)) {
                uint32_t uiIndex = i + 1;
                indicesToRemove.Add(uiIndex);
            }
        }
        if (!indicesToRemove.IsEmpty()) {
            for (int32_t i = indicesToRemove.iSize - 1; i >= 0; --i) {
                apReturnArray->RemoveFast(indicesToRemove.GetAt(i), 1u);
            }
        }
    }
}

bool NavMeshGrid::Load(const char* apBuffer, uint32_t aiBufferSize, TESFile* apFile) {
    uint32_t uiCopy = 36;
    ourMemcpy(this, apBuffer, 36u);
    if (apFile->QEndian()) {
        ENDIAN<uint32_t>(iGridSize, false);
        ENDIAN<float>(fColumnSectionLen, false);
        ENDIAN<float>(fRowSectionLen, false);
        ENDIAN<float>(GridBoundsMin.x, false);
        ENDIAN<float>(GridBoundsMin.y, false);
        ENDIAN<float>(GridBoundsMin.z, false);
        ENDIAN<float>(GridBoundsMax.x, false);
        ENDIAN<float>(GridBoundsMax.y, false);
        ENDIAN<float>(GridBoundsMax.z, false);
    }
    if (iGridSize) {
        uint32_t uiTotalGridSize = iGridSize * iGridSize;
        // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and
        // Clear's delete[] goes through it.
        GridData = new BSSimpleArray<uint16_t, 1024>[uiTotalGridSize];
        // The rest of the buffer is untyped bytes: per cell a count, then that many triangle indices, all uint16_t,
        // which are swapped in place in the caller's buffer.
        uint16_t* pBuffer = reinterpret_cast<uint16_t*>(const_cast<char*>(&apBuffer[uiCopy]));
        for (uint32_t i = 0; i < uiTotalGridSize; ++i) {
            uint16_t usSize = *pBuffer++;
            if (apFile->QEndian()) {
                ENDIAN<uint16_t>(usSize, false);
            }
            GridData[i].SetSize(usSize, true);
            if (usSize) {
                if (apFile->QEndian()) {
                    for (uint16_t k = 0; k < static_cast<int32_t>(usSize); ++k) {
                        ENDIAN<uint16_t>(pBuffer[k], false);
                    }
                }
                ourMemcpy(&GridData[i].GetAt(0), pBuffer, sizeof(uint16_t) * usSize);
                pBuffer += usSize;
            }
        }
        return true;
    }

    GridData = nullptr;
    return false;
}

void NavMeshGrid::Clear() {
    if (GridData) {
        BSSimpleArray<uint16_t, 1024>* pGridData = GridData;
        if (pGridData) {
            // This is the delete[] of an array of a class with a virtual destructor: when the stored element count is
            // set the elements are destroyed through delete[], otherwise the bare block is freed.
            if (reinterpret_cast<size_t*>(pGridData)[-1]) {
                delete[] pGridData;
            }
            else {
                ourFree(reinterpret_cast<size_t*>(pGridData) - 1);
            }
        }
        GridData = nullptr;
    }
    iGridSize = 0;
    GridBoundsMin.x = std::numeric_limits<float>::max();
}

void NavMeshGrid::SetBoundingExtents(const NiPoint3& aMin, const NiPoint3& aMax, uint32_t aiNumTriangles) {
    GridBoundsMin = aMin;
    GridBoundsMax = aMax;
    NiPoint3 sections = GridBoundsMax - GridBoundsMin;
    if (aiNumTriangles > 16) {
        iGridSize = aiNumTriangles / 50 + 2;
        BSClampUInt32(&iGridSize, 2u, 12u);
    }
    else {
        iGridSize = 1;
    }
    sections /= static_cast<float>(iGridSize);
    fColumnSectionLen = std::fabs(sections.x);
    fRowSectionLen = std::fabs(sections.y);
    uint32_t uiTotal = iGridSize * iGridSize;
    // A real new[], as in Load.
    GridData = new BSSimpleArray<uint16_t, 1024>[uiTotal];
}

void NavMeshGrid::CopyTo(NavMeshGrid* apGrid) const {
    apGrid->Clear();
    apGrid->iGridSize = iGridSize;
    apGrid->fColumnSectionLen = fColumnSectionLen;
    apGrid->fRowSectionLen = fRowSectionLen;
    apGrid->GridBoundsMin = GridBoundsMin;
    apGrid->GridBoundsMax = GridBoundsMax;
    uint32_t uiTotalGridSize = apGrid->iGridSize * apGrid->iGridSize;
    // A real new[], as in Load.
    apGrid->GridData = new BSSimpleArray<uint16_t, 1024>[uiTotalGridSize];
    for (uint32_t i = 0; i < uiTotalGridSize; ++i) {
        GridData[i].CopyTo(&apGrid->GridData[i]);
    }
}
