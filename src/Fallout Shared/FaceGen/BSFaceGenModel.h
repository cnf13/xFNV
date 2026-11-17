#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/iosfwd.h"

#include <__stddef_ptrdiff_t.h>
#include <__stddef_size_t.h>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class BSFaceGenBaseMorphExtraData;
class BSFaceGenModelMeshData;
class BSFaceGenModelTextureData;
class BSFaceGenMorph;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
struct EGMData;
// FaceGen's element classes the vector instantiations below hold; not modelled yet (sizes from the exe's strides).
template <class T> class FR2Vect2TC;
template <class T> class FR2Vect3TC;
struct FffModeGS;
struct FffModeTexS;
class FffTriFileDataS;
enum HAIR_PART : int32_t;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiSkinInstance;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;

// The .tri morph data of a FaceGen mesh (head or hair). Not an NiObject, but it repeats NiObject's run-time type
// virtuals, as NiShader does.
class BSFaceGenMorphData : public NiRefObject {
public:
	BSFaceGenMorphData();
	~BSFaceGenMorphData() override;

	virtual const NiRTTI* GetRTTI() const { return &ms_RTTI; }
	// type queries: the object as the named class, or null
	virtual NiNode* IsNode() { return nullptr; }
	virtual BSFadeNode* IsFadeNode() { return nullptr; }
	virtual BSMultiBoundNode* IsMultiBoundNode() { return nullptr; }
	virtual NiGeometry* IsGeometry() { return nullptr; }
	virtual NiTriBasedGeom* IsTriBasedGeom() { return nullptr; }
	virtual NiTriStrips* IsTriStrips() { return nullptr; }
	virtual NiTriShape* IsTriShape() { return nullptr; }
	virtual BSSegmentedTriShape* IsSegmentedTriShape() { return nullptr; }
	virtual BSResizableTriShape* IsResizableTriShape() { return nullptr; }
	virtual NiParticles* IsParticlesGeom() { return nullptr; }
	virtual NiLines* IsLinesGeom() { return nullptr; }
	virtual bhkNiCollisionObject* IsBhkNiCollisionObject() { return nullptr; }
	virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject() { return nullptr; }
	virtual bhkRigidBody* IsBhkRigidBody() const { return nullptr; }
	virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() { return nullptr; }

	// NiObject's member DynamicCast repeated: this when
	// IsKindOfMember holds, else null.
	BSFaceGenMorphData* DynamicCastMember(const NiRTTI* apRTTI) const;
	// Not named in the PDB or IDA; name chosen here. NiObject's member IsKindOf repeated: whether the run-time
	// type is apRTTI's class or derives from it (thiscall, ret 4; returns al).
	bool IsKindOfMember(const NiRTTI* apRTTI) const;

	// NiDynamicCast's static: apObject when its run-time type is apRTTI's class or derives
	// from it, else null.
	static BSFaceGenMorphData* DynamicCast(const NiRTTI* pkRTTI, const BSFaceGenMorphData* pkObject);

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSFaceGenMorphData);

// The morph data of a FaceGen head: per morph type, the morphs by index.
class BSFaceGenMorphDataHead : public BSFaceGenMorphData {
public:
	// The constructor from the .tri file's data apFaceGenTri;
	// returns apMemory.
	BSFaceGenMorphDataHead(FffTriFileDataS* apFaceGenTri);

	const NiRTTI* GetRTTI() const override;
	~BSFaceGenMorphDataHead() override;

	// Adds morph aiIndex of type aeType, scaled by afModifier, to apGeom's vertices.
	void ApplyMorph(BS_FACEGEN_MORPH_TYPE aeType, int32_t aiIndex, NiTriBasedGeom* apGeom,
		BSFaceGenBaseMorphExtraData* apBaseMorph, float afModifier);
	BSFaceGenMorph* GetExpressionMorph(BS_FACEGEN_EXPRESSION aeIndex);
	BSFaceGenMorph* GetModifierMorph(BS_FACEGEN_MODIFIER aeIndex);
	BSFaceGenMorph* GetPhonemeMorph(BS_FACEGEN_PHONEME aeIndex);
	BSFaceGenMorph* GetCustomMorph(BS_FACEGEN_CUSTOM_MORPH aeIndex);

