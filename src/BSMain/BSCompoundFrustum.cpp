#include "BSMain/BSCompoundFrustum.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

#include <cmath>

BSCompoundFrustum::BSCompoundFrustum()
  : bPrethreaded(false), iFreePlane(0), iFreeOp(0), iFirstOp(0), kViewPosition(NiPoint3_ZERO), bSkipViewFrustum(false) {
    kPlanes.SetSize(4u, true);
    kFunctionOperators.SetSize(10u, true);
}

BSCompoundFrustum::~BSCompoundFrustum() {
    kPlanes.Clear(true);
    kFunctionOperators.Clear(true);
}

void BSCompoundFrustum::ShiftOps(int32_t aiShift, bool abFillNoop) {
    if (aiShift <= 0) {
        if (aiShift < 0) {
            if (iFreeOp <= -aiShift) {
                iFreeOp = 0;
            }
            else {
                for (int32_t i = 0; i < iFreeOp + aiShift; ++i) {
                    kFunctionOperators.pBuffer[i].op = kFunctionOperators.pBuffer[i - aiShift].op;
                }
                iFreeOp += aiShift;
            }
        }
    }
    else {
        int32_t j;
        if (aiShift + iFreeOp >= static_cast<int32_t>(kFunctionOperators.iReservedSize)) {
            int32_t v3;
            if (aiShift <= 10) {
                v3 = 10;
            }
            else {
                v3 = aiShift;
            }
            kFunctionOperators.SetSize(v3 + kFunctionOperators.iReservedSize, true);
        }
        for (j = iFreeOp + aiShift - 1; j >= aiShift; --j) {
            kFunctionOperators.pBuffer[j].op = kFunctionOperators.pBuffer[j - aiShift].op;
        }
        if (abFillNoop) {
            while (j > 0) {
                kFunctionOperators.pBuffer[j--].op = 0;
            }
        }
        iFreeOp += aiShift;
    }
}

