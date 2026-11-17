#pragma once

#include "BSHavok/bhkListShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/hkNiTriStripsShape.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// The Havok shape collection over a NiTriStrips' triangle strips.
class bhkNiTriStripsShape : public bhkShapeCollection {
public:
	bhkNiTriStripsShape();
	~bhkNiTriStripsShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
	bool CanShare(NiCloningProcess& arCloneData) override;
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;
	HavokMaterialType QSubMaterial(uint32_t aiKey) override;
	virtual bool GetSubMaterialVarType(uint32_t aiKey, MATERIAL_VARIANT_TYPE aeType);

	// The shape's scale (its x), or 1 without a shape.
	float GetScale() const;

	// The engine's object factory for the class.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
	// The sub part returned for an index when the wrapper holds no shape.
	static hkNiTriStripsData EmptySubpart;
};

// The creation info of an hkNiTriStripsShape, 0x30 bytes: the sub parts' array at 0x8 and the scale the streaming
// code byte-swaps at 0x20. CreateCinfo zeroes m_radius.
class hkNiTriStripsShapeCinfo : public hkShapeCollectionCinfo {
public:
	// The serialized form (bhkSerialized.h): the 32-bit layout, 0x30 bytes. The sub parts' array is the 32-bit
	// NiTLargePrimitiveArray's image (its vtable and buffer addresses, then its counts); the files hold it empty
	// (SaveBinary empties it first), the sub parts following in the stream on their own.
	struct Serialized : hkShapeCinfo::Serialized {
		struct LargeArray {
			bhkSerializedPointer m_vtable;
			bhkSerializedPointer m_pBase;
			uint32_t m_uiMaxSize;
			uint32_t m_uiSize;
			uint32_t m_uiESize;
			uint32_t m_uiGrowBy;
		};

		float m_radius;
		LargeArray Shapes;
		bhkVector4 Scale;
	};

	// Fills the info from its serialized form. The sub parts' array is not taken from the file: the native one
	// CreateCinfo built (its vtable, no storage) is kept, and LoadBinary adds the sub parts.
	void ReadSerialized(const Serialized& arForm) {
		hkShapeCinfo::ReadSerialized(arForm);
		m_radius = arForm.m_radius;
		Scale = arForm.Scale;
	}

	float m_radius;
	NiTLargePrimitiveArray<hkNiTriStripsData> pShapesA;
	// hkNiTriStripsShape::CopyCinfo fills it with bhkVector4::operator=.
	bhkVector4 Scale;
};

static_assert(sizeof(hkNiTriStripsShapeCinfo::Serialized::LargeArray) == 0x18);
static_assert(offsetof(hkNiTriStripsShapeCinfo::Serialized, m_radius) == 0x4);
static_assert(offsetof(hkNiTriStripsShapeCinfo::Serialized, Shapes) == 0x8);
static_assert(offsetof(hkNiTriStripsShapeCinfo::Serialized, Scale) == 0x20);
static_assert(sizeof(hkNiTriStripsShapeCinfo::Serialized) == 0x30);

// The creation info of an old hkpMeshShape, which bhkNiTriStripsShape::LoadBinary reads from a file holding an old
// bhkpMeshShape and converts. The constructor and destructor touch the twelve arrays (element size 4 bytes, and 1 for
// m_stridingTypeArray).
struct hkpMeshShapeCinfo : public hkShapeCollectionCinfo {
	// The serialized form (bhkSerialized.h): the 32-bit layout, 0xC0 bytes.
	struct Serialized : hkShapeCinfo::Serialized {
		float m_radius;
		uint8_t padding08[8];
		hkVector4 m_scaling;
		int32_t m_numBitsForSubpartIndex;
		bhkSerializedArray m_vertexBaseArray;
		bhkSerializedArray m_vertexStridingArray;
		bhkSerializedArray m_numVerticesArray;
		bhkSerializedArray m_index32BaseArray;
		bhkSerializedArray m_stridingTypeArray;
		bhkSerializedArray m_indexStridingArray;
		bhkSerializedArray m_numTrianglesArray;
		bhkSerializedArray m_materialIndexBaseArray;
		bhkSerializedArray m_materialIndexStridingArray;
		bhkSerializedArray m_materialBaseArray;
		bhkSerializedArray m_materialStridingArray;
		bhkSerializedArray m_numMaterialsArray;
		uint8_t paddingB4[12];
	};

	hkpMeshShapeCinfo();
	~hkpMeshShapeCinfo();

	// Fills the info from its serialized form.
	void ReadSerialized(const Serialized& arForm) {
		hkShapeCinfo::ReadSerialized(arForm);
		m_radius = arForm.m_radius;
		m_scaling = arForm.m_scaling;
		m_numBitsForSubpartIndex = arForm.m_numBitsForSubpartIndex;
		arForm.m_vertexBaseArray.ToNative(m_vertexBaseArray);
		arForm.m_vertexStridingArray.ToNative(m_vertexStridingArray);
		arForm.m_numVerticesArray.ToNative(m_numVerticesArray);
		arForm.m_index32BaseArray.ToNative(m_index32BaseArray);
		arForm.m_stridingTypeArray.ToNative(m_stridingTypeArray);
		arForm.m_indexStridingArray.ToNative(m_indexStridingArray);
		arForm.m_numTrianglesArray.ToNative(m_numTrianglesArray);
		arForm.m_materialIndexBaseArray.ToNative(m_materialIndexBaseArray);
		arForm.m_materialIndexStridingArray.ToNative(m_materialIndexStridingArray);
		arForm.m_materialBaseArray.ToNative(m_materialBaseArray);
		arForm.m_materialStridingArray.ToNative(m_materialStridingArray);
		arForm.m_numMaterialsArray.ToNative(m_numMaterialsArray);
	}

	float m_radius;
	hkVector4 m_scaling;
	int32_t m_numBitsForSubpartIndex;
	hkArray<const float*> m_vertexBaseArray;
	hkArray<int32_t> m_vertexStridingArray;
	hkArray<int32_t> m_numVerticesArray;
	hkArray<const uint32_t*> m_index32BaseArray;
	hkArray<uint8_t> m_stridingTypeArray;
	hkArray<int32_t> m_indexStridingArray;
	hkArray<int32_t> m_numTrianglesArray;
	hkArray<const uint8_t*> m_materialIndexBaseArray;
	hkArray<int32_t> m_materialIndexStridingArray;
	hkArray<const void*> m_materialBaseArray;
	hkArray<int32_t> m_materialStridingArray;
	hkArray<int32_t> m_numMaterialsArray;
};

static_assert(offsetof(hkpMeshShapeCinfo::Serialized, m_scaling) == 0x10);
static_assert(offsetof(hkpMeshShapeCinfo::Serialized, m_numBitsForSubpartIndex) == 0x20);
static_assert(offsetof(hkpMeshShapeCinfo::Serialized, m_vertexBaseArray) == 0x24);
static_assert(offsetof(hkpMeshShapeCinfo::Serialized, m_stridingTypeArray) == 0x54);
static_assert(offsetof(hkpMeshShapeCinfo::Serialized, m_numMaterialsArray) == 0xA8);
static_assert(sizeof(hkpMeshShapeCinfo::Serialized) == 0xC0);
