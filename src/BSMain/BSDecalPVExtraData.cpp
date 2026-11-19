#include "BSMain/BSDecalPVExtraData.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSDecalPlacementVectorExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSDecalPlacementVectorExtraData::ms_RTTI("BSDecalPlacementVectorExtraData", &NiIntegerExtraData::ms_RTTI);

DECAL_PLACEMENT_VECTOR_GROUP_DATA::DECAL_PLACEMENT_VECTOR_GROUP_DATA() {
    usVectorCount = 0;
    for (int32_t i = 0; i < 8; i++) {
        VectorOrigin[i] = NiPoint3_ZERO;
        VectorDirection[i] = NiPoint3_ZERO;
    }
}

BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(const int32_t iValue)
  : NiIntegerExtraData(iValue), usGroupCount(0), pDecalPlacementGroups{} {}

// Eight slots, whatever usGroupCount says; each group's memory goes back to the engine's heap and the slot is
// cleared either way. The skinned-object list's destructor frees its items.
BSDecalPlacementVectorExtraData::~BSDecalPlacementVectorExtraData() {
    for (DECAL_PLACEMENT_VECTOR_GROUP_DATA*& rpGroup : pDecalPlacementGroups) {
        if (rpGroup) {
            MemoryManager_Singleton.Deallocate(rpGroup);
        }
        rpGroup = nullptr;
    }
}

NiObject* BSDecalPlacementVectorExtraData::CreateClone(NiCloningProcess& arCloneData) {
    BSDecalPlacementVectorExtraData* pobj;
    pobj = new BSDecalPlacementVectorExtraData(0);
    CopyMembers(pobj, arCloneData);
    return pobj;
}

void BSDecalPlacementVectorExtraData::LoadBinary(NiStream& arStream) {
    NiIntegerExtraData::LoadBinary(arStream);
    NiStreamLoadBinary(arStream, usGroupCount);
    // No check against the 8 group slots.
    for (int32_t i = 0; i < usGroupCount; i++) {
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(DECAL_PLACEMENT_VECTOR_GROUP_DATA));
        pDecalPlacementGroups[i] = pMemory ? ::new (pMemory) DECAL_PLACEMENT_VECTOR_GROUP_DATA() : nullptr;
        DECAL_PLACEMENT_VECTOR_GROUP_DATA* pGroup = pDecalPlacementGroups[i];
        NiStreamLoadBinary(arStream, pGroup->usVectorCount);
        // No check against the 8 vector slots.
        NiStreamLoadBinary(arStream, &pGroup->VectorOrigin[0].x, pGroup->usVectorCount * 3);
        NiStreamLoadBinary(arStream, &pGroup->VectorDirection[0].x, pGroup->usVectorCount * 3);
    }
}

void BSDecalPlacementVectorExtraData::LinkObject(NiStream& arStream) {
    NiIntegerExtraData::LinkObject(arStream);
}

bool BSDecalPlacementVectorExtraData::RegisterStreamables(NiStream& arStream) {
    // Returns the negated base result, deliberately, as the game does.
    return !NiIntegerExtraData::RegisterStreamables(arStream);
}

void BSDecalPlacementVectorExtraData::SaveBinary(NiStream& arStream) {
    NiIntegerExtraData::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, usGroupCount);
    for (int32_t i = 0; i < usGroupCount; i++) {
        DECAL_PLACEMENT_VECTOR_GROUP_DATA* pGroup = pDecalPlacementGroups[i];
        NiStreamSaveBinary(arStream, pGroup->usVectorCount);
        NiStreamSaveBinary(arStream, &pGroup->VectorOrigin[0].x, pGroup->usVectorCount * 3);
        NiStreamSaveBinary(arStream, &pGroup->VectorDirection[0].x, pGroup->usVectorCount * 3);
    }
}

bool BSDecalPlacementVectorExtraData::IsEqual(NiObject* apObject) {
    return NiIntegerExtraData::IsEqual(apObject);
}

void BSDecalPlacementVectorExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiIntegerExtraData::GetViewerStrings(pkStrings);
    // This block is titled with NiNode's RTTI name, not this class's, as the game does.
    pkStrings->Add(NiGetViewerString(NiNode::ms_RTTI.GetName()));
}

void BSDecalPlacementVectorExtraData::CopyMembers(
    BSDecalPlacementVectorExtraData* pDest,
    NiCloningProcess& arCloneData
) {
    NiIntegerExtraData::CopyMembers(pDest, arCloneData);
    pDest->usGroupCount = usGroupCount;
    for (int32_t i = 0; i < usGroupCount; ++i) {
        DECAL_PLACEMENT_VECTOR_GROUP_DATA* v7;
        DECAL_PLACEMENT_VECTOR_GROUP_DATA* v9 = static_cast<DECAL_PLACEMENT_VECTOR_GROUP_DATA*>(
            MemoryManager_Singleton.Allocate(sizeof(DECAL_PLACEMENT_VECTOR_GROUP_DATA))
        );
        if (v9) {
            v7 = new (v9) DECAL_PLACEMENT_VECTOR_GROUP_DATA();
        }
        else {
            v7 = nullptr;
        }
        pDest->pDecalPlacementGroups[i] = v7;
        pDest->pDecalPlacementGroups[i]->usVectorCount = pDecalPlacementGroups[i]->usVectorCount;
        for (int32_t j = 0; j < pDecalPlacementGroups[i]->usVectorCount; ++j) {
            NiPoint3* v3 = &pDecalPlacementGroups[i]->VectorOrigin[j];
            NiPoint3* v4 = &pDest->pDecalPlacementGroups[i]->VectorOrigin[j];
            v4->x = v3->x;
            v4->y = v3->y;
            v4->z = v3->z;
            NiPoint3* v5 = &pDecalPlacementGroups[i]->VectorDirection[j];
            NiPoint3* v6 = &pDest->pDecalPlacementGroups[i]->VectorDirection[j];
            v6->x = v5->x;
            v6->y = v5->y;
            v6->z = v5->z;
        }
    }
}

