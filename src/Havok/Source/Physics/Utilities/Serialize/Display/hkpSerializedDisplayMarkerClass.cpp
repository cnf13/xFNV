#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarker.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflection data of hkpSerializedDisplayMarker (the PDB's hkpSerializedDisplayMarkerClass.cpp).

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpSerializedDisplayMarker, as the exe's .rdata holds them.
using hkpSerializedDisplayMarkerClass_MembersArray = const hkClassMember[1];
static hkpSerializedDisplayMarkerClass_MembersArray hkpSerializedDisplayMarkerClass_Members{{"transform", nullptr,
	nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr}};

hkTypeInfo hkpSerializedDisplayMarkerTypeInfo = {"hkpSerializedDisplayMarker",
	finishLoadedObjecthkpSerializedDisplayMarker, cleanupLoadedObjecthkpSerializedDisplayMarker,
	getVtablehkpSerializedDisplayMarker(), &typeid(hkpSerializedDisplayMarker), sizeof(hkpSerializedDisplayMarker)};

hkClass hkpSerializedDisplayMarkerClass("hkpSerializedDisplayMarker", &hkReferencedObjectClass, 80, nullptr, 0, nullptr,
	0, hkpSerializedDisplayMarkerClass_Members, 1, nullptr, nullptr, 0, 0);

void cleanupLoadedObjecthkpSerializedDisplayMarker(void* p) {
	static_cast<hkpSerializedDisplayMarker*>(p)->~hkpSerializedDisplayMarker();
}
