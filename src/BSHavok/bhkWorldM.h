#pragma once

#include "BSHavok/bhkWorld.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class NiPoint3;

// The Havok world of the exterior: a bhkWorld that spans several cells.
class bhkWorldM : public bhkWorld {
public:
	// Builds the world from arInfo with the given extents.
	bhkWorldM(const bhkWorldCinfo& arInfo, hkVector4* arWorldSize, hkVector4* arBorderSize);

	const NiRTTI* GetRTTI() const override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// Pulls arCenter back by the amount the broad phase would reach past the world, on each axis, toward the origin.
	void CalcNewCenter(hkVector4& arCenter);
	// Converts arPos to Havok units and returns whether CheckForMove moved the world.
	bool CheckForMove(const NiPoint3& arPos);
	// Moves the world's origin when arPos is at least the border size from it on some axis and the new centre differs
	// from it by more than 0.5 on some axis; true when it moved.
	bool CheckForMove(const hkVector4& arPos);

	// The total extent of the world, the border around it and the broad phase's extent, in Havok units.
	hkVector4 WorldTotalSize;
	hkVector4 BorderSize;
	hkVector4 BroadPhaseSize;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(bhkWorldM);
