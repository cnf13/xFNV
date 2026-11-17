#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class TESForm;
class TESObjectCELL;
class TESWorldSpace;

// A point in a space: an interior cell or a world space.
class BGSWorldLocation {
public:
	BGSWorldLocation();
	BGSWorldLocation(const NiPoint3& arPoint, TESForm* apSpace);

	// The cell of the location.
	TESObjectCELL* GetCell() const;
	// Writes the position and the space's form ID.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	// IDA names it BGSLoadGameBuffer::LoadBGSWorldLocation; not in the PDB.
	// Reads the position and the space, and clears the space when it is not a cell or world
	// space (thiscall, ret 4, 0x527BA0). No caller reads eax after it.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// The squared distance to arWorldLocation (thiscall, ret 4, st0 result, 0x527A40).
	float GetDistanceSquared(const BGSWorldLocation& arWorldLocation) const;
	bool IsLocationLoaded() const;
	BGSWorldLocation* Clear();
	void SetLocation(const NiPoint3& arPoint, TESForm* apSpace);
	void SetPoint(const NiPoint3& arPoint);
	// The location's space, pSpace; the exe's shared one-field getter 0x84E3A0 (thiscall, ret
	// 0).
	TESForm* GetSpace();
	// Point; the exe's shared identity getter 0x6815C0 (thiscall, ret 0; registered as
	// NiNode::IsNode).
	NiPoint3* GetPoint();
	// Stores apSpace in pSpace.
	void SetSpace(TESForm* apSpace) { pSpace = apSpace; }
	float GetDistance(const BGSWorldLocation& arWorldLocation) const;
	float GetModifiedDistanceSquared(const BGSWorldLocation& arWorldLocation, const NiPoint3& arCoordMult) const;
	bool GetCellOrWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld);

	static char CheckSpace(TESObjectCELL* a1);

	NiPoint3 Point;
	// An interior cell or a world space: the exe tests the form's type and treats it as one or the other.
	TESForm* pSpace;
};
