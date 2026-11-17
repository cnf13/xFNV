#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkContactPoint(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkContactPointTypeInfo = {"hkContactPoint", nullptr, cleanupLoadedObjecthkContactPoint, nullptr,
	&typeid(hkContactPoint), sizeof(hkContactPoint)};
hkContactPoint::MembersArray hkContactPoint::Members{{"position", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4},
														 {hkClassMember::TYPE_VOID}, 0, {0},
														 offsetof(hkContactPoint, m_position), nullptr},
	{"separatingNormal", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkContactPoint, m_separatingNormal), nullptr}};
hkClass hkContactPointClass("hkContactPoint", nullptr, sizeof(hkContactPoint), nullptr, 0, nullptr, 0,
	hkContactPoint::Members, 2, nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkContactPoint(void* p) {
	;
}

// The reflection data of hkContactPoint (the PDB's hkContactPointClass.obj; the SDK has no .cpp here to name the file,
// so it takes the header's stem).
