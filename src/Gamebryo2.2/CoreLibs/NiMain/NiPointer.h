#pragma once

#include "Fallout Shared/BSCloneReserver.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class ActorPathingMessageQueue;
class BSMultiBoundNode;
class BSMultiBoundShape;
class BSShaderAccumulator;
class NavMesh;
class NiAVObject;
class NiCamera;
class NiD3DPass;
class NiD3DTextureStage;
class NiGeometry;
class NiGeometryData;
class NiLines;
class NiMaterialProperty;
class NiNode;
class NiObject;
class NiPixelData;
class NiScreenElements;
class NiSkinPartition;
class NiTriBasedGeom;
class NiTriShape;
class NiTriShapeData;
class NiTriStripsData;
class PathingAvoidNodeArray;
class PathingLOSGridMap;
class PathingRequest;
class PathingSolution;
class QueuedFile;
class TESAnimGroup;
class bhkRigidBody;
class bhkShape;

// A smart pointer that holds a reference on a NiRefObject-derived object: constructing, copying and assigning take
// one, destroying releases it, as the engine's inlined smart-pointer code does. Assignment from a raw pointer is the
// engine's 0x66B0D0, shared by every instantiation.
// MSVC passes a by-value NiPointer in place and the callee destroys it, so a by-value parameter is released by the
// callee's destructor and a caller's copy takes the reference that release consumes.
template <class T> class NiPointer {
public:
	constexpr NiPointer() : m_pObject(nullptr) {}
	NiPointer(T* apObject);
	NiPointer(const NiPointer& ptr);
	~NiPointer();

	operator T*() const { return m_pObject; }

	// Holds a reference on apObject and releases the old one (the exe's 0xA5F860 inlines operator=); returns this.
	NiPointer* SetNew(T* apObject) {
		*this = apObject;
		return this;
	}

	// Copies apSource's object and takes a reference on it, without releasing a previous one.
	NiPointer* SetPointer(const NiPointer* apSource) {
		m_pObject = apSource->m_pObject;
		if (m_pObject) {
			NiIncRefCount(m_pObject);
		}
		return this;
	}

	// IDA name; the PDB names these copies NiPointer<T>::operator=, which src defines inline below. An instance's
	// own out-of-line assignment, its reference counting inlined: holds a reference on apPointer's object and
	// releases the old one; returns this (thiscall, ret 4). Defined per instantiation, as a forwarder to that
	// instance's copy: NiRefObject's 0xC46270 (BSMain/NiObjectGarbage.cpp), BSTask<int64_t>'s 0xC41050
	// (BSMain/BSTaskManager.cpp).
	NiPointer* SetPointer_(const NiPointer* apPointer);

	// Assigns apOther to this pointer unless apOther holds no object.
	void SetIfNotNull(const NiPointer* apOther) {
		if (!(*apOther == nullptr)) {
			*this = *apOther;
		}
	}

	// Holds a reference on apObject and releases the old one.
	NiPointer& operator=(T* apObject);

	// Holds a reference on the other pointer's object and releases the old one. The other pointer is read again
	// after the release, as the engine's 0xC663D0 (shared by every instantiation) does.
	NiPointer& operator=(const NiPointer& ptr) {
		if (m_pObject != ptr.m_pObject) {
			if (m_pObject) {
				NiDecRefCount(m_pObject);
			}
			m_pObject = ptr.m_pObject;
			if (m_pObject) {
				NiIncRefCount(m_pObject);
			}
		}
		return *this;
	}

	T* operator->() const;

	bool operator==(T* pObject) const;
	bool operator==(const NiPointer& ptr) const;
	bool operator!=(T* pObject) const;
	bool operator!=(const NiPointer& ptr) const;
	void Free();

	// The native side of Destroy: releases the object as NiPointer<Model>::Destroy does, and nulls the
	// pointer, since the native local's own destructor runs again at the end of its scope.
	static void Destroy(NiPointer* apMemory) {
		if (apMemory->m_pObject) {
			apMemory->m_pObject->DecRefCount();
			apMemory->m_pObject = nullptr;
		}
	}

	T* m_pObject;
};

