#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarker.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarkerList.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpSerializedDisplayMarkerList, as the exe's .rdata holds them.
using hkpSerializedDisplayMarkerListClass_MembersArray = const hkClassMember[1];
static hkpSerializedDisplayMarkerListClass_MembersArray hkpSerializedDisplayMarkerListClass_Members{
	{"markers", &hkpSerializedDisplayMarkerClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER},
		0, 0, 0x8, nullptr}};

hkClass hkpSerializedDisplayMarkerListClass("hkpSerializedDisplayMarkerList", &hkReferencedObjectClass, 20, nullptr, 0,
	nullptr, 0, hkpSerializedDisplayMarkerListClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpSerializedDisplayMarkerListTypeInfo = {"hkpSerializedDisplayMarkerList",
	finishLoadedObjecthkpSerializedDisplayMarkerList, cleanupLoadedObjecthkpSerializedDisplayMarkerList,
	getVtablehkpSerializedDisplayMarkerList(), &typeid(hkpSerializedDisplayMarkerList),
	sizeof(hkpSerializedDisplayMarkerList)};

// The SDK constructs an hkpSerializedDisplayMarkerList on the stack with the finish-loading constructor and returns
// its first word; the exe inlines it and folds it to `mov eax, 0x10E66FC; ret` (the exe's vtable).
const void* getVtablehkpSerializedDisplayMarkerList() {
	return hkGetVtableOf<hkpSerializedDisplayMarkerList>();
}

void cleanupLoadedObjecthkpSerializedDisplayMarkerList(void* p) {
	static_cast<hkpSerializedDisplayMarkerList*>(p)->~hkpSerializedDisplayMarkerList();
}
