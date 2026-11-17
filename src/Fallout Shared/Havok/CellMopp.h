#pragma once

#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/hkBSHeightFieldShape.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

#include <stdint.h>

class bhkWorld;
class bhkWorldM;
class NiPoint3;
class NiQuaternion;

// A cell's MOPP collision, held by its ExtraHavok: the rigid bodies it created.
class CellMopp : public NiRefObject {
public:
	CellMopp();
	~CellMopp() override;

	// Removes every body from the world.
	void Remove();
	// The Havok world the MOPP was built in.
	bhkWorldM* GetParent();
	// Builds the MOPP into apWorld at arPosition/arRotation; false when there is no world or nothing was created.
	bool Make(bhkWorld* apWorld, const NiPoint3& arPosition, const NiQuaternion& arRotation);
	bool QCreated();
	// Removes the bodies from the world and empties the body array.
	void Kill();
	NiTObjectArray<NiPointer<bhkRigidBody>>& GetRigidBodyA();
	// Builds a fixed ground rigid body with a height-field shape for each strip of appStripArray, offset by afOffset;
	// true unless the bodies exist already.
	bool Create(NiTriStrips** appStripArray, uint32_t aiCount, float afOffset);

	NiTObjectArray<NiPointer<bhkRigidBody>> spRigidBodyA;

	static inline int32_t iCount;
};

NiSmartPointer(CellMopp);

using bhkRigidBodyArray = NiTArray<NiPointer<bhkRigidBody>, NiTNewInterface<NiPointer<bhkRigidBody>>>;

// The construction data of the terrain's triangle-sampled height field shape: hkBvTreeShapeCinfo's user data and
// shape collection, then the 16-aligned hkBSHeightFieldShapeCinfo base at 0x10.
class hkTriSampledHeightFieldBvTreeShapeCinfo : public hkBvTreeShapeCinfo, public hkBSHeightFieldShapeCinfo {
public:
	hkTriSampledHeightFieldBvTreeShapeCinfo();
};

// The game settings CellMopp.cpp defines.
extern SettingT<INISettingCollection> fLandFriction;