	// The morphs of each type, by index: the constructor stores differential and statistical morphs.
	BSFaceGenMorph** ppExpressionMorphs;
	BSFaceGenMorph** ppModifierMorphs;
	BSFaceGenMorph** ppPhonemeMorphs;
	BSFaceGenMorph** ppCustomMorphs;

	static NiRTTI ms_RTTI;
};

// A FaceGen head model: its mesh data and its texture data.
class BSFaceGenModel : public NiRefObject {
public:
	BSFaceGenModel();
	~BSFaceGenModel() override;

	// The .egm file name of the mesh data; null without mesh data.
	const char* GetEGMFilename();
	// The .egt file name of the texture data; null without texture data.
	const char* GetEGTFilename();
	// The morph data of the mesh; null without mesh data or morph.
	BSFaceGenMorphData* GetMorph();
	// Loads the mesh (.egm, .nif, .tri) and the texture (.egt) data when FaceGen heads are on; true when either
	// loaded.
	bool LoadModel(const char* apEGMFilename, const char* apNIFFilename, const char* apTRIFilename,
		const char* apEGTFilename, HAIR_PART aeHairPart, bool abIsAccessory);
	// Loads the mesh data once: the .egm name, the model's geometry and the .tri morph data.
	bool LoadModelMesh(const char* apEGMFilename, const char* apNIFFilename, const char* apTRIFilename,
		HAIR_PART aeHairPart, bool abIsAccessory);
	// Loads the texture data once: allocates it and copies the .egt file name; true unless it was already loaded or
	// apEGTFilename is null, which also return true.
	bool LoadModelTexture(const char* apEGTFilename);
	// Clones the model's mesh into aspReturnMesh.
	bool CreateNewMesh(NiPointer<NiTriBasedGeom>& aspReturnMesh);
	// Builds a new mesh and applies apCoord to it; on failure the new mesh is released.
	bool ApplyCoordinateToNewMesh(const FR2MatrixVTC<float> (*apCoord)[2][2], NiPointer<NiTriBasedGeom>& aspReturnMesh);
	// Morphs apMesh by apCoord, the face's coordinate when apCoord is null.
	bool ApplyCoordinateToExistingMesh(const FR2MatrixVTC<float> (*apCoord)[2][2], NiTriBasedGeom* apMesh,
		bool abForceBaseMorph);
	// Loads the texture data's .egt data once; false without texture data or with an empty file name.
	bool LoadEGTData();
	// Deletes the texture data's .egt data.
	void UnloadEGTData();
	// The size of the mesh (.egm) morph data.
	uint32_t GetEGMDataSize();
	// The size of the texture (.egt) morph data.
	uint32_t GetEGTDataSize();
	// The model cache's test of a model before unloading its texture data.
	bool UnloadEGMData();
	// The word at +4, which for a model is its reference count.
	uint32_t GetRefCount() const { return m_uiRefCount; }
	bool HasEGTData() const;
	NiSkinInstance* GetSkinInstance() const;
	void ForceLoadModelTexture();
	bool ApplyCoordinateTexturingToMesh(const FR2MatrixVTC<float> (*apCoord)[2][2],
		NiPointer<NiTexture>& aspReturnBaseMod, uint32_t aiForceBasisSize);
	bool LoadEGMData();
	void KillTextureData();

	BSFaceGenModelMeshData* pMeshData;
	BSFaceGenModelTextureData* pTextureData;
};

NiSmartPointer(BSFaceGenModel);

// The .egt texture morph data of a FaceGen model.
class EGTData {
public:
	EGTData(char* a1);
	~EGTData();

	// Loads the .egt file apEGTFilename.
	static EGTData* Create(const char* apEGTFilename);
	static void operator delete(void* apMemory);

	uint32_t lEGTBasisKey;
	// The PDB's EGTBasis[2]: the constructor (0x659670) builds the two vectors at 4 (0x18 bytes each) and Create
	// allocates 0x34 bytes.
	std::vector<FffModeTexS, std::allocator<FffModeTexS>> EGTBasis[2];
};

