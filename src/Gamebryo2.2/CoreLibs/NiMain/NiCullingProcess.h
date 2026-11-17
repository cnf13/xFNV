#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiAVObject;
class NiCamera;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class NiVisibleArray;
class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;

// Walks a scene graph and collects the objects visible to a camera.
class NiCullingProcess {
public:
	// Constructs a culling process in apMemory that appends to pkVisibleSet (the exe's
	// constructor, thiscall, ret 4, 0xA69400).
	// The hybrid's implicit default constructor (no zero-parameter Construct).
	NiCullingProcess() = default;
	NiCullingProcess(NiVisibleArray* pkVisibleSet);

	// the engine's root of the culling process classes: it repeats NiObject's type queries ahead of its destructor
	virtual const NiRTTI* GetRTTI() const { return &ms_RTTI; }
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
	virtual bhkRigidBody* IsBhkRigidBody() { return nullptr; }
	virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() { return nullptr; }
	virtual ~NiCullingProcess();
	// MSVC lays a virtual overload set out in reverse declaration order: the scene overload is declared first and
	// takes the later slot
	// culls the whole scene against the camera into the visible set
	virtual void Process(const NiCamera* pkCamera, NiAVObject* pkScene, NiVisibleArray* pkVisibleSet);
	// culls one object of the scene graph
	virtual void Process(NiAVObject* pkObject);
	// adds a visible geometry to the visible set
	virtual void AppendVirtual(NiGeometry& kVisible);

	const NiCamera* GetCamera() const;
	const NiFrustumPlanes& GetFrustumPlanes() const;
	// The visible set, m_pkVisibleSet. Same function as TESForm::GetFormFlags (0x44DDC0), registered there.
	NiVisibleArray* GetVisibleSet() { return m_pkVisibleSet; }
	void SetCamera(NiCamera* apCamera);
	// Takes the frustum and rebuilds the world-space planes from the camera's transform, all planes active; does
	// nothing without a camera.
	void SetFrustum(const NiFrustum& kFrustum);
	// Tests the object's world bound against the active planes: planes it lies wholly in front of are switched off
	// for its subtree, and the object is visited unless one plane rejects it. A zero-radius bound is skipped.
	void DoCulling(NiAVObject* apObject);

	// Freed through MemoryManager::Deallocate (the exe's deleting destructor 0xA69470).
	static void operator delete(void* apMemory);

	bool m_bUseVirtualAppend;
	uint8_t padding05[3];
	NiVisibleArray* m_pkVisibleSet;
	NiCamera* m_pkCamera;
	NiFrustum m_kFrustum;

	NiFrustumPlanes m_kPlanes;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiCullingProcess_vftable{};
