#pragma once

#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class NiPoint3;

// A volume (room, portal region) that scene graph nodes are bound to.
class BSMultiBound : public NiObject {
public:
	BSMultiBound();
	// Declared only: the exe's destructor bodies are the Destruct/DeletingDestruct statics below. Goes at parity.
	~BSMultiBound() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	virtual bool QPointWithin(NiPoint3& aPoint);
	// the shared empty stub of many unrelated classes
	virtual void SetCenter(const NiPoint3&) {}

	void SetShape(BSMultiBoundShape* apShape);
	// spShape; the exe's body is NiObjectNET::GetControllers' (0x43B230), a shared one-field
	// getter (thiscall).
	BSMultiBoundShape* QShape() const;

protected:
	void CopyMembers(BSMultiBound* pDest, NiCloningProcess& arCloneData);

public:
	static NiObject* CreateObject();

	uint32_t uiBoundFrameCount;
	NiPointer<BSMultiBoundShape> spShape;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSMultiBound);

inline constexpr fnvd::Vtable BSMultiBound_vftable{};
