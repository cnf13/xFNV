#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiNode;
class Sky;

// Base of the sky objects (Atmosphere, Clouds, Stars, Sun): holds the sky scene root they attach to.
class SkyObject {
public:
	SkyObject();
	virtual ~SkyObject();

	virtual NiNode* GetRoot() { return spRoot; }

	// The engine's SkyObject::Initialize has SEH and creates the sky root node.
	virtual void Initialize(NiNode* apSkyRoot);

	virtual void Update(Sky*, float) {}

	// The exe's deleting destructor frees with ourFree (0x63F8A0).
	static void operator delete(void* apMemory);

	// The root set up by Initialize. Public because the derived classes read it. PDB NiPointer<NiNode>; the exe reads
	// it through NiPointer's operator-> (0x559450 from Atmosphere::Initialize, 0x633260).
	NiPointer<NiNode> spRoot;
};
inline constexpr fnvd::Vtable SkyObject_vftable{};
