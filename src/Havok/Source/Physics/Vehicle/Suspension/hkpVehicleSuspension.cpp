#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkpVehicleSuspensionSuspensionWheelParameters(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleSuspensionSuspensionWheelParametersTypeInfo = {"hkpVehicleSuspensionSuspensionWheelParameters",
	nullptr, cleanupLoadedObjecthkpVehicleSuspensionSuspensionWheelParameters, nullptr,
	&typeid(hkpVehicleSuspension::SuspensionWheelParameters), sizeof(hkpVehicleSuspension::SuspensionWheelParameters)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVehicleSuspension::SuspensionWheelParameters, as the exe's .rdata holds them.
using hkpVehicleSuspensionSuspensionWheelParametersClass_MembersArray = const hkClassMember[3];
static hkpVehicleSuspensionSuspensionWheelParametersClass_MembersArray
	hkpVehicleSuspensionSuspensionWheelParametersClass_Members{{"hardpointChassisSpace", nullptr, nullptr,
																   {hkClassMember::TYPE_VECTOR4},
																   {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
		{"directionChassisSpace", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0},
			0x10, nullptr},
		{"length", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}};
hkClass hkpVehicleSuspensionSuspensionWheelParametersClass("hkpVehicleSuspensionSuspensionWheelParameters", nullptr, 48,
	nullptr, 0, nullptr, 0, hkpVehicleSuspensionSuspensionWheelParametersClass_Members, 3, nullptr, nullptr, 0, 0);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVehicleSuspension, as the exe's .rdata holds them.
using hkpVehicleSuspensionClass_MembersArray = const hkClassMember[1];
static hkpVehicleSuspensionClass_MembersArray hkpVehicleSuspensionClass_Members{
	{"wheelParams", &hkpVehicleSuspensionSuspensionWheelParametersClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr}};
hkClass hkpVehicleSuspensionClass("hkpVehicleSuspension", &hkReferencedObjectClass, 20, nullptr, 0, nullptr, 0,
	hkpVehicleSuspensionClass_Members, 1, nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkpVehicleSuspensionSuspensionWheelParameters(void* p) {
	;
}

const hkClass& hkpVehicleSuspension::SuspensionWheelParameters::staticClass() {
	return hkpVehicleSuspensionSuspensionWheelParametersClass;
}
