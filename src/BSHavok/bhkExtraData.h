#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Reconstruction.hpp"

// Havok extra data on a scene graph object: an array of time controllers (pControllerA).
class bhkExtraData : public NiExtraData {
public:
	bhkExtraData();
	~bhkExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void ProcessClone(NiCloningProcess& arCloneData) override;

	// Adds a controller after the last slot of pControllerA, growing it when full (thiscall, ret 4, 0xCC2080). The PDB
	// has bhkExtraData::PrependController in this file and no address; the shape and LinkObject's use pick it.
	void PrependController(NiTimeController* apController);
	// The name and a clone of the controllers, to apDest.
	void CopyMembers(bhkExtraData* pDest, NiCloningProcess& arCloneData);

	// The class' stream loader entry.
	static NiObject* CreateObject();
	// The controller array's RemoveAll body as the exe keeps it for this instantiation (thiscall, ret 0, 0xCC1CF0);
	// the caller resets the array's vtable and frees its buffer afterwards.
	static void DestroyControllerArray(NiTLargePrimitiveArray<NiPointer<NiTimeController>>* apArray);

	NiTLargePrimitiveArray<NiPointer<NiTimeController>> pControllerA;

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable bhkExtraData_vftable{};
// pControllerA's own vtable (NiTLargePrimitiveArray<NiPointer<NiTimeController>>), and the vtable of its
// NiTLargeArray base, which the destructor resets it to.
inline constexpr fnvd::Vtable bhkExtraData_controllers_vftable{};
inline constexpr fnvd::Vtable bhkExtraData_controllerArray_vftable{};
