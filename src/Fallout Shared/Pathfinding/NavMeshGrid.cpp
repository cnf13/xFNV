#include "Fallout Shared/Pathfinding/NavMeshGrid.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdint>

NavMeshGrid::NavMeshGrid() : iGridSize(0), GridData(nullptr) {
    GridBoundsMin.x = 3.4028235e+38f;
}

BSSimpleArray<uint16_t, 1024>* NavMeshGrid::GetTrianglesForGridCell(uint32_t aiIndex) {
    if (aiIndex < iGridSize * iGridSize) {
        return &GridData[aiIndex];
    }
    else {
        return nullptr;
    }
}

bool NavMeshGrid::IsTriangleInGridCell(uint16_t aiTriangle, uint32_t aiIndex) {
    return GridData[aiIndex].IsInArray(aiTriangle);
}

bool NavMeshGrid::IsTriangleInGridCell(uint16_t aiTriangle, uint32_t aiColumn, uint32_t aiRow) {
    return IsTriangleInGridCell(aiTriangle, aiColumn + iGridSize * aiRow);
}

// Takes z first, then computes y and x (each rounded to float) in that order.
NiPoint3* NavMeshGrid::GetGridPosition(NiPoint3* apResult, uint32_t aiColumn, uint32_t aiRow) {
    float fZ = GridBoundsMin.z;
    float fY = (float)((double)aiRow * fRowSectionLen + GridBoundsMin.y);
    float fX = (float)((double)aiColumn * fColumnSectionLen + GridBoundsMin.x);
    ::new (apResult) NiPoint3(fX, fY, fZ);
    return apResult;
}

void NavMeshGrid::GetTriangleData(const NiPoint3& aLoc, const BSSimpleArray<uint16_t, 1024>*& apArray) {
    if (!(GridBoundsMin.x > static_cast<double>(aLoc.x)) &&
        (!(GridBoundsMax.x < static_cast<double>(aLoc.x)) &&
         (!(GridBoundsMin.y > static_cast<double>(aLoc.y)) && !(GridBoundsMax.y < static_cast<double>(aLoc.y))))) {
        float v8 = aLoc.x - GridBoundsMin.x;
        float v6 = aLoc.y - GridBoundsMin.y;
        float v7 = v8 / fColumnSectionLen;
        float v5 = v6 / fRowSectionLen;
        uint32_t aiColumn = static_cast<int64_t>(std::floorf(v7));
        uint32_t aiRow = static_cast<int64_t>(std::floorf(v5));
        if (aiColumn >= iGridSize) {
            aiColumn = iGridSize - 1;
        }
        if (aiRow >= iGridSize) {
            aiRow = iGridSize - 1;
        }
        apArray = GetTriangleArray(aiColumn, aiRow);
    }
    else {
        apArray = nullptr;
    }
}

// The goal cell is computed from aLocStart too, as the game does: aLocEnd is never read.
void NavMeshGrid::GetTriangleDataForAABB(
    const NiPoint3& aLocStart,
    const NiPoint3& aLocEnd,
    BSSimpleArray<uint16_t, 1024>* apReturnArray
) {
    int32_t igoalcolumn;
    using indicesToRemove_t = BSSimpleArray<uint32_t, 1024>;
    int32_t istartcolumn;
    int32_t istartrow;
    int32_t igoalrow;
    float flocX = aLocStart.x - GridBoundsMin.x;
    float flocY = aLocStart.y - GridBoundsMin.y;
    float fscaleX = flocX / fColumnSectionLen;
    float fscaleY = flocY / fRowSectionLen;
    istartcolumn = static_cast<int32_t>(std::floorf(fscaleX));
    istartrow = static_cast<int32_t>(std::floorf(fscaleY));
    BSClamp<int32_t>(istartrow, 0, static_cast<int32_t>(iGridSize - 1));
    BSClamp<int32_t>(istartcolumn, 0, static_cast<int32_t>(iGridSize - 1));
    flocX = aLocStart.x - GridBoundsMin.x;
    flocY = aLocStart.y - GridBoundsMin.y;
    fscaleX = flocX / fColumnSectionLen;
    fscaleY = flocY / fRowSectionLen;
    igoalcolumn = static_cast<int32_t>(std::floorf(fscaleX));
    igoalrow = static_cast<int32_t>(std::floorf(fscaleY));
    BSClamp<int32_t>(igoalrow, 0, static_cast<int32_t>(iGridSize - 1));
    BSClamp<int32_t>(igoalcolumn, 0, static_cast<int32_t>(iGridSize - 1));
    for (int32_t i = istartrow; i <= igoalrow; ++i) {
        for (int32_t j = istartcolumn; j <= igoalcolumn; ++j) {
            BSSimpleArray<uint16_t, 1024>* parray = GetTriangleArray(static_cast<uint32_t>(j), i);
            for (uint32_t k = 0; k < parray->iSize; ++k) {
                apReturnArray->Add(parray->GetAt(k));
            }
        }
    }
    if (!apReturnArray->IsEmpty()) {
        apReturnArray->Sort(&fnvd::callFunction<&CompareUShort>);
        indicesToRemove_t indicesToRemove;
        for (uint32_t i_2 = 0;; ++i_2) {
            uint32_t FormFlags = apReturnArray->iSize;
            if (i_2 >= FormFlags - 1) {
                break;
            }
            int32_t v5 = apReturnArray->GetAt(i_2 + 1);
            if (v5 == apReturnArray->GetAt(i_2)) {
                uint32_t uiIndex = i_2 + 1;
                indicesToRemove.Add(uiIndex);
            }
        }
        if (!indicesToRemove.IsEmpty()) {
            for (int32_t i_3 = indicesToRemove.iSize - 1; i_3 >= 0; --i_3) {
                apReturnArray->RemoveFast(indicesToRemove.GetAt(i_3), 1u);
            }
        }
    }
}

