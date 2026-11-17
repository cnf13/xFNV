#pragma once

#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <memory>
#include <stdint.h>

class BSTreeNode;
class BipedAnim;
class TESBoundObject;
class TESObjectCELL;
class TESObjectList;
class TESWaterForm;

// Not named in the PDB or IDA; name chosen here. _Destroy_range(_First, _Last, _Al) of the float elements (the byte
// vector's share it): the category tag of [_First, _Last), then the scalar overload, which is the exe's shared empty
// function (0x40FBE0) (cdecl, ret 0). VC9 returns nothing.
void DestroyRange(float* _First, float* _Last, std::allocator<float>& _Al);

// A short integer 3-vector, used for object bounds.
struct NiNPShortPoint3 {
	// The exe's BOUND_DATA constructor leaves the points unset; a user constructor removes the implicit default.
	NiNPShortPoint3() = default;
	// The exe's constructor (thiscall, ret 0xC, 0x50E960); returns apMemory. The PDB's
	// parameters are shorts, but the exe's callers push whole ints and the constructor reads only their low words.
	NiNPShortPoint3(int32_t aiX, int32_t aiY, int32_t aiZ);

	float GetFloatX() const;
	float GetFloatY() const;
	float GetFloatZ() const;
	float Length() const;
	// IDA name (Bounds::CalculateRelativeBounds); not in the PDB, whose binary operator- this is, kept as a named
	// member so that callers get the exe's returned result pointer. Writes the component-wise difference
	// this - arPoint to apResult and returns apResult (thiscall, ret 8).
	NiNPShortPoint3* CalculateRelativeBounds(NiNPShortPoint3* apResult, const NiNPShortPoint3& arPoint) const;
	// The exe's out-of-line copy assignment: the three shorts, then this (thiscall, ret 4, 0x50E8B0).
	NiNPShortPoint3& operator=(const NiNPShortPoint3& arPt);

	static const NiNPShortPoint3& GetZERO();

	int16_t x;
	int16_t y;
	int16_t z;

	static NiNPShortPoint3 ZERO;
};

// A form that lives in an object list (the base of everything placeable).
class TESObject : public TESForm {
public:
	TESObject();
	~TESObject() override;

	bool IsObject() override { return true; }
	virtual NiAVObject* Load3D() { return nullptr; }
	virtual bool IsBoundAnimObject() { return false; }
	virtual TESWaterForm* GetWaterType() { return nullptr; }
	virtual bool IsAutoCalc() { return false; }
	virtual void SetAutoCalc(bool) {}
	virtual NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy);
	virtual void UnClone3D(TESObjectREFR* apRequester);
	virtual bool IsMarker();
	virtual bool IsOcclusionMarker();
	virtual void InitDefaultWorn() {}
	// MSVC lays overloads out in reverse declaration order: ReplaceModel() takes the lower slot
	virtual bool ReplaceModel(BipedAnim*) { return false; }
	virtual bool ReplaceModel();
	// The exe's shared empty stub (thiscall, ret 8), which overrides call out of line.
	virtual void Set3D(NiAVObject* apObject, bool abTestProcessLevel) {}
	// count a reference that uses the object, and release it
	virtual uint32_t IncRef() { return 0; }
	virtual uint32_t DecRef() { return 0; }
	virtual NiAVObject* LoadGraphics(TESObjectREFR* apRef);

	// The base form's 3D: always null in this build (the PDB's TESObject::Get3D, defined in ImpactMixer.cpp; the exe
	// calls it out of line on creature forms).
	NiAVObject* Get3D();
	// The model file apRef shows the object with.
	const char* GetModelFileName(TESObjectREFR* apRef);
	// Writes the model file name scaled for apRef to apScaledFileName.
	void GetScaledModelFileName(TESObjectREFR* apRef, char* apScaledFileName);
	// nod_lpNext.
	TESObject* nod_GetNext() { return nod_lpNext; }

	TESObjectList* m_pParentList;
	TESObject* nod_lpPrev;
	TESObject* nod_lpNext;
};