void BSCompoundFrustum::PrethreadOpList() {
    if (!bPrethreaded && iFreeOp > 0) {
        int32_t ipass = iFreeOp + 1;
        int32_t ifail = iFreeOp + 2;
        if (iFreeOp + 3 >= static_cast<int32_t>(kFunctionOperators.iReservedSize)) {
            kFunctionOperators.SetSize(kFunctionOperators.iReservedSize + 10, true);
        }
        kFunctionOperators.pBuffer[iFreeOp].op = 1;
        kFunctionOperators.pBuffer[iFreeOp + 1].op = 2;
        kFunctionOperators.pBuffer[iFreeOp + 2].op = 3;
        int32_t itempArraySize = iFreeOp / 2;
        ScrapHeap* pScrapHeap = MemoryManager_Singleton.GetThreadScrapHeap();
        int32_t* pgroupStack =
            static_cast<int32_t*>(pScrapHeap->Allocate(sizeof(int32_t) * itempArraySize, ScrapHeap::ALIGNMENT));
        *pgroupStack = 0;
        int32_t v13 = 0;
        int32_t v20 = 1;
        int32_t* pgroupEnds =
            static_cast<int32_t*>(pScrapHeap->Allocate(sizeof(int32_t) * itempArraySize, ScrapHeap::ALIGNMENT));
        int32_t* pgroupIndex =
            static_cast<int32_t*>(pScrapHeap->Allocate(sizeof(int32_t) * itempArraySize, ScrapHeap::ALIGNMENT));
        int32_t i = 0;
        while (i < iFreeOp + 1) {
            switch (kFunctionOperators.pBuffer[i].op) {
                case 1: {
                    *pgroupEnds = i;
                    if (!v13) {
                        goto LABEL_6;
                    }
                    iFreeOp = 0;
                    return;
                }
                case 4:
                case 5: {
                    pgroupStack[++v13] = v20++;
                    goto LABEL_6;
                }
                case 6: {
                    pgroupEnds[pgroupStack[v13--]] = i;
                    goto LABEL_6;
                }
                case 7:
                case 8: {
                    ++i;
                    goto LABEL_6;
                }
                default: {
                LABEL_6:;
                    ++i;
                    break;
                }
            }
        }
        std::memset(pgroupStack, 0, sizeof(int32_t) * itempArraySize);
        *pgroupStack = 0;
        *pgroupIndex = 0;
        int32_t v14 = 1;
        int32_t v21 = 1;
        bool bfoundfirst = false;
        for (int32_t i_2 = 0; i_2 < iFreeOp + 1; ++i_2) {
            switch (kFunctionOperators.pBuffer[i_2].op) {
                case 1: {
                    kFunctionOperators.pBuffer[i_2].jp = ipass;
                    kFunctionOperators.pBuffer[i_2].jf = ifail;
                    break;
                }
                case 4:
                case 5: {
                    pgroupStack[v14] = kFunctionOperators.pBuffer[i_2].op;
                    pgroupIndex[v14++] = v21++;
                    kFunctionOperators.pBuffer[i_2].jp = i_2 + 1;
                    kFunctionOperators.pBuffer[i_2].jf = i_2 + 1;
                    break;
                }
                case 6: {
                    int32_t v3 = pgroupStack[--v14 - 1];
                    if (v3) {
                        if (v3 == 4) {
                            kFunctionOperators.pBuffer[i_2].jp = i_2 + 1;
                            kFunctionOperators.pBuffer[i_2].jf = pgroupEnds[pgroupIndex[v14 - 1]];
                        }
                        else {
                            if (v3 == 5) {
                                kFunctionOperators.pBuffer[i_2].jp = pgroupEnds[pgroupIndex[v14 - 1]];
                                kFunctionOperators.pBuffer[i_2].jf = i_2 + 1;
                            }
                        }
                    }
                    else {
                        kFunctionOperators.pBuffer[i_2].jp = i_2 + 1;
                        kFunctionOperators.pBuffer[i_2].jf = i_2 + 1;
                    }
                    break;
                }
                case 7:
                case 8: {
                    if (!bfoundfirst) {
                        iFirstOp = i_2;
                        bfoundfirst = true;
                    }
                    int32_t v2 = pgroupStack[v14 - 1];
                    if (v2) {
                        if (v2 == 4) {
                            kFunctionOperators.pBuffer[i_2].jp = i_2 + 2;
                            kFunctionOperators.pBuffer[i_2].jf = pgroupEnds[pgroupIndex[v14 - 1]];
                        }
                        else {
                            if (v2 == 5) {
                                kFunctionOperators.pBuffer[i_2].jp = pgroupEnds[pgroupIndex[v14 - 1]];
                                kFunctionOperators.pBuffer[i_2].jf = i_2 + 2;
                            }
                        }
                    }
                    else {
                        kFunctionOperators.pBuffer[i_2].jp = iFreeOp;
                        kFunctionOperators.pBuffer[i_2].jf = iFreeOp;
                    }
                    ++i_2;
                    break;
                }
                default: {
                    continue;
                }
            }
        }
        if (!bfoundfirst) {
            Warning("BSCompoundFrustum : prethreading an effectively empty op list");
            iFirstOp = ipass;
            bPrethreaded = true;
            return;
        }
        for (int32_t i_3 = iFirstOp; i_3 < iFreeOp; ++i_3) {
            int32_t op = kFunctionOperators.pBuffer[i_3].op;
            if (op >= 7 && op <= 8) {
                while (true) {
                    int32_t idestop = kFunctionOperators.pBuffer[kFunctionOperators.pBuffer[i_3].jp].op;
                    if (idestop == 7 || (idestop == 8 || (idestop == 2 || idestop == 3))) {
                        break;
                    }
                    kFunctionOperators.pBuffer[i_3].jp =
                        kFunctionOperators.pBuffer[kFunctionOperators.pBuffer[i_3].jp].jp;
                }
                while (true) {
                    int32_t idestop_2 = kFunctionOperators.pBuffer[kFunctionOperators.pBuffer[i_3].jf].op;
                    if (idestop_2 == 7 || (idestop_2 == 8 || (idestop_2 == 2 || idestop_2 == 3))) {
                        break;
                    }
                    kFunctionOperators.pBuffer[i_3].jf =
                        kFunctionOperators.pBuffer[kFunctionOperators.pBuffer[i_3].jf].jf;
                }
                ++i_3;
            }
        }
        pScrapHeap->Deallocate(pgroupStack);
        pScrapHeap->Deallocate(pgroupEnds);
        pScrapHeap->Deallocate(pgroupIndex);
    }
    bPrethreaded = true;
}

