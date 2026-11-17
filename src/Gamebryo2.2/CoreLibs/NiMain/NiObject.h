#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

class BSFadeNode;
class BSMultiBoundNode;
class BSNiBound;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiBound;
class NiCloningProcess;
class NiControllerManager;
class NiGeometry;
class NiLines;
class NiNode;
class NiObjectGroup;
class NiParticles;
class NiStream;
template <class T> class NiTPrimitiveArray;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;

NiSmartPointer(NiObject);

// Adds run-time type information, cloning and streaming to NiRefObject (all virtual, no fields).
class NiObject : public NiRefObject {
public:
	NiObject();
	~NiObject() override;

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
	virtual NiObject* CreateClone(NiCloningProcess&) { return this; }
	virtual void LoadBinary(NiStream& stream);
	// Out of line: the exe calls it from the derived LinkObject bodies (0xA89AF0).
	virtual void LinkObject(NiStream& stream);
	virtual bool RegisterStreamables(NiStream& stream);
	virtual void SaveBinary(NiStream&) {}
	virtual bool IsEqual(NiObject* pObject);
	virtual void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings);
	virtual void AddViewerStrings(NiTPrimitiveArray<char*>*) {}
	virtual void ProcessClone(NiCloningProcess& kCloning);
	virtual void PostLinkObject(NiStream&) {}
	virtual bool StreamCanSkip() { return false; }
	virtual const NiRTTI* GetStreamableRTTI() const;
	virtual void SetBound(BSNiBound*) {}
	virtual uint32_t GetBlockAllocationSize() const { return 0; }
	virtual NiObjectGroup* GetGroup() const { return nullptr; }
	virtual void SetGroup(NiObjectGroup*) {}
	virtual NiControllerManager* IsNiControllerManager() { return nullptr; }

	// Records apDest as this object's clone in the cloning process's clone map (used by CreateClone).
	void CopyMembers(NiObject* pDest, NiCloningProcess& kCloning);
	NiObject* CreateSharedClone(NiCloningProcess& kCloning);
	NiObject* Clone();
	// Stores a deep copy of this object in aspObject.
	void CreateDeepCopy(NiPointer<NiObject>& aspObject);
	// Clones through an existing cloning process.
	NiObject* Clone(NiCloningProcess& arCloning);

	// Type checks along the RTTI base chain. Inline at most call sites; the exe also keeps out-of-line copies
	// (IsKindOf 0x6532C0, static IsKindOf 0x43B300, DynamicCast 0x653290, static DynamicCast 0x653270).
	bool IsKindOf(const NiRTTI* pkRTTI) const {
		for (const NiRTTI* pRTTI = GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
			if (pRTTI == pkRTTI) {
				return true;
			}
		}
		return false;
	}

	// This object when it is a pkRTTI (or derived), otherwise null.
	NiObject* DynamicCast(const NiRTTI* pkRTTI) const {
		return IsKindOf(pkRTTI) ? const_cast<NiObject*>(this) : nullptr;
	}

	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's member
	// NiObject::DynamicCast(const NiRTTI*) const (thiscall, ret 4, 0x653290).
	NiObject* DynamicCastMember(const NiRTTI* pkRTTI) const;
	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's
	// NiObject::IsKindOf(const NiRTTI*) const (thiscall, ret 4, 0x6532C0; returns al).
	bool IsKindOfMember(const NiRTTI* pkRTTI) const;
	// Whether this object's RTTI is exactly pkRTTI (an out-of-line copy at 0x45BAF0).
	bool IsExactKindOf(const NiRTTI* pkRTTI) const;

	static bool IsKindOf(const NiRTTI* pkRTTI, const NiObject* pkObject) {
		return pkObject && pkObject->IsKindOf(pkRTTI);
	}

	static NiObject* DynamicCast(const NiRTTI* pkRTTI, const NiObject* pkObject) {
		return pkObject ? pkObject->DynamicCast(pkRTTI) : nullptr;
	}

	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's static
	// NiObject::DynamicCast(const NiRTTI*, const NiObject*) (cdecl, 0x653270); the inline overload above stays for
	// the sites the exe inlines it at.
	static NiObject* DynamicCastStatic(const NiRTTI* pkRTTI, const NiObject* pkObject);
	// cdecl, 0x45BAD0: false for a null object, else whether its RTTI is exactly pkRTTI.
	static bool IsExactKindOf(const NiRTTI* pkRTTI, const NiObject* pkObject);
	static bool IsKindOf_0(NiRTTI* a1, NiObject* a2);

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiObject_vftable{};