// The texture half of a FaceGen model: the .egt file name and its data.
class BSFaceGenModelTextureData {
public:
	BSFaceGenModelTextureData();
	~BSFaceGenModelTextureData();

	static void operator delete(void* apMemory);

	BSStringT<char> EGTFilename;
	EGTData* pEGTData;
};

// The mesh half of a FaceGen model: the .egm morph data and the mesh it applies to. Reference counted on its own
// (not a NiRefObject).
class BSFaceGenModelMeshData {
public:
	BSFaceGenModelMeshData();
	~BSFaceGenModelMeshData();

	void IncRefCount();
	void DecRefCount();

	// delete frees with ourFree.
	static void operator delete(void* apMemory);

	BSStringT<char> EGMFilename;
	EGMData* pEGMData;
	NiPointer<NiNode> spModelRoot;
	NiPointer<NiTriBasedGeom> spModel;
	NiPoint3* pAdditionalVertices;
	uint32_t iAdditionalVerticesCount;
	NiPointer<BSFaceGenMorphData> spMorph;
	int32_t iRefCount;
};

// The engine's NiPointer of mesh data takes and releases references through the data's out-of-line IncRefCount
// and DecRefCount.
inline void NiIncRefCount(BSFaceGenModelMeshData* apData) {
	apData->IncRefCount();
}

inline void NiDecRefCount(BSFaceGenModelMeshData* apData) {
	apData->DecRefCount();
}

// The 6-byte FR2Vect3TC<short>: the diff morphs' vertex deltas and FffModeGS's vertex offsets. Its PDB source is
// BSFaceGenManager.cpp; the copies this file's vector rows make store a dword and then a word.
template <> class FR2Vect3TC<int16_t> {
public:
	int16_t x1;
	int16_t x2;
	int16_t x3;
};

// The 12-byte FR2Vect3TC<float>: the .tri reader's vertices. The PDB places it in BSFaceGenManager.cpp (the int
// instance has the same shape); this file's vector rows copy one element as three dwords.
template <> class FR2Vect3TC<float> {
public:
	// The PDB's operator+(v1): the element-wise sum with v1, built in apResult (the by-value
	// result; thiscall, ret 8, 0xAC10C0). src names it Add because it takes the hidden result pointer.
	// FffTriFileDataS::extractSurfLblPts (0xAB5720) calls it.
	FR2Vect3TC* Add(FR2Vect3TC* apResult, const FR2Vect3TC& v1) const;
	// The PDB's operator*(val): each element times val, built in apResult (the by-value
	// result; thiscall, ret 8, 0xAC1120). src names it MulScalar because it takes the hidden result pointer.
	// FffTriFileDataS::extractSurfLblPts (0xAB5720) calls it.
	FR2Vect3TC* MulScalar(FR2Vect3TC* apResult, float val) const;

	float x1;
	float x2;
	float x3;
};

// The 8-byte FR2Vect2TC<float>: a .tri texture coordinate. The PDB places it in FanControls.cpp; this file's vector
// rows copy one element as two floats.
template <> class FR2Vect2TC<float> {
public:
	float x1;
	float x2;
};

// The 0x1C-byte mode of a FaceGen mesh: its scale and its vertex offsets.
struct FffModeGS {
	FffModeGS() = default;
	FffModeGS(const FffModeGS& _Right);
	~FffModeGS();

	// Not named in the PDB or IDA; name chosen here. The PDB's operator=(const FffModeGS&): the scale, then the
	// vertex offsets' vector operator= 0x65EED0 (thiscall, ret 4, returns this, 0x65EB60). Not declared as operator=:
	// a user-declared copy assignment would make FffModeGS non-trivially copyable (vector's Assign has the same
	// reason).
	FffModeGS& Assign(const FffModeGS& _Right);

	float scale;
	std::vector<FR2Vect3TC<int16_t>, std::allocator<FR2Vect3TC<int16_t>>> vertPos;
};