void BSCompoundFrustum::SetCamera(const NiCamera* apCamera) {
    if (apCamera) {
        kViewFrustum.Set(apCamera->m_kViewFrustum, apCamera->m_kWorld);
        kViewPosition.x = apCamera->m_kWorld.m_Translate.x;
        kViewPosition.y = apCamera->m_kWorld.m_Translate.y;
        kViewPosition.z = apCamera->m_kWorld.m_Translate.z;
    }
    iFreePlane = 0;
    iFreeOp = 0;
    bPrethreaded = false;
    iFirstOp = 0;
    bSkipViewFrustum = false;
}

void BSCompoundFrustum::StartGroupIntersection() {
    if ((iFreeOp + 1) >= ((int32_t)kFunctionOperators.iReservedSize)) {
        (&kFunctionOperators)->SetSize(((int32_t)kFunctionOperators.iReservedSize) + 10, true);
    }
    kFunctionOperators.pBuffer[iFreeOp++].op = 4;
}

void BSCompoundFrustum::EndGroup() {
    if ((iFreeOp + 1) >= ((int32_t)kFunctionOperators.iReservedSize)) {
        (&kFunctionOperators)->SetSize(((int32_t)kFunctionOperators.iReservedSize) + 10, true);
    }
    bPrethreaded = false;
    kFunctionOperators.pBuffer[iFreeOp++].op = 6;
}

void BSCompoundFrustum::AddPortal(BSPortal* apPortal, bool abFinalRoom) {
    if ((iFreeOp + 2) >= ((int32_t)kFunctionOperators.iReservedSize)) {
        (&kFunctionOperators)->SetSize(((int32_t)kFunctionOperators.iReservedSize) + 10, true);
    }
    if ((iFreePlane + 1) >= ((int32_t)kPlanes.iReservedSize)) {
        (&kPlanes)->SetSize(((int32_t)kPlanes.iReservedSize) + 4, true);
    }
    NiPoint3 kVertices[4];
    apPortal->GetVertices(kVertices);
    // The plane's storage: only the three-point constructor is run on it.
    alignas(NiPlane) uint8_t kPlaneMemory[sizeof(NiPlane)];
    NiPlane* pPlane = ::new (kPlaneMemory) NiPlane(&kVertices[0], &kVertices[1], &kVertices[2]);
    kFunctionOperators.pBuffer[iFreeOp++].op = 7;
    kFunctionOperators.pBuffer[iFreeOp++].op = iFreePlane;
    apPortal->GetFrustumPlanes(kViewPosition, kPlanes.pBuffer[iFreePlane]);
    float fDistance = pPlane->m_kNormal.Dot(kViewPosition) - pPlane->m_fConstant;
    float fAbsDistance = (float)std::fabs((double)fDistance);
    // Equal, less or unordered clears the planes.
    if (fAbsDistance > 5.0) {
        kPlanes.pBuffer[iFreePlane].m_uiActivePlanes = 63;
    }
    else {
        kPlanes.pBuffer[iFreePlane].m_uiActivePlanes = 0;
    }
    ++iFreePlane;
}

