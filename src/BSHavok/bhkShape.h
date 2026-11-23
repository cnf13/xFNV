#pragma once

#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSerialized.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class bhkShapeCollection;
class hkGeometry;
struct hkpMassProperties;
class hkpShape;
struct hkShapeCinfo;
class NiCloningProcess;
class NiColor;
class NiNode;

// The materials a shape's surface can have; BHK_PLATFORM and BHK_STAIRS are flag bits, not materials.
enum HavokMaterialType : int32_t {
    BHK_MATERIAL_STONE = 0x0,
    BHK_MATERIAL_CLOTH = 0x1,
    BHK_MATERIAL_DIRT = 0x2,
    BHK_MATERIAL_GLASS = 0x3,
    BHK_MATERIAL_GRASS = 0x4,
    BHK_MATERIAL_METAL = 0x5,
    BHK_MATERIAL_ORGANIC = 0x6,
    BHK_MATERIAL_SKIN = 0x7,
    BHK_MATERIAL_WATER = 0x8,
    BHK_MATERIAL_WOOD = 0x9,
    BHK_MATERIAL_HEAVYSTONE = 0xA,
    BHK_MATERIAL_HEAVYMETAL = 0xB,
    BHK_MATERIAL_HEAVYWOOD = 0xC,
    BHK_MATERIAL_CHAIN = 0xD,
    BHK_MATERIAL_SNOW = 0xE,
    BHK_MATERIAL_ELEVATOR = 0xF,
    BHK_MATERIAL_HOLLOWMETAL = 0x10,
    BHK_MATERIAL_SHEETMETAL = 0x11,
    BHK_MATERIAL_SAND = 0x12,
    BHK_MATERIAL_BROKENCONCRETE = 0x13,
    BHK_MATERIAL_VEHICLEBODY = 0x14,
    BHK_MATERIAL_VEHICLEPARTSOLID = 0x15,
    BHK_MATERIAL_VEHICLEPARTHOLLOW = 0x16,
    BHK_MATERIAL_BARREL = 0x17,
    BHK_MATERIAL_BOTTLE = 0x18,
    BHK_MATERIAL_SODACAN = 0x19,
    BHK_MATERIAL_PISTOL = 0x1A,
    BHK_MATERIAL_RIFLE = 0x1B,
    BHK_MATERIAL_SHOPPINGCART = 0x1C,
    BHK_MATERIAL_LUNCHBOX = 0x1D,
    BHK_MATERIAL_BABYRATTLE = 0x1E,
    BHK_MATERIAL_RUBBERBALL = 0x1F,
    BHK_MATERIAL_CHAINLINK = 0x20,
    BHK_MATERIAL_TILE = 0x21,
    BHK_MATERIAL_CARPET = 0x22,
    BHK_MATERIAL_TUMBLEWEED = 0x23,
    BHK_MATERIAL_MAX = 0x24,
    BHK_PLATFORM = 0x20,
    BHK_STAIRS = 0x40,
};

// The flag bits a shape's material carries besides its material type: a platform or stairs surface.
enum MATERIAL_VARIANT_TYPE : int32_t {
    MVT_PLATFORM = 0x20,
    MVT_STAIRS = 0x40,
};

// The Gamebryo side of a Havok shape. Only the members this code uses are declared, besides the virtual functions.
class bhkShape : public bhkSerializable {
public:
    bhkShape();
    ~bhkShape() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Link(bool abAttach) override;

    // Copies the shape's data to apDestination.
    virtual void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData);
    // Adds the shape's mass properties to arProp; returns HK_SUCCESS when it did and HK_FAILURE when there is no Havok
    // shape.
    virtual hkResult CalcMass(hkpMassProperties& arProp);
    // The collection the shape is built over; none for a plain shape.
    virtual bhkShapeCollection* FindShapeCollection() {
        return nullptr;
    }
    // True when the clone can share this shape instead of copying it.
    virtual bool CanShare(NiCloningProcess& arCloneData);
    virtual void BuildDisplayGeometry(NiNode*, const NiColor&) {}
    virtual void DestroyDisplayGeometry() {}
    virtual void
    BuildDisplayFromhkGeometry(NiNode* apNode, hkGeometry* apGeom, const NiColor& arDefColor, const char* apName);

    // Copies the shape's user data into arInfo; it hides the base's CopyCinfo rather than overriding it, so the virtual
    // remains bhkSerializable's.
    void CopyCinfo(hkShapeCinfo& arInfo);
    // The wrapped Havok shape.
    hkpShape* Gethk();
    // Returns the wrapped Havok shape, or null for a null this.
    hkpShape* operator*() const;
    // The material of the sub-shape aiKey.
    HavokMaterialType GetSubshapeMaterial(uint32_t aiKey);
    HavokMaterialType GetMaterial();
    // Whether the shape's material has any of aeType's bits.
    bool GetMaterialVarType(MATERIAL_VARIANT_TYPE aeType);
    // Walks the collection chain to the shape that holds the sub-material, then compares whether that shape's material
    // carries any bit of aeType with aeType itself.
    bool GetSubMaterialVarType(uint32_t aiKey, MATERIAL_VARIANT_TYPE aeType);

    static bhkShape* Getbhk(const hkpShape* apShape);
    // The debug display color of material aeType.
    static void GetMaterialColor(HavokMaterialType aeType, NiColor& arColor);

    HavokMaterialType eMaterial;

    static NiRTTI ms_RTTI;
    static inline bool bMaterialColor;
    static inline uint32_t iCount;
};

// The creation info every shape's starts with: the user data its Havok shape carries. It has no vtable. Its constructor
// stores the zero.
struct hkShapeCinfo : public hkSerializableCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 4 bytes.
    struct Serialized {
        int32_t m_userData;
    };

    hkShapeCinfo();

    // Fills this part from its serialized form.
    void ReadSerialized(const Serialized& arForm) {
        m_userData = arForm.m_userData;
    }

    int32_t m_userData;
};

static_assert(sizeof(hkShapeCinfo::Serialized) == 0x4);
