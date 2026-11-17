#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

class NiPoint3;
class hkpRigidBody;
class hkpShape;

// The Gamebryo side of a Havok object: a scene graph object that owns the reference to the Havok object it wraps.
class bhkRefObject : public NiObject {
public:
	bhkRefObject();
	~bhkRefObject() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// Replaces the wrapped Havok object, detaching the old one from the wrapper and attaching the new one.
	virtual void SethkObject(hkReferencedObject* aphkObject);

	// Attaches the wrapped Havok object to the wrapper (true) or detaches it (false).
	virtual void Link(bool abAttach);

	// The wrapped body's hkpMotion::MotionType (thiscall, ret 0, 0x517630).
	// Inferred type: return: int32_t: signed uses from 0x9b006d
	int32_t GetMotionType();
	// IDA name GetLinearVelocityNi; not in the PDB. The wrapped body's linear velocity in
	// Gamebryo units, stored in apVelocity (thiscall, ret 4, 0x560D50).
	void* GetLinearVelocityNi(NiPoint3* apVelocity);
	// IDA name; not in the PDB. Returns nothing: its one caller, SetRBMass (0x5D3CA8), reads no eax after it, and
	// on the non-null path eax is what hkpRigidBody::setMass's tail call left.
	// Inferred type: a2: float from 0x5d3cfe (passed to hkpRigidBody::setMass 0)
	void SetMass(float a2);

	// The wrapped Havok object.
	hkReferencedObject* GethkObject() const;
	// The wrapped Havok object, or null for a null wrapper.
	hkReferencedObject* operator*() const;

	hkReferencedObject* phkObject;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable bhkRefObject_vftable{};