// The free-size tests are signed although iReservedSize is unsigned.
void BSCompoundFrustum::AddOcclusionPlane(BSOcclusionPlane* apPlane) {
    BSOcclusionPlane::BSOP_LINKTYPE klink = apPlane->GetJoinedPlaneType();
    BSOcclusionPlane* potherplane;
    if (klink == BSOcclusionPlane::BSOP_LINKTYPE_PARENT) {
        potherplane = apPlane->pLinkedPlanes[apPlane->pLinkedPlanes[1] == nullptr ? 3 : 1];
    }
    else {
        potherplane = apPlane->pLinkedPlanes[apPlane->pLinkedPlanes[0] != nullptr ? 0 : 2];
    }
    bool bsameside = false;
    int32_t iside = 0;
    if (potherplane) {
        NiPoint3 kVertices[4];
        NiFrustumPlanes ktempplanes;
        apPlane->GetFrustumPlanes(kViewPosition, ktempplanes);
        apPlane->GetVertices(kVertices);
        NiPlane koccplane(&kVertices[0], &kVertices[1], &kVertices[2]);
        iside = koccplane.WhichSide(kViewPosition);
        potherplane->GetFrustumPlanes(kViewPosition, ktempplanes);
        potherplane->GetVertices(kVertices);
        // The other plane is built in a temporary and copied over koccplane.
        koccplane = NiPlane(&kVertices[0], &kVertices[1], &kVertices[2]);
        bsameside = iside == koccplane.WhichSide(kViewPosition);
    }
    // This one-plane add covers two cases: a culled or opposite-side neighbour, and an unlinked plane on the same side.
    if ((potherplane && potherplane->kCullResult != BSMultiBoundShape::BS_CULL_VISIBLE) || !bsameside ||
        klink == BSOcclusionPlane::BSOP_LINKTYPE_NONE) {
        if (iFreeOp + 2 >= static_cast<int32_t>(kFunctionOperators.iReservedSize)) {
            kFunctionOperators.SetSize(kFunctionOperators.iReservedSize + 10, true);
        }
        if (iFreePlane + 1 >= static_cast<int32_t>(kPlanes.iReservedSize)) {
            kPlanes.SetSize(kPlanes.iReservedSize + 4, true);
        }
        kFunctionOperators.pBuffer[iFreeOp].op = 8;
        ++iFreeOp;
        kFunctionOperators.pBuffer[iFreeOp].op = iFreePlane;
        ++iFreeOp;
        apPlane->GetFrustumPlanes(kViewPosition, kPlanes.pBuffer[iFreePlane]);
        ++iFreePlane;
    }
    else if (klink == BSOcclusionPlane::BSOP_LINKTYPE_PARENT) {
        if (iFreeOp + 12 >= static_cast<int32_t>(kFunctionOperators.iReservedSize)) {
            kFunctionOperators.SetSize(kFunctionOperators.iReservedSize + 12, true);
        }
        if (iFreePlane + 4 >= static_cast<int32_t>(kPlanes.iReservedSize)) {
            kPlanes.SetSize(kPlanes.iReservedSize + 4, true);
        }
        // Each op: the index is taken and iFreeOp stored before the entry is written, as the game orders them.
        int32_t iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 4;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 8;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = iFreePlane;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 8;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = iFreePlane + 1;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 5;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 8;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = iFreePlane + 2;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 8;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = iFreePlane + 3;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 6;
        iOp = iFreeOp++;
        kFunctionOperators.pBuffer[iOp].op = 6;
        apPlane->GetFrustumPlanes(kViewPosition, kPlanes.pBuffer[iFreePlane]);
        apPlane->GetFrustumPlanes(kViewPosition, kPlanes.pBuffer[iFreePlane + 2]);
        int32_t ichild = apPlane->pLinkedPlanes[1] == nullptr ? 3 : 1;
        int32_t idisable = 0;
        // iside is set to 1 here, so the side test below is constant, as the game does.
        iside = 1;
        if (ichild == 1) {
            idisable = (iside != 1) + 2;
        }
        else {
            idisable = 2 * (iside != 1) + 3;
        }
        NiFrustumPlanes* pPlanes = &kPlanes.pBuffer[iFreePlane + 2];
        pPlanes->m_uiActivePlanes &= ~(1 << idisable);
        BSOcclusionPlane* pchildplane = apPlane->pLinkedPlanes[ichild];
        pchildplane->GetFrustumPlanes(kViewPosition, kPlanes.pBuffer[iFreePlane + 1]);
        pchildplane->GetFrustumPlanes(kViewPosition, kPlanes.pBuffer[iFreePlane + 3]);
        if (ichild == 1) {
            idisable = (idisable == 2) + 2;
        }
        else {
            idisable = (idisable == 4) + 4;
        }
        pPlanes = &kPlanes.pBuffer[iFreePlane + 3];
        pPlanes->m_uiActivePlanes &= ~(1 << idisable);
        iFreePlane += 4;
    }
}

