#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiCamera;
class NiGeometry;
class NiVisibleArray;

// Base of the classes that collect geometry during a scene traversal and draw it afterwards.
class NiAccumulator : public NiObject {
public:
	NiAccumulator();
	~NiAccumulator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	// the stream overrides fold into jumps to the NiObject bodies
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	virtual void StartAccumulating(const NiCamera* pkCamera);
	// Out of line: the exe calls it from the derived FinishAccumulating bodies (0xAA0FE0).
	virtual void FinishAccumulating();
	virtual void RegisterObjectArray(NiVisibleArray& arArray) = 0;
	// shared "return false", "return true" and empty stubs
	virtual bool RegisterObject(NiGeometry*) { return false; }
	virtual bool AddShared(NiAVObject*) { return true; }
	virtual void ClearSharedMap() {}

	bool IsAccumulating() const;
	// A jmp to NiObject::CopyMembers (thiscall, ret 8, 0xAA1000), which the exe folds with
	// NiAVObjectPalette's, NiCollisionObject's and NiInterpolator's CopyMembers: their derived classes' CopyMembers
	// and CreateClone call it.
	void CopyMembers(NiAccumulator* pkDest, NiCloningProcess& kCloning);

	NiCamera* m_pkCamera;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiAccumulator_vftable{};