NiFixedString* BSDecalPlacementVectorExtraData::GetTag() {
    return pTag;
}

bool BSDecalPlacementVectorExtraData::FindDecalPlacementVectorGroupRecurse(
    NiAVObject* apObject,
    NiAVObject* apRootObject
) {
    size_t sLen;
    NiMatrix3 rot;
    if (!apObject->IsNode()) {
        return false;
    }
    if (!sDVPG) {
        NiFixedString* v9;
        NiFixedString* v12 = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
        if (v12) {
            if ("DVPG") {
                v12->m_kHandle = NiGlobalStringTable::AddString("DVPG");
            }
            else {
                v12->m_kHandle = NiGlobalStringTable::NULL_STRING;
            }
            v9 = v12;
        }
        else {
            v9 = nullptr;
        }
        sDVPG = v9;
    }
    BSDecalPlacementVectorExtraData* pextra =
        static_cast<BSDecalPlacementVectorExtraData*>(apRootObject->GetExtraData(*sDVPG));
    NiNode* v24 = static_cast<NiNode*>(apObject);
    if (!apObject->m_kName.GetHandle() || !BSstristr(v24->m_kName.GetHandle(), "DecalPlacementVectorGroup")) {
        for (uint32_t i = 0; i < v24->m_kChildren.m_usSize; ++i) {
            NiAVObject* child = v24->m_kChildren.m_pBase[i].m_pObject;
            if (child) {
                BSDecalPlacementVectorExtraData::FindDecalPlacementVectorGroupRecurse(child, apRootObject);
            }
        }
        return false;
    }
    if (!pextra) {
        BSDecalPlacementVectorExtraData* pextra_2;
        pextra_2 = new BSDecalPlacementVectorExtraData(0);
        pextra = pextra_2;
        apRootObject->AddExtraData(*sDVPG, pextra_2);
    }
    BSStringT<char> strName("DecalPlacementVectorGroup");
    const char* tmp = v24->m_kName.GetHandle();
    if (strName.sLen == 0xFFFF) {
        sLen = std::strlen(strName.pString);
    }
    else {
        sLen = strName.sLen;
    }
    tmp += sLen;
    int32_t igroupnum = std::atoi(tmp);
    if (pextra->usGroupCount < 8u && igroupnum < 8 && !pextra->pDecalPlacementGroups[igroupnum]) {
        DECAL_PLACEMENT_VECTOR_GROUP_DATA* pnewdata;
        DECAL_PLACEMENT_VECTOR_GROUP_DATA* v10 = static_cast<DECAL_PLACEMENT_VECTOR_GROUP_DATA*>(
            MemoryManager_Singleton.Allocate(sizeof(DECAL_PLACEMENT_VECTOR_GROUP_DATA))
        );
        if (v10) {
            pnewdata = new (v10) DECAL_PLACEMENT_VECTOR_GROUP_DATA();
        }
        else {
            pnewdata = nullptr;
        }
        for (int32_t j = 0; j < v24->m_kChildren.m_usESize; ++j) {
            if (j < 8) {
                NiPoint3 direction;
                NiAVObject* pplace = v24->m_kChildren.m_pBase[j].m_pObject;
                rot = pplace->m_kLocal.m_Rotate;
                direction.x = rot.m_pEntry[0][2];
                direction.y = rot.m_pEntry[1][2];
                direction.z = rot.m_pEntry[2][2];
                direction.Unitize();
                NiPoint3* p_m_Translate = &pplace->m_kLocal.m_Translate;
                NiPoint3* v3 = &pnewdata->VectorOrigin[pnewdata->usVectorCount];
                v3->x = pplace->m_kLocal.m_Translate.x;
                v3->y = p_m_Translate->y;
                v3->z = p_m_Translate->z;
                NiPoint3* v4 = &pnewdata->VectorDirection[pnewdata->usVectorCount];
                v4->x = direction.x;
                v4->y = direction.y;
                v4->z = direction.z;
                ++pnewdata->usVectorCount;
            }
        }
        pextra->pDecalPlacementGroups[igroupnum] = pnewdata;
        ++pextra->usGroupCount;
    }
    strName.Set(nullptr, 0);
    return true;
}

void BSDecalPlacementVectorExtraData::CreateDecalPlacementVectorExtraData(NiAVObject* apObject) {
    BSDecalPlacementVectorExtraData::FindDecalPlacementVectorGroupRecurse(static_cast<NiNode*>(apObject), apObject);
}

NiObject* BSDecalPlacementVectorExtraData::CreateObject() {
    BSDecalPlacementVectorExtraData* v1;
    v1 = new BSDecalPlacementVectorExtraData(0);
    v1->SetName(EmptyFS);
    return v1;
}