void BSCompoundFrustum::AddCompoundFrustum(BSCompoundFrustum* apFrustum) {
    if (apFrustum->iFreeOp + iFreeOp + 1 >= static_cast<int32_t>(kFunctionOperators.iReservedSize)) {
        int32_t v5;
        if (apFrustum->iFreeOp + 1 <= 10) {
            v5 = 10;
        }
        else {
            v5 = apFrustum->iFreeOp + 1;
        }
        kFunctionOperators.SetSize(v5 + kFunctionOperators.iReservedSize, true);
    }
    if (apFrustum->iFreePlane + iFreePlane + 1 >= static_cast<int32_t>(kPlanes.iReservedSize)) {
        int32_t v4;
        if (apFrustum->iFreePlane + 1 <= 4) {
            v4 = 4;
        }
        else {
            v4 = apFrustum->iFreePlane + 1;
        }
        kPlanes.SetSize(v4 + kPlanes.iReservedSize, true);
    }
    bPrethreaded = false;
    for (int32_t i = 0; i < apFrustum->iFreeOp; ++i) {
        kFunctionOperators.pBuffer[i + iFreeOp].op = apFrustum->kFunctionOperators.pBuffer[i].op;
    }
    for (int32_t j = 0; j < apFrustum->iFreePlane; ++j) {
        kPlanes.pBuffer[j + iFreePlane] = apFrustum->kPlanes.pBuffer[j];
    }
    if (iFreePlane) {
        for (int32_t k = iFreeOp; k < apFrustum->iFreeOp + iFreeOp; ++k) {
            int32_t op = kFunctionOperators.pBuffer[k].op;
            if (op >= 7 && op <= 8) {
                BSCompoundFrustum::FunctionOp* v2 = &kFunctionOperators.pBuffer[k + 1];
                v2->op += iFreePlane;
                ++k;
            }
        }
    }
    iFreeOp += apFrustum->iFreeOp;
    iFreePlane += apFrustum->iFreePlane;
}

void BSCompoundFrustum::MakeIntoUnion() {
    int32_t op;
    int32_t i;
    int32_t v4;
    int8_t v5 = 1;
    if (kFunctionOperators.pBuffer->op == 5) {
        v4 = 1;
        for (i = 1; (v4 > 0) && (i < iFreeOp); ++i) {
            op = kFunctionOperators.pBuffer[i].op;
            if (op >= 4) {
                if (op <= 5) {
                    ++v4;
                }
                else {
                    if (op == 6) {
                        --v4;
                    }
                }
            }
        }
        if ((i == iFreeOp) && (!v4)) {
            v5 = 0;
        }
    }
    if (v5) {
        if ((iFreeOp + 1) >= ((int32_t)kFunctionOperators.iReservedSize)) {
            (&kFunctionOperators)->SetSize(kFunctionOperators.iReservedSize + 10, true);
        }
        ShiftOps(1, false);
        kFunctionOperators.pBuffer->op = 5;
    }
    else {
        --iFreeOp;
    }
}

void BSCompoundFrustum::BuildForPortal(BSCompoundFrustum* apExistingFrustum, BSPortal* apPortal) {
    int32_t iopcount = apExistingFrustum->iFreeOp;
    kFunctionOperators.SetSize(iopcount, true);
    kPlanes.SetSize(apExistingFrustum->iFreePlane, true);
    for (int32_t i = 0; i < iopcount; ++i) {
        int32_t op = apExistingFrustum->kFunctionOperators.pBuffer[i].op;
        if (op < 7 || op > 8) {
            kFunctionOperators.pBuffer[i].op = apExistingFrustum->kFunctionOperators.pBuffer[i].op;
            ++iFreeOp;
        }
        else {
            bool bintersect = apPortal->WithinFrustumFullTest(
                apExistingFrustum->kPlanes.pBuffer[apExistingFrustum->kFunctionOperators.pBuffer[i + 1].op]
            );
            kFunctionOperators.pBuffer[i].op = apExistingFrustum->kFunctionOperators.pBuffer[i].op;
            ++iFreeOp;
            kFunctionOperators.pBuffer[i + 1].op = iFreePlane;
            ++iFreeOp;
            kPlanes.pBuffer[apExistingFrustum->kFunctionOperators.pBuffer[i + 1].op] =
                apExistingFrustum->kPlanes.pBuffer[apExistingFrustum->kFunctionOperators.pBuffer[i + 1].op];
            ++iFreePlane;
            if (bintersect && apPortal->QEdgeVisibleState() == 15) {
                kPlanes.pBuffer[apExistingFrustum->kFunctionOperators.pBuffer[i + 1].op].m_uiActivePlanes = 0;
            }
            ++i;
        }
    }
    iFreeOp = iopcount;
    RemoveEmptyGroups();
}