// An object with a bounding box.
class TESBoundObject : public TESObject {
public:
	struct BOUND_DATA {
		BOUND_DATA();

		// Swaps the bounds read from a big-endian file, the maximum first.
		void Endian();

		NiNPShortPoint3 BoundMin;
		NiNPShortPoint3 BoundMax;
	};

	TESBoundObject();
	~TESBoundObject() override;

	void SaveObjectBound() override;
	void LoadObjectBound(TESFile* apFile) override;
	bool IsBoundObject() override { return true; }
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;
	bool ReplaceModel() override;

	virtual NiAVObject* Clone3D(TESObjectREFR* pRequester);
	virtual bool ReplaceModel(const char* pfilename);

	using TESObject::ReplaceModel;

	// The length of the bound box's diagonal (x87 return). thiscall, ret 0, 0x50EBF0.
	float GetBoundSize();
	// The bound box's corners as floats (a class result: thiscall, ret 4, 0x50EAD0 and
	// 0x50EB90).
	NiPoint3 GetBoundMin();
	NiPoint3 GetBoundMax();
	void CopyBoundData(TESBoundObject* apCopy);

	BOUND_DATA BoundData;
};

class TESBoundAnimObject : public TESBoundObject {
public:
	TESBoundAnimObject();
	~TESBoundAnimObject() override;

	bool IsBoundAnimObject() override { return true; }
	void Set3D(NiAVObject* apObject, bool abTestProcessLevel) override;
	bool ReplaceModel(const char* pfilename) override;
	using TESBoundObject::ReplaceModel;
};

// A bound object drawn as a SpeedTree tree.
class TESBoundTreeObject : public TESBoundObject {
public:
	TESBoundTreeObject();
	~TESBoundTreeObject() override;

	NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;
	void UnClone3D(TESObjectREFR*) override {}
	bool ReplaceModel() override { return true; }
	NiAVObject* LoadGraphics(TESObjectREFR*) override { return nullptr; }

	virtual void RefreshBound(BSTreeNode*) {}
	using TESBoundObject::Clone3D;
	using TESBoundObject::ReplaceModel;
};

// The data handler's list of every base object, linked through TESObject::nod_lpPrev/nod_lpNext.
class TESObjectList {
public:
	TESObjectList(bool modelload);
	~TESObjectList();

	// Links lpNew in after the tail.
	TESObject* lnk_Append(TESObject* lpNew);
	// IDA name; not in the PDB (the PDB's lnk_Insert takes one parameter). Links lpNew in after lpPrev and returns
	// it (thiscall, ret 8, 0x5101E0). lpPrev is a TESObject: the exe reads and writes only its nod_lpNext there and
	// its caller lnk_Append passes lnk_lpTail.
	TESObject* lnk_Place(TESObject* lpPrev, TESObject* lpNew);
	// The PDB returns TESObject* (the exe returns TESObjectREFR::GetObjectReference's TESBoundObject*, which
	// converts); its parameter type, TESObject*, contradicts the exe's callee, which takes a TESObjectREFR.
	TESObject* lnk_GetNext(TESObject* lpNode);
	void lnk_Remove(TESObject* lpNode);
	void lnk_Destroy();
	TESObject* lnk_GetPrev(TESObject* lpNode);

	// The exe's deleting destructor (0x45DF10) frees through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t lnk_dwCount;
	TESObject* lnk_lpRoot;
	TESObject* lnk_lpTail;
	TESObjectCELL* pOwner;
};

inline NiNPShortPoint3 NiNPShortPoint3::ZERO{};
inline constexpr fnvd::Vtable TESObject_vftable{};
inline constexpr fnvd::Vtable TESBoundObject_vftable{};

// The number of scaled models TESBoundObject::Clone3D added to the model loader (a file static int in the PDB).
inline int32_t iNumScaledModelsAdded;
inline int32_t iNumScaledModelsCloned;

// The guard of NiNPShortPoint3::GetZERO's function-local static ZERO.
inline int32_t ZERO_guard;
inline constexpr fnvd::Vtable TESBoundAnimObject_vftable{};
inline constexpr fnvd::Vtable TESBoundTreeObject_vftable{};
