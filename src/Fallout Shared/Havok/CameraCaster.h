#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiPoint3;
class bhkCharacterController;
class bhkSimpleShapePhantom;
class bhkWorld;
struct hkpRootCdPoint;

class CameraCaster {
public:
	CameraCaster(float afRadius, uint32_t aiGroup);
	~CameraCaster();

	// Adds the camera casting to apWorld.
	void Add(bhkWorld* apWorld);
	void SetPlayerGroup(uint32_t aiGroup);
	// spPlayerShapePhantom, through the shared getter of the field
	// at +0.
	bhkSimpleShapePhantom* GetPlayerShapePhantom() { return spPlayerShapePhantom.m_pObject; }
	// Casts the player phantom from arFrom to arTo; on a hit that counts, copies it to arPoint and its position to
	// arTo.
	bool Pick(NiPoint3& arFrom, NiPoint3& arTo, hkpRootCdPoint& arPoint);
	void Remove();
	// The Havok world the phantom is in.
	bhkWorld* GetWorld();
	// Whether the camera at arPoint is inside apController's character: with a chase biped, whether it is closer
	// than fCameraCasterFadeSittingRadius to the player's head; otherwise whether a phantom sphere of afRadius
	// there touches the character's phantom, both lowered by afScaleOffset for the player when the camera is above
	// the capsule.
	bool KeepOutOfCharacter(bhkCharacterController* apController, NiPoint3& arPoint, float afRadius,
		float afScaleOffset);
	// Builds the player and horse phantoms: camera-pick spheres of afRadius, the player's in group aiGroup, the
	// horse's in a new system group.
	void Init(float afRadius, uint32_t aiGroup);

	// The exe's deleting destructor (0x93A5A0) frees through ourFree.
	static void operator delete(void* apMemory);

	// The exe constructs and destroys it as a NiPointer (0x620850, 0x6208D0) and assigns it through 0x66B0D0.
	NiPointer<bhkSimpleShapePhantom> spPlayerShapePhantom;
	NiPointer<bhkSimpleShapePhantom> spHorseShapePhantom;
};

// The game settings CameraCaster.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fCameraCasterFadeSittingRadius;
