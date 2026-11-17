#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class bhkCharacterController;
class bhkSimpleShapePhantom;
class bhkWorld;
struct hkpRootCdPoint;
class NiPoint3;

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

	// Deleting frees through ourFree.
	static void operator delete(void* apMemory);

	NiPointer<bhkSimpleShapePhantom> spPlayerShapePhantom;
	NiPointer<bhkSimpleShapePhantom> spHorseShapePhantom;
};

extern SettingT<INISettingCollection> fCameraCasterFadeSittingRadius;
