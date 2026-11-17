#pragma once

#include "BSHavok/bhkListShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class NiStream;
class hkPackedNiTriStripsShapeCinfo;

// A sub-part's material: the mesh material's filter info and the material's CRC. LoadBinary allocates 0x18 bytes for
// it and aligns the entry to 16 bytes, the padding length in the byte before it; the stream
// fills its two dwords.
class hkpExtendedMaterial : public hkpMeshMaterial {
public:
	uint32_t uiMaterialCRC;
};

// One streamed sub-part of an extended mesh shape: a vertex block and a triangle block, and the material block the
// exe keeps 16-aligned. The PDB names the fields; the asm gives the sizes: 12 bytes per vertex, 6 per triangle
// (three 16-bit indices), and 0x14 for the sub-part itself.
class bhkExtendedMeshSubPart {
public:
	bhkExtendedMeshSubPart();
	// Frees the vertex, index and material blocks under the Havok memory context.
	~bhkExtendedMeshSubPart();

	// Writes the sub-part to apStream.
	void SaveBinary(NiStream& arStream);
	// Reads the sub-part from arStream.
	void LoadBinary(NiStream& arStream);

	uint32_t uiNumVerts;
	// uiNumVerts vertices of three floats each.
	float* pVertexBaseA;
	uint32_t uiNumTris;
	// uiNumTris triangles of three 16-bit indices each.
	uint16_t* pIndexBaseA;
	// Goes back through the aligned free.
	hkpExtendedMaterial* pMat;
};

// The streamed sub-part list of an extended mesh shape: the number of sub parts and their array.
class bhkExtendedMeshShapeData : public NiObject {
public:
	bhkExtendedMeshShapeData();
	~bhkExtendedMeshShapeData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;

	// The stream factory's creator: a new, empty sub-part list.
	static NiObject* CreateObject();

	uint32_t uiNumSubParts;
	bhkExtendedMeshSubPart* pSubDataA;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(bhkExtendedMeshShapeData);

// The creation info of an extended mesh shape, built by bhkExtendedMeshShape::CreateCinfo and streamed by LoadCInfo.
// The PDB's own size is 0x20 too: vScale is a 16-byte-aligned hkVector4, so the four bytes after fRadius are the
// alignment's, and nothing follows vScale. CreateCinfo's 0x30-byte allocation (0xCCC009) is the 16-byte-aligned one,
// its 16 bytes of padding included; the constructor writes only the user data, spData, fRadius and vScale.
struct hkExtendedMeshShapeCinfo : hkShapeCollectionCinfo {
	// The local the exe constructs in place (FNVD_INLINED_LOCAL): the destructor releases spData, so it starts
	// empty.
	hkExtendedMeshShapeCinfo() : spData(nullptr) {}
	// The PC build's constructor from the sub-part data, not in the PDB (whose only constructor takes nothing):
	// thiscall, ret 4, 0xCCBAA0. It lies among bhkExtendedMeshShape's functions, and bhkExtendedMeshShape::Init
	// (0xCCC980) builds the info it passes to Init(const hkSerializableCinfo&) with it.
	hkExtendedMeshShapeCinfo(bhkExtendedMeshShapeData* apData);
	~hkExtendedMeshShapeCinfo();

	bhkExtendedMeshShapeData* spData;
	float fRadius;
	hkVector4 vScale;
};

// The Gamebryo side of Havok's extended mesh shape (triangle strips with sub parts): the shape collection plus the
// streamed sub-part data.
class bhkExtendedMeshShape : public bhkShapeCollection {
public:
	bhkExtendedMeshShape();
	~bhkExtendedMeshShape() override;

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
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) override {}
	HavokMaterialType QSubMaterial(uint32_t aiKey) override;
	virtual void Init(const hkPackedNiTriStripsShapeCinfo& arInfo);

	// Fills apInfo from the wrapped Havok object; the exe's CreateCinfo calls it once the shape
	// exists (thiscall, ret 4, 0xCCBCE0).
	void CCBCE0(hkExtendedMeshShapeCinfo* apInfo);

	// The stream factory's creator: a new, empty shape.
	static NiObject* CreateObject();

	NiPointer<bhkExtendedMeshShapeData> spData;

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable bhkExtendedMeshShapeData_vftable{};
inline constexpr fnvd::Vtable bhkExtendedMeshShape_vftable{};
