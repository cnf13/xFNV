#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/iosfwd.h"

#include <cstddef>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
class BSFaceGenBaseMorphExtraData;
class BSFaceGenModelMeshData;
class BSFaceGenModelTextureData;
class BSFaceGenMorph;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
struct EGMData;
struct FffModeGS;
struct FffModeTexS;
class FffTriFileDataS;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiSkinInstance;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

enum HAIR_PART : int32_t;

// FaceGen's element classes that the vector instantiations below hold; they are not modelled yet.
template <class T>
class FR2Vect2TC;
template <class T>
class FR2Vect3TC;

// The .tri morph data of a FaceGen mesh (head or hair). Not an NiObject, but it repeats NiObject's run-time type
// virtuals, as NiShader does.
class BSFaceGenMorphData : public NiRefObject {
public:
    BSFaceGenMorphData();
    ~BSFaceGenMorphData() override;

    virtual const NiRTTI* GetRTTI() const {
        return &ms_RTTI;
    }
    // type queries: the object as the named class, or null
    virtual NiNode* IsNode() {
        return nullptr;
    }
    virtual BSFadeNode* IsFadeNode() {
        return nullptr;
    }
    virtual BSMultiBoundNode* IsMultiBoundNode() {
        return nullptr;
    }
    virtual NiGeometry* IsGeometry() {
        return nullptr;
    }
    virtual NiTriBasedGeom* IsTriBasedGeom() {
        return nullptr;
    }
    virtual NiTriStrips* IsTriStrips() {
        return nullptr;
    }
    virtual NiTriShape* IsTriShape() {
        return nullptr;
    }
    virtual BSSegmentedTriShape* IsSegmentedTriShape() {
        return nullptr;
    }
    virtual BSResizableTriShape* IsResizableTriShape() {
        return nullptr;
    }
    virtual NiParticles* IsParticlesGeom() {
        return nullptr;
    }
    virtual NiLines* IsLinesGeom() {
        return nullptr;
    }
    virtual bhkNiCollisionObject* IsBhkNiCollisionObject() {
        return nullptr;
    }
    virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject() {
        return nullptr;
    }
    virtual bhkRigidBody* IsBhkRigidBody() const {
        return nullptr;
    }
    virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() {
        return nullptr;
    }

    // NiObject's member DynamicCast repeated: this when
    // IsKindOfMember holds, else null.
    BSFaceGenMorphData* DynamicCastMember(const NiRTTI* apRTTI) const;
    // NiObject's member IsKindOf repeated: whether the run-time type is apRTTI's class or derives from it.
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
    void ApplyMorph(
        BS_FACEGEN_MORPH_TYPE aeType,
        int32_t aiIndex,
        NiTriBasedGeom* apGeom,
        BSFaceGenBaseMorphExtraData* apBaseMorph,
        float afModifier
    );
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
    bool LoadModel(
        const char* apEGMFilename,
        const char* apNIFFilename,
        const char* apTRIFilename,
        const char* apEGTFilename,
        HAIR_PART aeHairPart,
        bool abIsAccessory
    );
    // Loads the mesh data once: the .egm name, the model's geometry and the .tri morph data.
    bool LoadModelMesh(
        const char* apEGMFilename,
        const char* apNIFFilename,
        const char* apTRIFilename,
        HAIR_PART aeHairPart,
        bool abIsAccessory
    );
    // Loads the texture data once: allocates it and copies the .egt file name; true unless it was already loaded or
    // apEGTFilename is null, which also return true.
    bool LoadModelTexture(const char* apEGTFilename);
    // Clones the model's mesh into aspReturnMesh.
    bool CreateNewMesh(NiPointer<NiTriBasedGeom>& aspReturnMesh);
    // Builds a new mesh and applies apCoord to it; on failure the new mesh is released.
    bool ApplyCoordinateToNewMesh(const FR2MatrixVTC<float> (*apCoord)[2][2], NiPointer<NiTriBasedGeom>& aspReturnMesh);
    // Morphs apMesh by apCoord, the face's coordinate when apCoord is null.
    bool ApplyCoordinateToExistingMesh(
        const FR2MatrixVTC<float> (*apCoord)[2][2],
        NiTriBasedGeom* apMesh,
        bool abForceBaseMorph
    );
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
    uint32_t GetRefCount() const {
        return m_uiRefCount;
    }
    bool HasEGTData() const;
    NiSkinInstance* GetSkinInstance() const;
    void ForceLoadModelTexture();
    bool ApplyCoordinateTexturingToMesh(
        const FR2MatrixVTC<float> (*apCoord)[2][2],
        NiPointer<NiTexture>& aspReturnBaseMod,
        uint32_t aiForceBasisSize
    );
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

// The engine's NiPointer of mesh data takes and releases references through the data's IncRefCount and DecRefCount.
inline void NiIncRefCount(BSFaceGenModelMeshData* apData) {
    apData->IncRefCount();
}

inline void NiDecRefCount(BSFaceGenModelMeshData* apData) {
    apData->DecRefCount();
}

// The 6-byte FR2Vect3TC<short>: the diff morphs' vertex deltas and FffModeGS's vertex offsets.
template <>
class FR2Vect3TC<int16_t> {
public:
    int16_t x1;
    int16_t x2;
    int16_t x3;
};

// The 12-byte FR2Vect3TC<float>: the .tri reader's vertices. The int instance has the same shape.
template <>
class FR2Vect3TC<float> {
public:
    // The element-wise sum with v1, built in apResult.
    FR2Vect3TC* Add(FR2Vect3TC* apResult, const FR2Vect3TC& v1) const;
    // Multiplies each element by val and builds the result in apResult.
    FR2Vect3TC* MulScalar(FR2Vect3TC* apResult, float val) const;

