#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// Not named in the PDB or IDA; the SDK class file's name. hkpArrayAction's one member, its entity pointer array
// (.rdata).
using hkpArrayActionClass_MembersArray = const hkClassMember[1];
static hkpArrayActionClass_MembersArray hkpArrayActionClass_Members{{"entities", &hkpEntityClass, nullptr,
	{hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0}, 0x18, nullptr}};

// The reflection class of hkpArrayAction (the SDK's class file).
hkClass hkpArrayActionClass("hkpArrayAction", &hkpActionClass, 36, nullptr, 0, nullptr, 0, hkpArrayActionClass_Members,
	1, nullptr, nullptr, 0, 0);