bool BSCompoundFrustum::ProcessPlane(BSOcclusionPlane* apPlane) {
    bool v8 = true;
    if (!bSkipViewFrustum) {
        v8 = apPlane->WithinFrustumDistFirst(kViewFrustum, kViewPosition);
    }
    if (v8 && iFreeOp > 0) {
        int32_t jp;
        int32_t i;
        for (i = iFirstOp; kFunctionOperators.pBuffer[i].op != 2 && kFunctionOperators.pBuffer[i].op != 3; i = jp) {
            bool v6 = false;
            int32_t op = kFunctionOperators.pBuffer[i].op;
            if (op == 7) {
                v6 =
                    apPlane->WithinFrustum(kPlanes.pBuffer[kFunctionOperators.pBuffer[i + 1].op], false, NiPoint3_ZERO);
            }
            else {
                if (op == 8) {
                    v6 = !apPlane->CompletelyWithinFrustum(kPlanes.pBuffer[kFunctionOperators.pBuffer[i + 1].op]);
                }
            }
            if (v6) {
                jp = kFunctionOperators.pBuffer[i].jp;
            }
            else {
                jp = kFunctionOperators.pBuffer[i].jf;
            }
        }
        return kFunctionOperators.pBuffer[i].op == 2;
    }
    return v8;
}

int32_t BSCompoundFrustum::QFuncOpCount() {
    int32_t i;
    int32_t v3 = 0;
    for (i = 0; i < ((int32_t)((uint32_t)iFreeOp)); ++i) {
        if (kFunctionOperators.pBuffer[i].op == 7 || kFunctionOperators.pBuffer[i].op == 8) {
            ++v3;
            ++i;
        }
    }
    return v3;
}

void BSCompoundFrustum::SetActivePlaneState(int32_t* apPlaneState) {
    int32_t i;
    kViewFrustum.m_uiActivePlanes = (uint32_t)(*apPlaneState);
    for (i = 1; (i < (iFreePlane + 1)) && (i < ((int32_t)kPlanes.iReservedSize)); ++i) {
        kPlanes.pBuffer[i - 1].m_uiActivePlanes = (uint32_t)apPlaneState[i];
    }
    ScrapHeap* threadScrapHeap = (&MemoryManager_Singleton)->GetThreadScrapHeap();
    threadScrapHeap->Deallocate(apPlaneState);
}

void BSCompoundFrustum::StartGroupUnion() {
    if ((iFreeOp + 1) >= ((int32_t)kFunctionOperators.iReservedSize)) {
        (&kFunctionOperators)->SetSize(((int32_t)kFunctionOperators.iReservedSize) + 10, true);
    }
    bPrethreaded = false;
    kFunctionOperators.pBuffer[iFreeOp++].op = 5;
}

bool BSCompoundFrustum::TestNotContained(NiBound* apBound, NiFrustumPlanes& apPlanes) {
    int32_t i;
    if (!(&apPlanes)->m_uiActivePlanes) {
        return false;
    }
    for (i = 0; i < 6; ++i) {
        if (((&apPlanes)->m_uiActivePlanes & (1 << i)) != 0) {
            if (apBound->WhichSide((&apPlanes)->m_akCullingPlanes[i]) != 1) {
                return (bool)((uint8_t)(i != 6));
            }
            (&apPlanes)->m_uiActivePlanes &= (~(1 << i));
        }
    }
    return (bool)((uint8_t)(i != 6));
}

