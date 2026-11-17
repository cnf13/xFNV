#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void cleanupLoadedObjecthkCustomAttributesAttribute(void* p);
static void cleanupLoadedObjecthkCustomAttributes(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkCustomAttributesAttributeTypeInfo = {"hkCustomAttributesAttribute", nullptr,
	cleanupLoadedObjecthkCustomAttributesAttribute, nullptr, &typeid(hkCustomAttributes::Attribute),
	sizeof(hkCustomAttributes::Attribute)};
hkTypeInfo hkCustomAttributesTypeInfo = {"hkCustomAttributes", nullptr, cleanupLoadedObjecthkCustomAttributes, nullptr,
	&typeid(hkCustomAttributes), sizeof(hkCustomAttributes)};

// hkCustomAttributesAttributeClass's member table (.rdata). Not named in the PDB or IDA; name chosen here (the Havok
// SDK's file-local table).
using hkCustomAttributes_AttributeClass_MemberArray = const hkClassMember[2];
static hkCustomAttributes_AttributeClass_MemberArray hkCustomAttributes_AttributeClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"value", nullptr, nullptr, {hkClassMember::TYPE_VARIANT}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr}};
hkCustomAttributes::MemberArray hkCustomAttributes::Members{{"attributes", &hkCustomAttributesAttributeClass, nullptr,
	{hkClassMember::TYPE_SIMPLEARRAY}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x0, nullptr}};
hkClass hkCustomAttributesAttributeClass("hkCustomAttributesAttribute", nullptr, 12, nullptr, 0, nullptr, 0,
	hkCustomAttributes_AttributeClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkCustomAttributesClass("hkCustomAttributes", nullptr, 8, nullptr, 0, nullptr, 0, hkCustomAttributes::Members,
	1, nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkCustomAttributesAttribute(void* p) {
	;
}

static void cleanupLoadedObjecthkCustomAttributes(void* p) {
	;
}

const hkClass& hkCustomAttributes::Attribute::staticClass() {
	return hkCustomAttributesAttributeClass;
}

const hkClass& hkCustomAttributes::staticClass() {
	return hkCustomAttributesClass;
}
