#pragma once

#include "BSHavok/bhkBvTreeShape.h"

class bhkShapeCollection;
struct hkMoppBvTreeShapeCinfo;
class hkpMoppCode;

// A bounding volume tree shape searched through a MOPP code (Havok's compiled tree over the shape it was built over).
class bhkMoppBvTreeShape : public bhkBvTreeShape {
public:
    bhkMoppBvTreeShape();
    bhkMoppBvTreeShape(bhkShape* apShape);
    ~bhkMoppBvTreeShape() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    uint32_t QCinfoSize() const override;
    int32_t LoadCInfo(NiStream& arStream) override;
    // Builds the MOPP shape from the creation info.
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void PostInit(const hkSerializableCinfo& arInfo) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

    // Builds the MOPP code over apShapeCollection and passes it to the virtual Init in a creation info; the
    // constructor calls it.
    void Init(bhkShapeCollection* apShapeCollection);
    // Calls bhkBvTreeShape::CopyCinfo, then copies the unscaled MOPP code and the scale of the wrapped
    // hkScaledMoppBvTreeShape into arInfo; CreateCinfo calls it.
    void CopyCinfo(hkMoppBvTreeShapeCinfo& arInfo);

    // Creates an uninitialised instance for the stream's factory.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Number of live instances; the constructors count them.
    static inline uint32_t iCount;
};

// The creation info of a MOPP shape: the MOPP code and its scale.
struct hkMoppBvTreeShapeCinfo : public hkBvTreeShapeCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 16 bytes, the size QCinfoSize gives.
    struct Serialized : hkBvTreeShapeCinfo::Serialized {
        bhkSerializedPointer m_code;
        float fScale;
    };

    hkMoppBvTreeShapeCinfo();

    // Fills the info from its serialized form; the code is left null (LoadBinary reads it after the info).
    void ReadSerialized(const Serialized& arForm) {
        hkBvTreeShapeCinfo::ReadSerialized(arForm);
        m_code = nullptr;
        fScale = arForm.fScale;
    }

    const hkpMoppCode* m_code;
    float fScale;
};

static_assert(offsetof(hkMoppBvTreeShapeCinfo::Serialized, fScale) == 0xC);
static_assert(sizeof(hkMoppBvTreeShapeCinfo::Serialized) == 0x10);

inline hkMoppBvTreeShapeCinfo::hkMoppBvTreeShapeCinfo() : m_code(nullptr), fScale(1.0f) {}