void BSCompoundFrustum::VerifyGroups() {
    int32_t i;
    int32_t v3 = 0;
    for (i = 0; i < iFreeOp; ++i) {
        switch (kFunctionOperators.pBuffer[i].op) {
            case 4:
            case 5: {
                ++v3;
                break;
            }
            case 6: {
                --v3;
                break;
            }
            case 7:
            case 8: {
                ++i;
                break;
            }
        }
        if (v3 < 0) {
            platform::DebuggerBreak();
        }
    }
    if (v3) {
        platform::DebuggerBreak();
    }
}

void BSCompoundFrustum::RemoveEmptyGroups() {
    int32_t v2 = 0;
    int32_t iFreeOp = this->iFreeOp;
    for (int32_t i = 0; i < this->iFreeOp; ++i) {
        while (i < this->iFreeOp &&
               (this->iFreeOp > 0 &&
                ((kFunctionOperators.pBuffer[v2 + i].op == 4 || kFunctionOperators.pBuffer[v2 + i].op == 5) &&
                 kFunctionOperators.pBuffer[i + 1 + v2].op == 6))) {
            v2 += 2;
            this->iFreeOp -= 2;
        }
        if (v2 + i < iFreeOp) {
            kFunctionOperators.pBuffer[i].op = kFunctionOperators.pBuffer[v2 + i].op;
            if (kFunctionOperators.pBuffer[i].op == 7 || kFunctionOperators.pBuffer[i].op == 8) {
                kFunctionOperators.pBuffer[i + 1].op = kFunctionOperators.pBuffer[i + 1 + v2].op;
                ++i;
            }
        }
    }
}

void BSCompoundFrustum::CopyMembers(BSCompoundFrustum* apDest) {
    apDest->iFreeOp = iFreeOp;
    apDest->iFreePlane = iFreePlane;
    apDest->iFirstOp = iFirstOp;
    apDest->kViewPosition = kViewPosition;
    apDest->kViewFrustum = kViewFrustum;
    apDest->bPrethreaded = bPrethreaded;
    apDest->kFunctionOperators.SetSize(kFunctionOperators.iReservedSize, true);
    for (uint32_t i = 0; i < kFunctionOperators.iReservedSize; ++i) {
        apDest->kFunctionOperators.pBuffer[i].op = kFunctionOperators.pBuffer[i].op;
        apDest->kFunctionOperators.pBuffer[i].jp = kFunctionOperators.pBuffer[i].jp;
        apDest->kFunctionOperators.pBuffer[i].jf = kFunctionOperators.pBuffer[i].jf;
    }
    apDest->kPlanes.SetSize(kPlanes.iReservedSize, true);
    for (uint32_t j = 0; j < kPlanes.iReservedSize; ++j) {
        apDest->kPlanes.pBuffer[j] = kPlanes.pBuffer[j];
    }
}

void BSCompoundFrustum::MakeIntoIntersection() {
    char v5 = 1;
    if (kFunctionOperators.pBuffer->op == 4) {
        int32_t i;
        int32_t v4 = 1;
        for (i = 1; v4 > 0 && i < iFreeOp; ++i) {
            int32_t op = kFunctionOperators.pBuffer[i].op;
            if (op >= 4) {
                if (op <= 5) {
                    ++v4;
                }
                else {
                    if (op == 6) {
                        --v4;
                    }
                }
            }
        }
        if (i == iFreeOp && !v4) {
            v5 = 0;
        }
    }
    if (v5) {
        if (iFreeOp + 1 >= static_cast<int32_t>(kFunctionOperators.iReservedSize)) {
            kFunctionOperators.SetSize(kFunctionOperators.iReservedSize + 10, true);
        }
        ShiftOps(1, false);
        kFunctionOperators.pBuffer->op = 4;
    }
    else {
        --iFreeOp;
    }
}