// The 0x58-byte texture mode of a FaceGen mesh: its scale, width, height and row padding, then its three byte
// vectors of image data. The PDB's width, height and rowPad are unsigned long; fffReadEgtFile (0xAB2640) stores the
// .egt header's column and row counts there as dwords.
struct FffModeTexS {
	FffModeTexS();
	FffModeTexS(const FffModeTexS& _Right);
	~FffModeTexS();

	// Not named in the PDB or IDA; name chosen here. The PDB's operator=(const FffModeTexS&): the scale and three
	// dimension dwords, then each image vector's operator= 0x65F0C0 (thiscall, ret 4, returns this, 0x65EB90). Not
	// declared as operator=, as FffModeGS::Assign.
	FffModeTexS& Assign(const FffModeTexS& _Right);

	static void operator delete(void* apMemory);

	float scale;
	uint32_t width;
	uint32_t height;
	uint32_t rowPad;
	// Unsigned bytes: the exe's vector<char> copy constructor 0xAC5F00 is another instance, and the KEEP
	// _Insert_n 0xAB3370 on these vectors is the unsigned char one.
	std::vector<uint8_t, std::allocator<uint8_t>> img[3];
};

// FffModeGS::vertPos: the mode's vertex offsets.
using FR2Vect3SVector = std::vector<FR2Vect3TC<int16_t>, std::allocator<FR2Vect3TC<int16_t>>>;
// EGTData::EGTBasis: the texture modes of each of the two bases.
using FffModeTexSVector = std::vector<FffModeTexS, std::allocator<FffModeTexS>>;
// BSFaceGenModel's morph vectors; the PDB names them per element type only. Each one's ~vector is _Tidy, then the
// base destructor.
using FR2Vect3FloatVector = std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using FutVect4Vector = std::vector<FutVect4TC<int32_t>, std::allocator<FutVect4TC<int32_t>>>;
using FffTriStatMorphVector = std::vector<FffTriStatMorphS, std::allocator<FffTriStatMorphS>>;

// FffModeTexS::img: the three byte vectors of a texture mode's image data.
using FffModeTexImgVector = std::vector<uint8_t, std::allocator<uint8_t>>;

// Returns its first argument. Several identity functions fold into it, so callers push one to three arguments.
int32_t return_value(int32_t a1, ...);
// Inferred type: return: uint32_t: unsigned uses from 0x6608f9; a: uint16_t: unsigned uses from 0x660944; b: uint32_t:
// unsigned uses from 0x66094b
uint32_t BSMIN(uint16_t a, uint32_t b);
// The modes' backward move dispatcher:
// passes _First, _Last, _Dest and the iterator-category tag _Cat (the fourth argument's byte) on to the
// backward copy CopyBackwardOpt (FffModeGS steps of 0x1C) with two more tag bytes, and returns its result.
// vector<FffModeGS>::_Insert_n calls it with six arguments; a5 and a6 are not read.
FffModeGS* MoveBackwardOpt(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, uint8_t _Cat, uint8_t a5, uint8_t a6);
// _Destroy_range(_First, _Last, _Al): passes the elements on with their category tag (cdecl, ret 0). One out-of-line
// instance per element class; IDA names, not in the PDB.
void DestroyRange(FR2Vect3TC<float>* _First, FR2Vect3TC<float>* _Last, std::allocator<FR2Vect3TC<float>>& _Al);
void DestroyRange(FR2Vect2TC<float>* _First, FR2Vect2TC<float>* _Last, std::allocator<FR2Vect2TC<float>>& _Al);
void DestroyRange(FffTriVtxLabelS* _First, FffTriVtxLabelS* _Last, std::allocator<FffTriVtxLabelS>& _Al);
void DestroyRange(FffTriDiffMorphS* _First, FffTriDiffMorphS* _Last, std::allocator<FffTriDiffMorphS>& _Al);
void DestroyRange(FffTriStatMorphS* _First, FffTriStatMorphS* _Last, std::allocator<FffTriStatMorphS>& _Al);
void DestroyRange(FffModeGS* _First, FffModeGS* _Last, std::allocator<FffModeGS>& _Al);
void DestroyRange(FffTriSurfLabelS* _First, FffTriSurfLabelS* _Last, std::allocator<FffTriSurfLabelS>& _Al);
// _Destroy_range(_First, _Last, _Al, _Nonscalar_ptr_iterator_tag): destroys the elements one by
// one through _Al.destroy (cdecl, ret 0); IDA names, not in the PDB. _Cat is the tag's byte (see _Move_cat).
void DestroyRange(FR2Vect3TC<float>* _First, FR2Vect3TC<float>* _Last, std::allocator<FR2Vect3TC<float>>& _Al,
	uint8_t _Cat);
