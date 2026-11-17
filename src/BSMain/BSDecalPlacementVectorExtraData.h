#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"

class NiAVObject;

// One decal group of the extra data: the points a decal may be placed at and the directions it projects along.
struct DECAL_PLACEMENT_VECTOR_GROUP_DATA {
    // No vectors; every origin and direction zero.
    DECAL_PLACEMENT_VECTOR_GROUP_DATA();

    uint16_t usVectorCount;
    NiPoint3 VectorOrigin[8];
    NiPoint3 VectorDirection[8];
};

// The extra data a decal carries: the world-space points the decal projects onto.
class BSDecalPlacementVectorExtraData : public NiIntegerExtraData {
public:
    BSDecalPlacementVectorExtraData(const int32_t iValue);
    ~BSDecalPlacementVectorExtraData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    // NiIntegerExtraData's part, then copies of the group data, into apDest; not virtual.
    void CopyMembers(BSDecalPlacementVectorExtraData* pDest, NiCloningProcess& arCloneData);

    // The key the form-by-tag lookups find this extra data by.
    static NiFixedString* GetTag();
    // Whether a decal placement vector group is found on apSource's children for apTarget.
    static bool FindDecalPlacementVectorGroupRecurse(NiAVObject* apObject, NiAVObject* apRootObject);
    static void CreateDecalPlacementVectorExtraData(NiAVObject* apObject);
    static NiObject* CreateObject();

    uint16_t usGroupCount;
    DECAL_PLACEMENT_VECTOR_GROUP_DATA* pDecalPlacementGroups[8];
    NiTList<NiAVObject*> skinnedObjects;

    // The key string, allocated at startup and deleted through NiFixedString's deleting destructor at shutdown, as the
    // fixed strings beside it are.
    static inline NiFixedString* pTag;
    static NiRTTI ms_RTTI;
};
