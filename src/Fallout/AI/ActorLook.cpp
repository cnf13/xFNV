#include "Fallout/AI/ActorLook.h"

#include "BSHavok/bhkMouseSpringAction.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

SettingT<INISettingCollection> ActorLook::bActorLookWithHavok("bActorLookWithHavok:General", false);

// The PDB places both SetMousePosition overloads of bhkMouseSpringAction (header inlines) in this file.

// Converts the game-unit point to Havok units in a local (NI2HK, which writes all four lanes) and moves the spring's
// target there through the hkVector4 overload, called out of line. The exe realigns its frame to 16 for the local
// and checks a stack cookie (left out); its call of the local's constructor, 0x6815C0, is a trivial accessor whose
// result it drops, left out.
void bhkMouseSpringAction::SetMousePosition(const NiPoint3& arPos) {
	hkVector4 hkp;
	SetMousePosition(NI2HK(hkp, arPos));
}

// The wrapped action's target set to arPos when there is a wrapped action (operator*, called out of line).
void bhkMouseSpringAction::SetMousePosition(const hkVector4& arPos) {
	hkpMouseSpringAction* pm = operator*();
	if (pm)
		pm->setMousePosition(arPos);
}
