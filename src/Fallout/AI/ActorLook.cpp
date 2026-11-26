#include "Fallout/AI/ActorLook.h"

#include "BSHavok/bhkMouseSpringAction.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

SettingT<INISettingCollection> ActorLook::bActorLookWithHavok("bActorLookWithHavok:General", false);