    float x1;
    float x2;
    float x3;
};

// FR2Vect2TC<float>: a .tri texture coordinate. This file's vector rows copy one element as two floats.
template <>
class FR2Vect2TC<float> {
public:
    float x1;
    float x2;
};

// The 0x1C-byte mode of a FaceGen mesh: its scale and its vertex offsets.
struct FffModeGS {
    FffModeGS() = default;
    FffModeGS(const FffModeGS& _Right);
    ~FffModeGS();

    // Copy assignment: the scale, then the vertex offsets' vector assignment; returns this. Not declared as operator=:
    // a user-declared copy assignment would make FffModeGS non-trivially copyable (vector's Assign has the same
    // reason).
    FffModeGS& Assign(const FffModeGS& _Right);

    float scale;
    std::vector<FR2Vect3TC<int16_t>, std::allocator<FR2Vect3TC<int16_t>>> vertPos;
};

// The texture mode of a FaceGen mesh: its scale, width, height and row padding, then its three byte vectors of image
// data. fffReadEgtFile stores the .egt header's column and row counts in width and height.
struct FffModeTexS {
    FffModeTexS();
    FffModeTexS(const FffModeTexS& _Right);
    ~FffModeTexS();

    // Copy assignment: copies the scale and the three dimensions, then assigns each image vector. Not declared as
    // operator=, as FffModeGS::Assign.
    FffModeTexS& Assign(const FffModeTexS& _Right);

    static void operator delete(void* apMemory);