void DestroyRange(FR2Vect2TC<float>* _First, FR2Vect2TC<float>* _Last, std::allocator<FR2Vect2TC<float>>& _Al,
	uint8_t _Cat);
void DestroyRange(FffTriVtxLabelS* _First, FffTriVtxLabelS* _Last, std::allocator<FffTriVtxLabelS>& _Al, uint8_t _Cat);
void DestroyRange(FffTriDiffMorphS* _First, FffTriDiffMorphS* _Last, std::allocator<FffTriDiffMorphS>& _Al,
	uint8_t _Cat);
void DestroyRange(FffTriStatMorphS* _First, FffTriStatMorphS* _Last, std::allocator<FffTriStatMorphS>& _Al,
	uint8_t _Cat);
void DestroyRange(FffModeGS* _First, FffModeGS* _Last, std::allocator<FffModeGS>& _Al, uint8_t _Cat);
void DestroyRange(FffTriSurfLabelS* _First, FffTriSurfLabelS* _Last, std::allocator<FffTriSurfLabelS>& _Al,
	uint8_t _Cat);
// _Construct(_Ptr, _Val) of FffModeGS and FffModeTexS (cdecl, ret 0): placement new, then the element's copy
// constructor. IDA names; not in the PDB.
FffModeGS* Construct(FffModeGS* _Ptr, const FffModeGS& _Val);
FffModeTexS* Construct(FffModeTexS* _Ptr, const FffModeTexS& _Val);
// _Destroy_range(_First, _Last, _Al) of FR2Vect3TC<short> (cdecl, ret 0). IDA name; not in the PDB.
void DestroyRange(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, std::allocator<FR2Vect3TC<int16_t>>& _Al);
// _Destroy_range(_First, _Last, _Al, _Nonscalar_ptr_iterator_tag) of FR2Vect3TC<short>
// (cdecl, ret 0, 0x65FF00). IDA name; not in the PDB.
void DestroyRange(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, std::allocator<FR2Vect3TC<int16_t>>& _Al,
	uint8_t _Cat);
// _Uninitialized_move(_First, _Last, _Dest, _Al) of FffModeGS and FffModeTexS (cdecl, ret 0): unchecks the source
// range and passes it on with _Dest's move category and a _Range_checked_iterator_tag. IDA names; not in the PDB.
FffModeGS* UninitializedMove(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, std::allocator<FffModeGS>& _Al);
FffModeTexS* UninitializedMove(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest,
	std::allocator<FffModeTexS>& _Al);
// _Uninit_move(_First, _Last, _Dest, _Al, _Cat, _Range_checked_iterator_tag) of FffModeGS
// (cdecl, ret 0, 0x660210) and FffModeTexS (cdecl, ret 0, 0x660230); _Cat and _Checked are the tags' bytes. IDA
// names; not in the PDB.
FffModeGS* UninitMove(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, std::allocator<FffModeGS>& _Al,
	uint8_t _Cat, uint8_t _Checked);
FffModeTexS* UninitMove(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest, std::allocator<FffModeTexS>& _Al,
	uint8_t _Cat, uint8_t _Checked);
