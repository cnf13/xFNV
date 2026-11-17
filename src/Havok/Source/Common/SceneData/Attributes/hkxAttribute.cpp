#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxAttributeTypeInfo = {"hkxAttribute", finishLoadedObjecthkxAttribute, cleanupLoadedObjecthkxAttribute,
	nullptr, &typeid(hkxAttribute), sizeof(hkxAttribute)};

// The class record's tables: the items of
// the Hint enum, the enum table and the member table.
using hkxAttributeHintEnumItemArray = const hkClassEnum::Item[6];
static hkxAttributeHintEnumItemArray hkxAttributeHintEnumItems{{0, "HINT_NONE"}, {1, "HINT_IGNORE"},
	{2, "HINT_TRANSFORM"}, {4, "HINT_SCALE"}, {6, "HINT_TRANSFORM_AND_SCALE"}, {8, "HINT_FLIP"}};
using hkxAttributeClassEnumArray = const hkClassEnum[1];
static hkxAttributeClassEnumArray hkxAttributeEnums{{"Hint", hkxAttributeHintEnumItems, 6, nullptr, 0}};
using hkxAttributeClassMemberArray = const hkClassMember[2];
static hkxAttributeClassMemberArray hkxAttributeClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"value", &hkReferencedObjectClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x4,
		nullptr}};
// The PDB's name: the enum of hkxAttribute's Hint, by pointer (.data); the animated attributes' member tables read it.
const hkClassEnum* hkxAttributeHintEnum = &hkxAttributeEnums[0];
hkClass hkxAttributeClass("hkxAttribute", nullptr, 8, nullptr, 0, hkxAttributeEnums, 1, hkxAttributeClass_Members, 2,
	nullptr, nullptr, 0, 0);

hkxAttribute::hkxAttribute() : m_name(), m_value() {}

void hkxAttribute::setValue(hkReferencedObject* obj, const hkClass* klass) {
	m_value.set(obj, klass);
}

const hkClass& hkxAttribute::staticClass() {
	return hkxAttributeClass;
}