    float scale;
    uint32_t width;
    uint32_t height;
    uint32_t rowPad;
    std::vector<uint8_t, std::allocator<uint8_t>> img[3];
};

// FffModeGS::vertPos: the mode's vertex offsets.
using FR2Vect3SVector = std::vector<FR2Vect3TC<int16_t>, std::allocator<FR2Vect3TC<int16_t>>>;
// EGTData::EGTBasis: the texture modes of each of the two bases.
using FffModeTexSVector = std::vector<FffModeTexS, std::allocator<FffModeTexS>>;
// BSFaceGenModel's morph vectors.
using FR2Vect3FloatVector = std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using FutVect4Vector = std::vector<FutVect4TC<int32_t>, std::allocator<FutVect4TC<int32_t>>>;
using FffTriStatMorphVector = std::vector<FffTriStatMorphS, std::allocator<FffTriStatMorphS>>;

// FffModeTexS::img: the three byte vectors of a texture mode's image data.
using FffModeTexImgVector = std::vector<uint8_t, std::allocator<uint8_t>>;

// Returns its first argument. Callers push one to three arguments.
int32_t return_value(int32_t a1, ...);
uint32_t BSMIN(uint16_t a, uint32_t b);
// The modes' backward move dispatcher:
// passes _First, _Last, _Dest and the iterator-category tag _Cat (the fourth argument's byte) on to the
// backward copy CopyBackwardOpt (FffModeGS steps of 0x1C) with two more tag bytes, and returns its result.
// vector<FffModeGS>::_Insert_n calls it with six arguments; a5 and a6 are not read.
FffModeGS* MoveBackwardOpt(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, uint8_t _Cat, uint8_t a5, uint8_t a6);
// _Destroy_range(_First, _Last, _Al): passes the elements on with their category tag. One overload per element class.
void DestroyRange(FR2Vect3TC<float>* _First, FR2Vect3TC<float>* _Last, std::allocator<FR2Vect3TC<float>>& _Al);
void DestroyRange(FR2Vect2TC<float>* _First, FR2Vect2TC<float>* _Last, std::allocator<FR2Vect2TC<float>>& _Al);
void DestroyRange(FffTriVtxLabelS* _First, FffTriVtxLabelS* _Last, std::allocator<FffTriVtxLabelS>& _Al);
void DestroyRange(FffTriDiffMorphS* _First, FffTriDiffMorphS* _Last, std::allocator<FffTriDiffMorphS>& _Al);
void DestroyRange(FffTriStatMorphS* _First, FffTriStatMorphS* _Last, std::allocator<FffTriStatMorphS>& _Al);
void DestroyRange(FffModeGS* _First, FffModeGS* _Last, std::allocator<FffModeGS>& _Al);
void DestroyRange(FffTriSurfLabelS* _First, FffTriSurfLabelS* _Last, std::allocator<FffTriSurfLabelS>& _Al);
// _Destroy_range(_First, _Last, _Al, _Nonscalar_ptr_iterator_tag): destroys the elements one by one through
// _Al.destroy. _Cat is the tag's byte (see _Move_cat).
void DestroyRange(
    FR2Vect3TC<float>* _First,
    FR2Vect3TC<float>* _Last,
    std::allocator<FR2Vect3TC<float>>& _Al,
    uint8_t _Cat
);
void DestroyRange(
    FR2Vect2TC<float>* _First,
    FR2Vect2TC<float>* _Last,
    std::allocator<FR2Vect2TC<float>>& _Al,
    uint8_t _Cat
);
void DestroyRange(FffTriVtxLabelS* _First, FffTriVtxLabelS* _Last, std::allocator<FffTriVtxLabelS>& _Al, uint8_t _Cat);
void DestroyRange(
    FffTriDiffMorphS* _First,
    FffTriDiffMorphS* _Last,
    std::allocator<FffTriDiffMorphS>& _Al,
    uint8_t _Cat
);
void DestroyRange(
    FffTriStatMorphS* _First,
    FffTriStatMorphS* _Last,
    std::allocator<FffTriStatMorphS>& _Al,
    uint8_t _Cat
);
void DestroyRange(FffModeGS* _First, FffModeGS* _Last, std::allocator<FffModeGS>& _Al, uint8_t _Cat);
void DestroyRange(
    FffTriSurfLabelS* _First,
    FffTriSurfLabelS* _Last,
    std::allocator<FffTriSurfLabelS>& _Al,
    uint8_t _Cat
);
// Construct(_Ptr, _Val) of FffModeGS and FffModeTexS: placement new, then the element's copy constructor.
FffModeGS* Construct(FffModeGS* _Ptr, const FffModeGS& _Val);
FffModeTexS* Construct(FffModeTexS* _Ptr, const FffModeTexS& _Val);
// _Destroy_range(_First, _Last, _Al) of FR2Vect3TC<short>.
void DestroyRange(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, std::allocator<FR2Vect3TC<int16_t>>& _Al);
// _Destroy_range(_First, _Last, _Al, _Nonscalar_ptr_iterator_tag) of FR2Vect3TC<short>.
void DestroyRange(
    FR2Vect3TC<int16_t>* _First,
    FR2Vect3TC<int16_t>* _Last,
    std::allocator<FR2Vect3TC<int16_t>>& _Al,
    uint8_t _Cat
);
// _Uninitialized_move(_First, _Last, _Dest, _Al) of FffModeGS and FffModeTexS: unchecks the source range and passes it
// on with _Dest's move category and a _Range_checked_iterator_tag.
FffModeGS* UninitializedMove(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, std::allocator<FffModeGS>& _Al);
FffModeTexS*
UninitializedMove(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest, std::allocator<FffModeTexS>& _Al);
// _Uninit_move(_First, _Last, _Dest, _Al, _Cat, _Range_checked_iterator_tag) of FffModeGS and FffModeTexS; _Cat and
// _Checked are the tags' bytes.
FffModeGS* UninitMove(
    FffModeGS* _First,
    FffModeGS* _Last,
    FffModeGS* _Dest,
    std::allocator<FffModeGS>& _Al,
    uint8_t _Cat,
    uint8_t _Checked
);
FffModeTexS* UninitMove(
    FffModeTexS* _First,
    FffModeTexS* _Last,
    FffModeTexS* _Dest,
    std::allocator<FffModeTexS>& _Al,
    uint8_t _Cat,
    uint8_t _Checked
);
// unchecked_copy(_First, _Last, _Dest) of FffModeTexS, FR2Vect3TC<short> and bytes, which the vectors' erase call:
// unchecks the source and copies through _Copy_opt with the category tags.
FffModeTexS* Copy(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest);
FR2Vect3TC<int16_t>* UncheckedMove(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
uint8_t* UncheckedMove(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest);
// unchecked_uninitialized_copy(_First, _Last, _Dest, _Al) of FR2Vect3TC<short>, FffModeGS and FffModeTexS: unchecks the
// source and hands it to _Uninit_copy with the pointer category tag of _First and _Dest and a
// _Range_checked_iterator_tag.
FR2Vect3TC<int16_t>* UninitializedCopy(
    FR2Vect3TC<int16_t>* _First,
    FR2Vect3TC<int16_t>* _Last,
    FR2Vect3TC<int16_t>* _Dest,
    std::allocator<FR2Vect3TC<int16_t>>& _Al
);
FffModeGS* UninitializedCopy(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, std::allocator<FffModeGS>& _Al);
FffModeTexS*
UninitializedCopy(FffModeTexS* _First, FffModeTexS* _Last, FffModeTexS* _Dest, std::allocator<FffModeTexS>& _Al);
// The FR2Vect3TC<short> vector's and the byte vector's members, which their copy constructors call. VectorValConstruct
// is one body for every element type, so apThis is untyped.
void* VectorValConstruct(void* apThis, uint32_t _Al);
uintptr_t VectorBuy(FR2Vect3SVector* apThis, size_t _Capacity);
uintptr_t VectorBuy(FffModeTexImgVector* apThis, size_t _Capacity);
FR2Vect3TC<int16_t>* VectorUcopy(
    FR2Vect3SVector* apThis,
    FR2Vect3SVector::const_iterator _First,
    FR2Vect3SVector::const_iterator _Last,
    FR2Vect3TC<int16_t>* _Ptr
);
uint8_t* VectorUcopy(
    FffModeTexImgVector* apThis,
    FffModeTexImgVector::const_iterator _First,
    FffModeTexImgVector::const_iterator _Last,
    uint8_t* _Ptr
);
// vector::_Tidy of FR2Vect3TC<short>.
void VectorTidy(FR2Vect3SVector* apThis);
// The element destroy that the catch of _Uninit_fill_n<FffModeTexS> calls.
void Destroy(FffModeTexS* apThis);
// allocator<FffTriVtxLabelS>::destroy.
FffTriVtxLabelS* DestroyTriVtxLabel(FffTriVtxLabelS* a1);
void* DestroyTriSurfLabel(void* a1);
// std::_Destroy(_Ptr) of FffTriSurfLabelS: calls _Ptr->~FffTriSurfLabelS() without freeing the memory and returns the
// element. allocator<FffTriSurfLabelS>::destroy calls it.
FffTriSurfLabelS* Destroy(FffTriSurfLabelS* _Ptr);
// _Copy_opt's loop of FffModeTexS (0x58-byte elements): assigns
// [a1, a2) to a3 element by element through FffModeTexS::Assign and returns the end of the copies.
FffModeTexS* CopyOpt(const FffModeTexS* a1, const FffModeTexS* a2, FffModeTexS* a3);
// std::_Construct(_Ptr, _Val) of FR2Vect3TC<short>: placement new at _Ptr, then _Val is copied in when the storage is
// not null; returns the element or null.
FR2Vect3TC<int16_t>* Construct(FR2Vect3TC<int16_t>* _Ptr, const FR2Vect3TC<int16_t>& _Val);
// std::_Destroy(_Ptr) of FffTriVtxLabelS: _Ptr->~FffTriVtxLabelS(); returns _Ptr.
FffTriVtxLabelS* ExceptionPtrDestroy_1(FffTriVtxLabelS* a1);
// _Copy_opt's loop of FffModeGS (0x1C-byte elements): assigns
// [a1, a2) to a3 element by element through FffModeGS::Assign and returns the end of the copies.
FffModeGS* CopyOpt(const FffModeGS* a1, const FffModeGS* a2, FffModeGS* a3);
// _Checked_base(_It) of a checked vector iterator: returns the iterator's element pointer. One body serves every
// instantiation (the FR2Vect3TC<short> and byte iterators), so apIterator is untyped.
void* CheckedBase(void* apIterator);
// std::copy(_First, _Last, _Dest) of FffModeGS, Copy's twin.
FffModeGS* Copy(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest);
void DestroyTriDiffMorph(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);
void DestroyTriStatMorph(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);
// std::_Destroy(_Ptr) of FffModeGS: _Ptr->~FffModeGS(); returns _Ptr. allocator<FffModeGS>::destroy and the vector's
// _Insert_n call it.
FffModeGS* Destroy(FffModeGS* a1);
// Copies through CopyOpt and returns the end of the copy. It deliberately passes a byte nothing writes, as the game
// does (the body says where).
FR2Vect3TC<int16_t>* Copy(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
// _Copy_opt of bytes: memmove_s of [_First, _Last) to _Dest when the
// count, compared signed, is positive; returns the end of the copy.
uint8_t* CopyOpt(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest);
uint8_t* UninitializedCopy(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest, std::allocator<uint8_t>& _Al);
// Assigns [_First, _Last) to _Dest element by element and returns the end of the copies.
FR2Vect3TC<int16_t>* CopyOpt(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
FR2Vect3TC<int16_t>* UninitializedCopy(
    FR2Vect3SVector::const_iterator _First,
    FR2Vect3SVector::const_iterator _Last,
    FR2Vect3TC<int16_t>* _Dest,
    std::allocator<FR2Vect3TC<int16_t>>& _Al
);
uint8_t* UninitializedCopy(
    FffModeTexImgVector::const_iterator _First,
    FffModeTexImgVector::const_iterator _Last,
    uint8_t* _Dest,
    std::allocator<uint8_t>& _Al
);
// Copies the bytes [Source, a2) to Destination with memmove_s when the count is not 0, and returns the end of the copy.
// _Al, _Cat and _Checked are not read.
uint8_t* UninitCopy(
    uint8_t* Source,
    uint8_t* a2,
    void* Destination,
    std::allocator<uint8_t>& _Al,
    uint8_t _Cat,
    uint8_t _Checked
);
// _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag, _Nonscalar_ptr_iterator_tag, _Range_checked_iterator_tag)
// of FffModeGS, CopyOpt's twin: the three tags are pushed as bytes and never read. Returns _Dest past the copied
// elements.
FffModeGS*
CopyOpt(FffModeGS* _First, FffModeGS* _Last, FffModeGS* _Dest, uint8_t _IterCat, uint8_t _PtrCat, uint8_t _Checked);
// _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag, _Nonscalar_ptr_iterator_tag, _Range_checked_iterator_tag)
// of FffModeTexS: the three tags are pushed as bytes and never read. Returns _Dest past the copied elements.
FffModeTexS* CopyOpt(
    FffModeTexS* _First,
    FffModeTexS* _Last,
    FffModeTexS* _Dest,
    uint8_t _IterCat,
    uint8_t _PtrCat,
    uint8_t _Checked
);
// The _Unchecked_move_backward(_First, _Last, _Dest) of FR2Vect3TC<short> that vector<FR2Vect3TC<short>>::_Insert_n
// calls.
FR2Vect3TC<int16_t>*
UncheckedMoveBackward(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
FR2Vect3TC<int16_t>* UncheckedUninitializedMove(
    FR2Vect3TC<int16_t>* _First,
    FR2Vect3TC<int16_t>* _Last,
    FR2Vect3TC<int16_t>* _Dest,
    std::allocator<FR2Vect3TC<int16_t>>& _Al
);
// Copies [_First, _Last) backward to end at _Dest and returns the start of the copies.
FR2Vect3TC<int16_t>*
CopyBackwardOpt(FR2Vect3TC<int16_t>* _First, FR2Vect3TC<int16_t>* _Last, FR2Vect3TC<int16_t>* _Dest);
// Copies the modes [a1, a2) backward to the ones ending at a3, each through FffModeGS's assignment, and returns the
// first one written.
FffModeGS* CopyBackwardOpt(FffModeGS* a1, FffModeGS* a2, FffModeGS* a3);
// Copies the FR2Vect2TC<float> elements [_First, _Last) over _Dest and returns the end of the copies (used by
// vector::erase).
FR2Vect2TC<float>* CopyOpt(FR2Vect2TC<float>* _First, FR2Vect2TC<float>* _Last, FR2Vect2TC<float>* _Dest);
// Copies the FR2Vect3TC<float> elements
// [_First, _Last) over _Dest and returns the end of the copies; vector::Assign pushes three tag bytes
// after the three pointers, which it does not read.
FR2Vect3TC<float>* CopyOpt(FR2Vect3TC<float>* _First, FR2Vect3TC<float>* _Last, FR2Vect3TC<float>* _Dest);

// allocator::construct and allocator::destroy of the FaceGen element classes are declared in xmemory.h.
