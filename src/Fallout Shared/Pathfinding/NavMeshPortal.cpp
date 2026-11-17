#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

bool NavMeshTriangleEdgePortal::IsValid() const {
    return (bool)((uint8_t)(pOtherMesh && (sTriangle != 65535)));
}

void NavMeshTriangleEdgePortal::Load(TESForm* apOwner, TESFile* apFile, char* pBuffer) {
    uint32_t uiFormID = *reinterpret_cast<uint32_t*>(pBuffer);
    if (apFile->QEndian()) {
        ENDIAN(uiFormID, false);
    }
    TESForm::AddCompileIndex(uiFormID, apFile);
    if (uiFormID) {
        pOtherMesh = TES::pInstance->GetNavMeshInfoMap()->GetNavMeshInfo(uiFormID);
    }
    else {
        pOtherMesh = nullptr;
    }
    pBuffer += 4;
    sTriangle = *reinterpret_cast<uint16_t*>(pBuffer);
    if (apFile->QEndian()) {
        ENDIAN(sTriangle, false);
    }
}
