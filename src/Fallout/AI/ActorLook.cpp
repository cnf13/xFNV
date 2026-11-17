#include "Fallout/AI/ActorLook.h"

#include "BSHavok/bhkMouseSpringAction.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

SettingT<INISettingCollection> ActorLook::bActorLookWithHavok("bActorLookWithHavok:General", false);

// Converts the game-unit point to Havok units in a local (NI2HK, which writes all four lanes) and moves the spring's
// target there through the hkVector4 overload.
void bhkMouseSpringAction::SetMousePosition(const NiPoint3& arPos) {
	hkVector4 hkp;
	SetMousePosition(NI2HK(hkp, arPos));
}

// Sets the wrapped action's target to arPos when there is a wrapped action.
void bhkMouseSpringAction::SetMousePosition(const hkVector4& arPos) {
	hkpMouseSpringAction* pm = operator*();
	if (pm)
		pm->setMousePosition(arPos);
}