int32_t* BSCompoundFrustum::GetActivePlaneState() {
    ScrapHeap* ThreadScrapHeap = MemoryManager_Singleton.GetThreadScrapHeap();
    int32_t* v5 =
        static_cast<int32_t*>(ThreadScrapHeap->Allocate(sizeof(int32_t) * kPlanes.iReservedSize, ScrapHeap::ALIGNMENT));
    *v5 = kViewFrustum.GetActivePlaneState();
    for (int32_t i = 1; i < iFreePlane + 1 && i < static_cast<int32_t>(kPlanes.iReservedSize); ++i) {
        v5[i] = kPlanes.pBuffer[i - 1].GetActivePlaneState();
    }
    return v5;
}

bool BSCompoundFrustum::Process(NiAVObject* apObject) {
    NiBound* pBound;
    NiBound Bound;
    if (!apObject->m_kWorldBound || apObject->m_kWorldBound->m_fRadius == 0.0) {
        return false;
    }
    if (apObject->m_kWorldBound) {
        pBound = apObject->m_kWorldBound;
    }
    else {
        pBound = &NiAVObject::NullBoundS;
    }
    Bound = *pBound;
    bool bResult = true;
    if (!bSkipViewFrustum) {
        bResult = TestIntersection(&Bound, kViewFrustum);
    }
    if (bResult && iFreeOp > 0) {
        int32_t jp;
        int32_t i;
        for (i = iFirstOp; kFunctionOperators.pBuffer[i].op != 2 && kFunctionOperators.pBuffer[i].op != 3; i = jp) {
            bool v8 = false;
            int32_t uiFunction = kFunctionOperators.pBuffer[i].op;
            if (uiFunction == 7) {
                v8 = TestIntersection(&Bound, kPlanes.pBuffer[kFunctionOperators.pBuffer[i + 1].op]);
            }
            else {
                if (uiFunction == 8) {
                    v8 = TestNotContained(&Bound, kPlanes.pBuffer[kFunctionOperators.pBuffer[i + 1].op]);
                }
            }
            if (v8) {
                jp = kFunctionOperators.pBuffer[i].jp;
            }
            else {
                jp = kFunctionOperators.pBuffer[i].jf;
            }
        }
        return kFunctionOperators.pBuffer[i].op == 2;
    }
    return bResult;
}

bool BSCompoundFrustum::ProcessMultiBound(BSMultiBound* apBound) {
    bool v8 = true;
    if (!bSkipViewFrustum) {
        v8 = apBound->spShape.m_pObject->WithinFrustum(kViewFrustum);
    }
    if (v8 && iFreeOp > 0) {
        int32_t jp;
        int32_t i;
        for (i = iFirstOp; kFunctionOperators.pBuffer[i].op != 2 && kFunctionOperators.pBuffer[i].op != 3; i = jp) {
            bool v6 = false;
            int32_t op = kFunctionOperators.pBuffer[i].op;
            if (op == 7) {
                v6 = apBound->spShape.m_pObject->WithinFrustum(kPlanes.pBuffer[kFunctionOperators.pBuffer[i + 1].op]);
            }
            else {
                if (op == 8) {
                    v6 = !apBound->spShape.m_pObject->CompletelyWithinFrustum(
                        kPlanes.pBuffer[kFunctionOperators.pBuffer[i + 1].op]
                    );
                }
            }
            if (v6) {
                jp = kFunctionOperators.pBuffer[i].jp;
            }
            else {
                jp = kFunctionOperators.pBuffer[i].jf;
            }
        }
        return kFunctionOperators.pBuffer[i].op == 2;
    }
    return v8;
}

bool BSCompoundFrustum::TestIntersection(NiBound* apBound, NiFrustumPlanes& apPlanes) {
    int32_t i;
    if (!(&apPlanes)->GetActivePlaneState()) {
        return true;
    }
    for (i = 0; i < 6; ++i) {
        if (((&apPlanes)->GetActivePlaneState() & 1 << i) != 0) {
            int32_t planeSide = apBound->WhichSide(apPlanes.m_akCullingPlanes[i]);
            if (planeSide == 2) {
                return i == 6;
            }
            if (planeSide == 1) {
                apPlanes.m_uiActivePlanes &= ~(1 << i);
            }
        }
    }
    return i == 6;
}

void BSCompoundFrustum::operator delete(void* apMemory) {
    ourFree(apMemory);
}