bool NavMeshGrid::Load(const char* apBuffer, uint32_t aiBufferSize, TESFile* apFile) {
    uint32_t icopy = 36;
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
        BSSimpleArray<uint16_t, 1024>* v9;
        uint32_t itotalGridSize = iGridSize * iGridSize;
        // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and
        // Clear's delete[] goes through it.
        v9 = new BSSimpleArray<uint16_t, 1024>[itotalGridSize];
        GridData = v9;
        // The rest of the buffer is untyped bytes: per cell a count, then that many triangle indices, all uint16_t,
        // which are swapped in place in the caller's buffer.
        uint16_t* pbuffer = reinterpret_cast<uint16_t*>(const_cast<char*>(&apBuffer[icopy]));
        for (uint32_t i = 0; i < itotalGridSize; ++i) {
            uint16_t isize = *pbuffer++;
            if (apFile->QEndian()) {
                ENDIAN<uint16_t>(isize, false);
            }
            GridData[i].SetSize(isize, true);
            if (isize) {
                if (apFile->QEndian()) {
                    for (uint16_t k = 0; k < static_cast<int32_t>(isize); ++k) {
                        ENDIAN<uint16_t>(pbuffer[k], false);
                    }
                }
                size_t v8 = sizeof(uint16_t) * isize;
                uint16_t* v7 = pbuffer;
                uint16_t* v6 = &GridData[i].GetAt(0);
                ourMemcpy(v6, v7, v8);
                pbuffer += isize;
            }
        }
        return true;
    }
    else {
        GridData = nullptr;
        return false;
    }
}

void NavMeshGrid::Clear() {
    if (this->GridData) {
        BSSimpleArray<uint16_t, 1024>* GridData = this->GridData;
        if (GridData) {
            // This is the delete[] of an array of a class with a virtual destructor: when the stored element count is
            // set the elements are destroyed through delete[], otherwise the bare block is freed.
            if (reinterpret_cast<size_t*>(GridData)[-1]) {
                delete[] GridData;
            }
            else {
                ourFree(reinterpret_cast<size_t*>(GridData) - 1);
            }
        }
        this->GridData = nullptr;
    }
    iGridSize = 0;
    GridBoundsMin.x = 3.4028235e+38f;
}

void NavMeshGrid::SetBoundingExtents(const NiPoint3& aMin, const NiPoint3& aMax, uint32_t aiNumTriangles) {
    BSSimpleArray<uint16_t, 1024>* v6;
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
    sections.operator/=(static_cast<float>(iGridSize));
    fColumnSectionLen = std::fabs(sections.x);
    fRowSectionLen = std::fabs(sections.y);
    uint32_t itotal = iGridSize * iGridSize;
    // A real new[], as in Load.
    v6 = new BSSimpleArray<uint16_t, 1024>[itotal];
    GridData = v6;
}

void NavMeshGrid::CopyTo(NavMeshGrid* apGrid) const {
    BSSimpleArray<uint16_t, 1024>* v3;
    apGrid->Clear();
    apGrid->iGridSize = iGridSize;
    apGrid->fColumnSectionLen = fColumnSectionLen;
    apGrid->fRowSectionLen = fRowSectionLen;
    apGrid->GridBoundsMin = GridBoundsMin;
    apGrid->GridBoundsMax = GridBoundsMax;
    uint32_t v7 = apGrid->iGridSize * apGrid->iGridSize;
    // A real new[], as in Load.
    v3 = new BSSimpleArray<uint16_t, 1024>[v7];
    apGrid->GridData = v3;
    for (uint32_t i = 0; i < v7; ++i) {
        GridData[i].CopyTo(&apGrid->GridData[i]);
    }
}
