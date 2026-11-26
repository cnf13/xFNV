#pragma once

#include "BSHavok/bhkShapePhantom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"

#include <stdint.h>

class hkpFixedBufferCdPointCollector;

// The creation info of a simple shape phantom; adds nothing to the shape phantom's.
struct hkSimpleShapePhantomCinfo : public hkShapePhantomCinfo {
    hkSimpleShapePhantomCinfo();
    ~hkSimpleShapePhantomCinfo();
};

// A shape phantom that keeps no list of what overlaps it. Only part of the class is declared.
class bhkSimpleShapePhantom : public bhkShapePhantom {
public:
    bhkSimpleShapePhantom();
    bhkSimpleShapePhantom(const hkSimpleShapePhantomCinfo& arInfo);
    ~bhkSimpleShapePhantom() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void KillCinfo(bool abCreated) override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    // The size of the creation info in a file: its serialized form's.
    uint32_t QCinfoSize() const override {
        return sizeof(hkSimpleShapePhantomCinfo::Serialized);
    }
    // Reads the serialized form of the cinfo rather than reading it raw through bhkSerializable::LoadCInfo.
    int32_t LoadCInfo(NiStream& arStream) override;

    void GetFixedClosestPoints(hkpFixedBufferCdPointCollector& arCollector);
    hkArray<hkpSimpleShapePhantom::CollisionDetail, hkContainerHeapAllocator>& GetCollisionDetails();

    // The object a stream creates for this class: a phantom from the default constructor.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};
NiSmartPointer(bhkSimpleShapePhantom);

inline hkSimpleShapePhantomCinfo::hkSimpleShapePhantomCinfo() {}

inline bhkSimpleShapePhantom::bhkSimpleShapePhantom(const hkSimpleShapePhantomCinfo& arInfo) {
    bhkSimpleShapePhantom::Init(arInfo);
    ++iCount;
    ResetPhantomFlags();
}

inline hkArray<hkpSimpleShapePhantom::CollisionDetail, hkContainerHeapAllocator>&
bhkSimpleShapePhantom::GetCollisionDetails() {
    return static_cast<hkpSimpleShapePhantom*>(**this)->getCollisionDetails();
}
