#include "Fallout Shared/MultiBoundMarkerData.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundShape.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

MultiBoundMarkerData::MultiBoundMarkerData() : HalfExtents(MultiBoundMarkerData::DefHalfExtents) {}

const NiPoint3& MultiBoundMarkerData::GetHalfExtents() {
    return HalfExtents;
}

// Reads the XMBO chunk (the half extents) of apFile's current record; byte swaps it from a big-endian file. False
// when there is no file or the chunk is not XMBO.
bool MultiBoundMarkerData::Load(TESFile* apFile) {
    bool bLoaded = false;
    if (apFile && apFile->GetTESChunk() == XMBO_ID) {
        apFile->GetChunkData(this, 0xC);
        if (apFile->QEndian()) {
            ENDIAN(HalfExtents.x, false);
            ENDIAN(HalfExtents.y, false);
            ENDIAN(HalfExtents.z, false);
        }
        bLoaded = true;
    }
    return bLoaded;
}

void MultiBoundMarkerData::Copy(MultiBoundMarkerData* apCopy) {
    if (apCopy) {
        *this = *apCopy;
    }
}

void MultiBoundMarkerData::Save() {
    if (!IsDefault()) {
        TESForm::AddChunkArray(XMBO_ID, &HalfExtents.x, 3u);
    }
}

bool MultiBoundMarkerData::Compare(MultiBoundMarkerData* apCompare) {
    return !apCompare || HalfExtents.operator!=(apCompare->HalfExtents);
}

bool MultiBoundMarkerData::IsDefault() {
    return !HalfExtents.operator!=(DefHalfExtents);
}

BSMultiBound* MultiBoundMarkerData::CreateMultiBoundForRef(TESObjectREFR* apRef) {
    const NiPoint3* rotation;
    const NiPoint3* angleOnReference;
    const NiPoint3* v3;
    const NiPoint3* v4;
    NiMatrix3* localRotation;
    const NiPoint3* multiBoundHalfExtent;
    const NiPoint3* v7;
    BSMultiBoundOBB* v10;
    BSMultiBoundAABB* v11;
    void* v12;
    NiMatrix3 result;
    void* v14;
    void* apMemory;
    BSMultiBoundAABB* v18;
    int32_t v20;
    BSMultiBound* v9 = nullptr;
    if (apRef) {
        v18 = nullptr;
        rotation = &apRef->GetAngleOnReference();
        if ((!(std::fabs(rotation->x) < 0.009999999776482582)) ||
            (((angleOnReference = &apRef->GetAngleOnReference()),
              (!(std::fabs(angleOnReference->y) < 0.009999999776482582))) ||
             ((v3 = &apRef->GetAngleOnReference()), (!(std::fabs(v3->z) < 0.009999999776482582))))) {
            v14 = NiMemObject::operator new(sizeof(BSMultiBoundOBB));
            v20 = 1;
            if (v14) {
                v10 = ::new (v14) BSMultiBoundOBB;
            }
            else {
                v10 = nullptr;
            }
            v20 = (-1);
            v18 = v10;
            localRotation = &(result = apRef->GetOrientation());
            static_cast<BSMultiBoundOBB*>(v18)->SetAxis(*localRotation);
            multiBoundHalfExtent = &apRef->GetMultiBoundHalfExtent();
            v18->SetHalfExtents(*multiBoundHalfExtent);
        }
        else {
            apMemory = NiMemObject::operator new(sizeof(BSMultiBoundAABB));
            v20 = 0;
            if (apMemory) {
                v11 = ::new (apMemory) BSMultiBoundAABB;
            }
            else {
                v11 = nullptr;
            }
            v20 = (-1);
            v18 = v11;
            v4 = &apRef->GetMultiBoundHalfExtent();
            v11->SetHalfExtents(*v4);
        }
        v7 = &apRef->GetLocationOnReference();
        v18->SetCenter(*v7);
        v12 = NiMemObject::operator new(sizeof(BSMultiBound));
        v20 = 2;
        if (v12) {
            v9 = ::new (v12) BSMultiBound;
        }
        else {
            v9 = nullptr;
        }
        v20 = (-1);
        v9->SetShape(v18);
    }
    return v9;
}