// unchecked_copy(_First, _Last, _Dest) of FffModeTexS (0x65E260), FR2Vect3TC<short> (0x65F820) and bytes (0x65F8C0),
// which the vectors' erase (0x65D8E0, 0x65F5A0, 0x65F700) call: unchecks the source and copies through _Copy_opt
// (0x65E7C0, 0x65FD80, 0x65FE60) with the category tags (cdecl, ret 0). IDA names; not in the PDB. 0x65F820 and
// 0x65F8C0 are KEEP, unregistered: the exe passes on a tag byte nothing writes.
FffModeTexS* Copy(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest);
FR2Vect3TC<int16_t>* UncheckedMove(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
uint8_t* UncheckedMove(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest);
// unchecked_uninitialized_copy(_First, _Last, _Dest, _Al) of FR2Vect3TC<short> (0x65FDE0), FffModeGS (0x660340) and
// FffModeTexS (0x6603A0): unchecks the source and hands it to _Uninit_copy (0x660070, 0x660510, 0x6605D0) with the
// pointer category tag of _First and _Dest and a _Range_checked_iterator_tag (cdecl, ret 0). IDA names; not in the
// PDB.
FR2Vect3TC<int16_t>* UninitializedCopy(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last,
	FR2Vect3TC<int16_t>* _Dest, std::allocator<FR2Vect3TC<int16_t>>& _Al);
FffModeGS* UninitializedCopy(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, std::allocator<FffModeGS>& _Al);
FffModeTexS* UninitializedCopy(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest,
	std::allocator<FffModeTexS>& _Al);
// The FR2Vect3TC<short> vector's and the byte vector's out-of-line members, which their copy constructors call. IDA
// names; each is the VC9 member the PDB names, free here because src's vector declares it inline (size, begin, end)
// or because the instance is not reconstructed yet. The allocator temporary's copy constructor is
// allocator::Construct (xmemory.h).
// The base constructor _Container_base_aux_alloc_real, src's vector::_Construct_aux (thiscall,
// ret 4, 0x65D3A0): one body for every element type (the exe's 15 callers are vector<T>::vector() of different T), so
// apThis is untyped. The exe builds the iterator proxy through vector<float>::_Construct_aux (0x65DC90) and copies
// the allocator into _Alval at +8.
void* VectorValConstruct(void* apThis, uint32_t _Al);
// vector::_Buy(_Capacity) (thiscall, ret 4): 0x65F380 (FR2Vect3TC<short>, not reconstructed yet), 0x657EE0 (byte).
uintptr_t VectorBuy(FR2Vect3SVector* apThis, size_t _Capacity);
uintptr_t VectorBuy(FffModeTexImgVector* apThis, size_t _Capacity);
// vector::_Ucopy(_First, _Last, _Ptr): the iterators by value as two words each (thiscall, ret 0x14). 0x65FF30
// (FR2Vect3TC<short>), 0x65FF60 (byte).
FR2Vect3TC<int16_t>* VectorUcopy(FR2Vect3SVector* apThis, FR2Vect3SVector::const_iterator _First,
	FR2Vect3SVector::const_iterator _Last, FR2Vect3TC<int16_t>* _Ptr);
uint8_t* VectorUcopy(FffModeTexImgVector* apThis, FffModeTexImgVector::const_iterator _First,
	FffModeTexImgVector::const_iterator _Last, uint8_t* _Ptr);
// vector::_Tidy of FR2Vect3TC<short>.
void VectorTidy(FR2Vect3SVector* apThis);
// The element destroy the catch of _Uninit_fill_n<FffModeTexS> calls, with the 0 it pushes
// (cdecl, one argument, ret 0, 0x65EB50; IDA name).
void Destroy(FffModeTexS* apThis);
// Not named in the PDB or IDA; name chosen here. allocator<FffTriVtxLabelS>::destroy: a1 through
// ExceptionPtrDestroy_1; eax is that call's.
FffTriVtxLabelS* DestroyTriVtxLabel(FffTriVtxLabelS* a1);
void* DestroyTriSurfLabel(void* a1);
// Not named in the PDB or IDA; name chosen here. std::_Destroy(_Ptr) of FffTriSurfLabelS:
// _Ptr->~FffTriSurfLabelS() through the scalar deleting destructor 0x65EC40 with flags 0; eax is that call's, the
// element (cdecl, ret 0, 0x65EB10). allocator<FffTriSurfLabelS>::destroy (0x65E9E0) calls it.
FffTriSurfLabelS* Destroy(FffTriSurfLabelS* _Ptr);
// _Copy_opt's loop of FffModeTexS (0x58-byte elements): assigns
// [a1, a2) to a3 element by element through FffModeTexS::Assign and returns the end of the copies.
FffModeTexS* CopyOpt(const FffModeTexS* a1, const FffModeTexS* a2, FffModeTexS* a3);
// Not named in the PDB or IDA; name chosen here. std::_Construct(_Ptr, _Val) of
// FR2Vect3TC<short>: placement new of 6 bytes at _Ptr, then the dword and word copied from _Val when the storage is
// not null; eax is the element or null (cdecl, ret 0, 0x6602B0).
FR2Vect3TC<int16_t>* Construct(FR2Vect3TC<int16_t>* _Ptr, const FR2Vect3TC<int16_t>& _Val);
// IDA name; not in the PDB. std::_Destroy(_Ptr) of FffTriVtxLabelS: _Ptr->~FffTriVtxLabelS(); returns _Ptr (cdecl,
// ret 0, 0x65EB00).
FffTriVtxLabelS* ExceptionPtrDestroy_1(FffTriVtxLabelS* a1);
// _Copy_opt's loop of FffModeGS (0x1C-byte elements): assigns
// [a1, a2) to a3 element by element through FffModeGS::Assign and returns the end of the copies.
FffModeGS* CopyOpt(const FffModeGS* a1, const FffModeGS* a2, FffModeGS* a3);
// Not named in the PDB or IDA; name chosen here. VC9's _Checked_base(_It) of a checked vector iterator:
// _It._Checked_iterator_base(), the iterator's element pointer at +4 (cdecl, ret 0). One body for every
// instantiation (the FR2Vect3TC<short> and byte iterators' callers 0x660130 and 0x660190), so apIterator is untyped.
void* CheckedBase(void* apIterator);
// std::copy(_First, _Last, _Dest) of FffModeGS (cdecl, ret 0), Copy's twin. Not named in the PDB or IDA; name
// chosen here.
FffModeGS* Copy(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest);
void DestroyTriDiffMorph(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);
void DestroyTriStatMorph(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);
// Not named in the PDB or IDA; name chosen here. std::_Destroy(_Ptr) of FffModeGS: _Ptr->~FffModeGS(); returns _Ptr
// (cdecl, ret 0). allocator<FffModeGS>::destroy (0x65EA40) and the vector's _Insert_n (0xAB1800) call it.
FffModeGS* Destroy(FffModeGS* a1);
// Not named in the PDB or IDA; name chosen here. _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag,
// _Nonscalar_ptr_iterator_tag, _Range_checked_iterator_tag) of FR2Vect3TC<short> (cdecl, ret 0): copies through
// CopyOpt and returns the end of the copy. KEEP: it passes a frame byte nothing writes (the body says where).
FR2Vect3TC<int16_t>* Copy(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
// _Copy_opt of bytes: memmove_s of [_First, _Last) to _Dest when the
// count, compared signed, is positive; returns the end of the copy.
uint8_t* CopyOpt(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest);
// VC9's unchecked_uninitialized_copy(_First, _Last, _Dest, _Al) of
// bytes: the pointer category tag of _First and _Dest, both pointers unchecked, then the scalar _Uninit_copy
// UninitCopy.
uint8_t* UninitializedCopy(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest, std::allocator<uint8_t>& _Al);
// _Copy_opt's loop of FR2Vect3TC<short>: assigns [_First, _Last) to
// _Dest element by element (a dword, then a word) and returns the end of the copies.
FR2Vect3TC<int16_t>* CopyOpt(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's unchecked_uninitialized_copy(_First, _Last, _Dest, _Al) of a
// checked vector iterator range: the category tag of _First and _Dest, both iterators unchecked through CheckedBase,
// then _Uninit_copy (cdecl, ret 0; the iterators by value as two words each). 0x660130 (FR2Vect3TC<short>), 0x660190
// (byte).
FR2Vect3TC<int16_t>* UninitializedCopy(FR2Vect3SVector::const_iterator _First, FR2Vect3SVector::const_iterator _Last,
	FR2Vect3TC<int16_t>* _Dest, std::allocator<FR2Vect3TC<int16_t>>& _Al);
uint8_t* UninitializedCopy(FffModeTexImgVector::const_iterator _First, FffModeTexImgVector::const_iterator _Last,
	uint8_t* _Dest, std::allocator<uint8_t>& _Al);
// VC9's _Uninit_copy(_First, _Last, _Dest, _Al,
// _Scalar_ptr_iterator_tag, _Range_checked_iterator_tag) of bytes: memmove_s of [Source, a2) to Destination when the
// count is not 0; returns the end of the copy. The allocator and the two tags are not read.
uint8_t* UninitCopy(uint8_t* Source, uint8_t* a2, void* Destination, std::allocator<uint8_t>& _Al, uint8_t _Cat,
	uint8_t _Checked);
// _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag) of FffModeGS (cdecl, ret 0), CopyOpt's twin: the three tags are pushed as bytes and
// never read. Returns _Dest past the copied elements. Not named in the PDB or IDA; name chosen here.
FffModeGS* CopyOpt(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, uint8_t _IterCat, uint8_t _PtrCat,
	uint8_t _Checked);
// _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag) of FffModeTexS (cdecl, ret 0): the three tags are pushed as bytes and never read.
// Returns _Dest past the copied elements. Not named in the PDB or IDA; name chosen here.
FffModeTexS* CopyOpt(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest, uint8_t _IterCat, uint8_t _PtrCat,
	uint8_t _Checked);
// Not named in the PDB or IDA; name chosen here. VC9's _Unchecked_move_backward(_First, _Last, _Dest) of
// FR2Vect3TC<short>, which vector<FR2Vect3TC<short>>::_Insert_n (0xAB0FD0) calls (cdecl, ret 0). KEEP: the body says
// why.
FR2Vect3TC<int16_t>* UncheckedMoveBackward(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last,
	FR2Vect3TC<int16_t>* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's _Uninitialized_move(_First, _Last, _Dest, _Al) of
// FR2Vect3TC<short>, which vector<FR2Vect3TC<short>>::_Insert_n (0xAB0FD0) calls (cdecl, ret 0). KEEP: the body says
// why.
FR2Vect3TC<int16_t>* UncheckedUninitializedMove(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last,
	FR2Vect3TC<int16_t>* _Dest, std::allocator<FR2Vect3TC<int16_t>>& _Al);
// Not named in the PDB or IDA; name chosen here. VC9's _Move_backward_opt of FR2Vect3TC<short>: copies [_First,
// _Last) backward to end at _Dest and returns the start of the copies (cdecl, ret 0). The exe's callers push three
// tag bytes after _Dest, which it never reads; src takes none. KEEP: the body says why.
FR2Vect3TC<int16_t>* CopyBackwardOpt(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last,
	FR2Vect3TC<int16_t>* _Dest);
// Copies the modes [a1, a2) backward to the ones ending at a3, each
// through FffModeGS's assignment inlined, and returns the first one written.
FffModeGS* CopyBackwardOpt(FffModeGS* a1, FffModeGS* a2, FffModeGS* a3);
// Not named in the PDB or IDA; name chosen here. Copies the FR2Vect2TC<float> elements
// [_First, _Last) over _Dest and returns the end of the copies (vector::erase, 0xABAF60).
FR2Vect2TC<float>* CopyOpt(FR2Vect2TC<float>* _First, FR2Vect2TC<float>* _Last, FR2Vect2TC<float>* _Dest);
// Copies the FR2Vect3TC<float> elements
// [_First, _Last) over _Dest and returns the end of the copies; vector::Assign pushes three tag bytes
// after the three pointers, which it does not read.
FR2Vect3TC<float>* CopyOpt(FR2Vect3TC<float>* _First, FR2Vect3TC<float>* _Last, FR2Vect3TC<float>* _Dest);

inline constexpr fnvd::Vtable BSFaceGenMorphData_vftable{};
inline constexpr fnvd::Vtable BSFaceGenMorphDataHead_vftable{};
inline constexpr fnvd::Vtable BSFaceGenModel_vftable{};

// allocator::construct and allocator::destroy of the FaceGen element classes are declared in xmemory.h.