// IDA name; not in the PDB. A third `default constructor closure', one body the exe shares
// among the NiPointer instances whose arrays it builds (NiTexture in BSShaderBloodSplatterProperty, bhkPhantom in
// LoadedAreaBound, AnimIdle in AnimData...): the folded constructor 0x633C90 with a null pointer (thiscall, ret 0,
// 0x6694E0). One function for every instance, so apThis is typed as IDA's NiRefObjectPtr.
void NiPointerDefaultConstructor(NiPointer<NiRefObject>* apThis);
// Not named in the PDB or IDA; name chosen here. A second exe copy of the default constructor
// closure (src's NiPointer<NiRefObject> one is 0xC46230), compiled with the reference test dropped: stores a null
// pointer (thiscall, ret 0, 0xB54620; IDA ??0?$NiPointer@VNiRefObject@@@@). BSShaderManager::Initialize (0xB577A6),
// BSRenderedTexture's and BSShaderBloodSplatter's constructors (0xB6B3B5, 0xB80D64), the shader constant map
// entries' Allocate (0xB79AC4) and NiControllerSequence::LoadBinary (0xA3437C) hand it to
// `eh vector constructor iterator'.
void NiPointer_DefaultConstructorClosure_B54620(NiPointer<NiRefObject>* apThis);
// Not named in the PDB (IDA: `NiPointer<NiRefObject>::~NiPointer<NiRefObject>`); name chosen here. An exe copy of
// NiPointer<NiRefObject>'s destructor with the release inlined: InterlockedDecrement of the count at +4, DeleteThis
// (slot 1) at zero (thiscall, ret 0, 0xAE9290). The exe's EH unwind funclets call it;
// NiTNewInterface<NiPointer<NiRefObject>>::Allocate and NiPointer<NiRefObject>'s vector deleting destructor hand it
// to the `eh vector' iterators (as NiPointer_NiRefObject_Destruct_original). NiPointer<NiRefObject>::Destruct is the
// 0x45CEC0 fold, which the exe's other code calls.
void NiPointer_Destruct_AE9290(NiPointer<NiRefObject>* apThis);
// Not named in the PDB or IDA; name chosen here. Another exe copy of that destructor, the same code (thiscall,
// ret 0, 0xB66F20). The exe's EH unwind funclets, NiDevImageConverter and TallGrassShaderProperty::GetCachedGeometry
// run it; BSRenderedTexture's and BSShaderBloodSplatter's constructors (0xB6B3B0, 0xB80D84) hand it to
// `eh vector constructor iterator'.
void NiPointer_Destruct_B66F20(NiPointer<NiRefObject>* apThis);
// Not named in the PDB or IDA; name chosen here. A second exe copy of
// NiPointer<ActorPathingMessageQueue>'s constructor (src's Construct is the folded 0x633C90): holds apObject through
// NiRefObject::IncRefCount 0x40F6E0 on its NiRefObject base at +0x14; returns apThis (thiscall, ret 4, 0x6EBF60; the
// PDB's NiPointer(pObject) in PathManager.cpp). PathingTaskData, HighProcess, CombatState and ActorMover's
// constructors call it.
NiPointer<ActorPathingMessageQueue>* NiPointer_Construct_6EBF60(NiPointer<ActorPathingMessageQueue>* apThis,
	ActorPathingMessageQueue* apObject);

// Declares classname and its smart pointer type classname##Ptr, as Gamebryo's NiSmartPointer.h does.
#define NiSmartPointer(classname) \
	class classname; \
	typedef NiPointer<classname> classname##Ptr

// NiRefObject's smart pointer is declared here: NiRefObject.h is included by this header, so it cannot use the macro.
NiSmartPointer(NiRefObject);

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.inl"