BSMultiBoundRoom* MultiBoundMarkerData::CreateRoomForRef(TESObjectREFR* apRef) {
    BSMultiBoundRoom* pbound = nullptr;
    if (apRef) {
        BSMultiBound* pmb;
        BSMultiBoundRoom* pbound_2;
        BSMultiBoundShape* pshape = nullptr;
        if (!(std::fabs((&apRef->GetAngleOnReference())->x) < 0.009999999776482582) ||
            (!(std::fabs((&apRef->GetAngleOnReference())->y) < 0.009999999776482582) ||
             !(std::fabs((&apRef->GetAngleOnReference())->z) < 0.009999999776482582))) {
            BSMultiBoundShape* pshape_3;
            BSMultiBoundOBB* v13 = static_cast<BSMultiBoundOBB*>(NiMemObject::operator new(sizeof(BSMultiBoundOBB)));
            if (v13) {
                pshape_3 = ::new (v13) BSMultiBoundOBB;
            }
            else {
                pshape_3 = nullptr;
            }
            pshape = pshape_3;
            static_cast<BSMultiBoundOBB*>(pshape)->BSMultiBoundOBB::SetAxis(apRef->GetOrientation());
            const NiPoint3* MultiBoundHalfExtent = &apRef->GetMultiBoundHalfExtent();
            static_cast<BSMultiBoundAABB*>(pshape)->BSMultiBoundAABB::SetHalfExtents(*MultiBoundHalfExtent);
        }
        else {
            BSMultiBoundShape* pshape_2;
            void* apMemory = NiMemObject::operator new(sizeof(BSMultiBoundAABB));
            if (apMemory) {
                pshape_2 = ::new (apMemory) BSMultiBoundAABB;
            }
            else {
                pshape_2 = nullptr;
            }
            pshape = pshape_2;
            const NiPoint3* v1 = &apRef->GetMultiBoundHalfExtent();
            static_cast<BSMultiBoundAABB*>(pshape_2)->BSMultiBoundAABB::SetHalfExtents(*v1);
        }
        const NiPoint3* v4 = &apRef->GetLocationOnReference();
        pshape->SetCenter(*v4);
        BSMultiBoundRoom* v11 = static_cast<BSMultiBoundRoom*>(NiMemObject::operator new(sizeof(BSMultiBoundRoom)));
        if (v11) {
            pbound_2 = ::new (v11) BSMultiBoundRoom;
        }
        else {
            pbound_2 = nullptr;
        }
        pbound = pbound_2;
        BSMultiBound* v10 = static_cast<BSMultiBound*>(NiMemObject::operator new(sizeof(BSMultiBound)));
        if (v10) {
            pmb = ::new (v10) BSMultiBound;
        }
        else {
            pmb = nullptr;
        }
        pmb->SetShape(pshape);
        pbound->SetMultiBound(pmb);
    }
    return pbound;
}

bool MultiBoundMarkerData::MultiBoundIntersectsCell(TESObjectREFR* apRef, int32_t aiCellX, int32_t aiCellY) {
    if (apRef && apRef->IsMultiBoundRef()) {
        NiPoint3 kY;
        NiPoint3 kZ;
        NiPoint3 boundverts[8];
        tagRECT cellrect;
        NiPoint3 kX;
        // The box's four corners and the three edges out of each, as vertex indices: 32 signed bytes, the last 16 zero.
        int8_t edges[32];
        // A cell is 4096 units wide.
        cellrect.left = aiCellX << 12;
        cellrect.right = (aiCellX << 12) + 4096;
        cellrect.bottom = aiCellY << 12;
        cellrect.top = (aiCellY << 12) + 4096;
        rt::eh_vector_constructor_iterator(boundverts, sizeof(NiPoint3), 8, &emptyPointConstructor);
        NiPoint3 ext = apRef->GetMultiBoundHalfExtent();
        NiPoint3 pos = apRef->GetLocationOnReference();
        NiMatrix3 rot = apRef->GetOrientation();
        rot.GetCol(0, kX);
        kX.operator*=(ext.x);
        rot.GetCol(1u, kY);
        kY.operator*=(ext.y);
        rot.GetCol(2u, kZ);
        kZ.operator*=(ext.z);
        boundverts[0] = pos + kX + kY + kZ;
        boundverts[1] = pos + kX + kY - kZ;
        boundverts[2] = pos + kX - kY + kZ;
        boundverts[3] = pos + kX - kY - kZ;
        boundverts[4] = pos - kX + kY + kZ;
        boundverts[5] = pos - kX + kY - kZ;
        boundverts[6] = pos - kX - kY + kZ;
        boundverts[7] = pos - kX - kY - kZ;
        edges[0] = 0;
        edges[1] = 1;
        edges[2] = 2;
        edges[3] = 4;
        edges[4] = 3;
        edges[5] = 1;
        edges[6] = 2;
        edges[7] = 7;
        edges[8] = 5;
        edges[9] = 1;
        edges[10] = 4;
        edges[11] = 7;
        edges[12] = 6;
        edges[13] = 2;
        edges[14] = 4;
        edges[15] = 7;
        for (int32_t k = 16; k < 32; ++k) {
            edges[k] = 0;
        }
        for (int32_t i = 0; i < 4; ++i) {
            NiPoint3 startpt = boundverts[edges[4 * i]];
            for (int32_t j = 1; j < 4; ++j) {
                NiPoint3 ignore;
                NiPoint3 endpt = boundverts[edges[4 * i + j]];
                if (LineVsRect(cellrect, startpt, endpt, ignore)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void BSMultiBoundOBB::SetAxis(const NiMatrix3& arAxis) {
    Axis = arAxis;
}

void NiAVObject::AttachProperty(NiProperty* pkProperty) {
    if (pkProperty->Type() < static_cast<int32_t>(NiProperty::MAX_TYPES)) {
        NiPointer<NiProperty> spProperty(pkProperty);
        m_kPropertyList.AddHead(spProperty);
    }
}

NiAlphaProperty::~NiAlphaProperty() {}

NiProperty::NiProperty() {}

void NiTAbstractPoolAllocator<uint32_t>::CreateFreeMem() {
    uint32_t v0;
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem =
        static_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(ourMalloc(
            NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize <=
                    SIZE_MAX / sizeof(NiTAbstractPoolAllocator<uint32_t>::AllocNode)
                ? sizeof(NiTAbstractPoolAllocator<uint32_t>::AllocNode) *
                    NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize
                : SIZE_MAX
        ));
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem[NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize - 1].m_pkNext =
        nullptr;
    uint32_t i = 1;
    do {
        NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem[i].m_pkNext =
            &NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem[i + 1];
        v0 = i++;
    }
    while (v0 < NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize - 2);
    if (NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader) {
        NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem->m_pkNext =
            NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader;
        NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
    }
    else {
        NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
        NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem->m_pkNext = nullptr;
    }
    ++NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
}
