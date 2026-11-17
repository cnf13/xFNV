#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

// The attribute classes' reflection records: per class its enum items, enums and enum pointer, its member table and its
// hkClass. A member table that names an enum pointer is not constant: the pointer is no constant, so a dynamic
// initializer stores it into the member.

// The class record's member table.
using hkRangeRealAttributeClassMemberArray = const hkClassMember[4];
static hkRangeRealAttributeClassMemberArray hkRangeRealAttributeClass_Members{
	{"absmin", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"absmax", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
	{"softmin", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"softmax", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr}};
hkClass hkRangeRealAttributeClass("hkRangeRealAttribute", nullptr, 16, nullptr, 0, nullptr, 0,
	hkRangeRealAttributeClass_Members, 4, nullptr, nullptr, 0, 0);

// The class record's member table.
using hkRangeInt32AttributeClassMemberArray = const hkClassMember[4];
static hkRangeInt32AttributeClassMemberArray hkRangeInt32AttributeClass_Members{
	{"absmin", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"absmax", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
	{"softmin", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"softmax", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr}};
hkClass hkRangeInt32AttributeClass("hkRangeInt32Attribute", nullptr, 16, nullptr, 0, nullptr, 0,
	hkRangeInt32AttributeClass_Members, 4, nullptr, nullptr, 0, 0);

// The enum's items and the enum.
using hkUiAttributeHideInModelerEnumItemArray = const hkClassEnum::Item[3];
static hkUiAttributeHideInModelerEnumItemArray hkUiAttributeHideInModelerEnumItems{{hkUiAttribute::NONE, "NONE"},
	{hkUiAttribute::MAX, "MAX"}, {hkUiAttribute::MAYA, "MAYA"}};
using hkUiAttributeEnumArray = const hkClassEnum[1];
static hkUiAttributeEnumArray hkUiAttributeEnums{{"HideInModeler", hkUiAttributeHideInModelerEnumItems, 3, nullptr, 0}};
static const hkClassEnum* hkUiAttributeHideInModelerEnum = &hkUiAttributeEnums[0];
// The class record's member table.
using hkUiAttributeClassMemberArray = hkClassMember[8];
static hkUiAttributeClassMemberArray hkUiAttributeClass_Members{
	{"visible", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"hideInModeler", nullptr, hkUiAttributeHideInModelerEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8},
		0, 0, 0x1, nullptr},
	{"label", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
	{"group", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"hideBaseClassMembers", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC,
		nullptr},
	{"endGroup", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
	{"endGroup2", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x11, nullptr},
	{"advanced", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x12, nullptr}};
hkClass hkUiAttributeClass("hkUiAttribute", nullptr, 20, nullptr, 0, hkUiAttributeEnums, 1, hkUiAttributeClass_Members,
	8, nullptr, nullptr, 0, 2);

// The enum's items and the enum.
using hkGizmoAttributeGizmoTypeEnumItemArray = const hkClassEnum::Item[4];
static hkGizmoAttributeGizmoTypeEnumItemArray hkGizmoAttributeGizmoTypeEnumItems{{hkGizmoAttribute::POINT, "POINT"},
	{hkGizmoAttribute::SPHERE, "SPHERE"}, {hkGizmoAttribute::PLANE, "PLANE"}, {hkGizmoAttribute::ARROW, "ARROW"}};
using hkGizmoAttributeEnumArray = const hkClassEnum[1];
static hkGizmoAttributeEnumArray hkGizmoAttributeEnums{
	{"GizmoType", hkGizmoAttributeGizmoTypeEnumItems, 4, nullptr, 0}};
static const hkClassEnum* hkGizmoAttributeGizmoTypeEnum = &hkGizmoAttributeEnums[0];
// The class record's member table.
using hkGizmoAttributeClassMemberArray = hkClassMember[3];
static hkGizmoAttributeClassMemberArray hkGizmoAttributeClass_Members{
	{"visible", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"label", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
	{"type", nullptr, hkGizmoAttributeGizmoTypeEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, 0, 0x8,
		nullptr}};
hkClass hkGizmoAttributeClass("hkGizmoAttribute", nullptr, 12, nullptr, 0, hkGizmoAttributeEnums, 1,
	hkGizmoAttributeClass_Members, 3, nullptr, nullptr, 0, 0);

// The enum's items and the enum.
using hkModelerNodeTypeAttributeModelerTypeEnumItemArray = const hkClassEnum::Item[2];
static hkModelerNodeTypeAttributeModelerTypeEnumItemArray hkModelerNodeTypeAttributeModelerTypeEnumItems{
	{hkModelerNodeTypeAttribute::DEFAULT, "DEFAULT"}, {hkModelerNodeTypeAttribute::LOCATOR, "LOCATOR"}};
using hkModelerNodeTypeAttributeEnumArray = const hkClassEnum[1];
static hkModelerNodeTypeAttributeEnumArray hkModelerNodeTypeAttributeEnums{
	{"ModelerType", hkModelerNodeTypeAttributeModelerTypeEnumItems, 2, nullptr, 0}};
static const hkClassEnum* hkModelerNodeTypeAttributeModelerTypeEnum = &hkModelerNodeTypeAttributeEnums[0];
// The class record's member table.
using hkModelerNodeTypeAttributeClassMemberArray = hkClassMember[1];
static hkModelerNodeTypeAttributeClassMemberArray hkModelerNodeTypeAttributeClass_Members{
	{"type", nullptr, hkModelerNodeTypeAttributeModelerTypeEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8},
		0, 0, 0x0, nullptr}};
hkClass hkModelerNodeTypeAttributeClass("hkModelerNodeTypeAttribute", nullptr, 1, nullptr, 0,
	hkModelerNodeTypeAttributeEnums, 1, hkModelerNodeTypeAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

// The enum's items and the enum.
using hkLinkAttributeLinkEnumItemArray = const hkClassEnum::Item[5];
static hkLinkAttributeLinkEnumItemArray hkLinkAttributeLinkEnumItems{{hkLinkAttribute::NONE, "NONE"},
	{hkLinkAttribute::DIRECT_LINK, "DIRECT_LINK"}, {hkLinkAttribute::CHILD, "CHILD"}, {hkLinkAttribute::MESH, "MESH"},
	{hkLinkAttribute::PARENT_NAME, "PARENT_NAME"}};
using hkLinkAttributeEnumArray = const hkClassEnum[1];
static hkLinkAttributeEnumArray hkLinkAttributeEnums{{"Link", hkLinkAttributeLinkEnumItems, 5, nullptr, 0}};
static const hkClassEnum* hkLinkAttributeLinkEnum = &hkLinkAttributeEnums[0];
// The class record's member table.
using hkLinkAttributeClassMemberArray = hkClassMember[1];
static hkLinkAttributeClassMemberArray hkLinkAttributeClass_Members{{"type", nullptr, hkLinkAttributeLinkEnum,
	{hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, 0, 0x0, nullptr}};
hkClass hkLinkAttributeClass("hkLinkAttribute", nullptr, 1, nullptr, 0, hkLinkAttributeEnums, 1,
	hkLinkAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

// The enum's items and the enum.
using hkSemanticsAttributeSemanticsEnumItemArray = const hkClassEnum::Item[5];
static hkSemanticsAttributeSemanticsEnumItemArray hkSemanticsAttributeSemanticsEnumItems{
	{hkSemanticsAttribute::UNKNOWN, "UNKNOWN"}, {hkSemanticsAttribute::DISTANCE, "DISTANCE"},
	{hkSemanticsAttribute::ANGLE, "ANGLE"}, {hkSemanticsAttribute::NORMAL, "NORMAL"},
	{hkSemanticsAttribute::POSITION, "POSITION"}};
using hkSemanticsAttributeEnumArray = const hkClassEnum[1];
static hkSemanticsAttributeEnumArray hkSemanticsAttributeEnums{
	{"Semantics", hkSemanticsAttributeSemanticsEnumItems, 5, nullptr, 0}};
static const hkClassEnum* hkSemanticsAttributeSemanticsEnum = &hkSemanticsAttributeEnums[0];
// The class record's member table.
using hkSemanticsAttributeClassMemberArray = hkClassMember[1];
static hkSemanticsAttributeClassMemberArray hkSemanticsAttributeClass_Members{{"type", nullptr,
	hkSemanticsAttributeSemanticsEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, 0, 0x0, nullptr}};
hkClass hkSemanticsAttributeClass("hkSemanticsAttribute", nullptr, 1, nullptr, 0, hkSemanticsAttributeEnums, 1,
	hkSemanticsAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

// The class record's member table.
using hkDescriptionAttributeClassMemberArray = const hkClassMember[1];
static hkDescriptionAttributeClassMemberArray hkDescriptionAttributeClass_Members{
	{"string", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr}};
hkClass hkDescriptionAttributeClass("hkDescriptionAttribute", nullptr, 4, nullptr, 0, nullptr, 0,
	hkDescriptionAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

// The enum's items and the enum.
using hkArrayTypeAttributeArrayTypeEnumItemArray = const hkClassEnum::Item[3];
static hkArrayTypeAttributeArrayTypeEnumItemArray hkArrayTypeAttributeArrayTypeEnumItems{
	{hkArrayTypeAttribute::NONE, "NONE"}, {hkArrayTypeAttribute::POINTSOUP, "POINTSOUP"},
	{hkArrayTypeAttribute::ENTITIES, "ENTITIES"}};
using hkArrayTypeAttributeEnumArray = const hkClassEnum[1];
static hkArrayTypeAttributeEnumArray hkArrayTypeAttributeEnums{
	{"ArrayType", hkArrayTypeAttributeArrayTypeEnumItems, 3, nullptr, 0}};
static const hkClassEnum* hkArrayTypeAttributeArrayTypeEnum = &hkArrayTypeAttributeEnums[0];
// The class record's member table.
using hkArrayTypeAttributeClassMemberArray = hkClassMember[1];
static hkArrayTypeAttributeClassMemberArray hkArrayTypeAttributeClass_Members{{"type", nullptr,
	hkArrayTypeAttributeArrayTypeEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, 0, 0x0, nullptr}};
hkClass hkArrayTypeAttributeClass("hkArrayTypeAttribute", nullptr, 1, nullptr, 0, hkArrayTypeAttributeEnums, 1,
	hkArrayTypeAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

// The class record's member table.
using hkDataObjectTypeAttributeClassMemberArray = const hkClassMember[1];
static hkDataObjectTypeAttributeClassMemberArray hkDataObjectTypeAttributeClass_Members{
	{"typeName", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr}};
hkClass hkDataObjectTypeAttributeClass("hkDataObjectTypeAttribute", nullptr, 4, nullptr, 0, nullptr, 0,
	hkDataObjectTypeAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

// The class record's member table.
using hkDocumentationAttributeClassMemberArray = const hkClassMember[1];
static hkDocumentationAttributeClassMemberArray hkDocumentationAttributeClass_Members{{"docsSectionTag", nullptr,
	nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr}};
hkClass hkDocumentationAttributeClass("hkDocumentationAttribute", nullptr, 4, nullptr, 0, nullptr, 0,
	hkDocumentationAttributeClass_Members, 1, nullptr, nullptr, 0, 0);

void cleanupLoadedObjecthkGizmoAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkSemanticsAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkArrayTypeAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkLinkAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkUiAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkDescriptionAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkRangeRealAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkRangeInt32Attribute(void* p) {
	;
}

void cleanupLoadedObjecthkModelerNodeTypeAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkDataObjectTypeAttribute(void* p) {
	;
}

void cleanupLoadedObjecthkDocumentationAttribute(void* p) {
	;
}

const hkClass& hkRangeRealAttribute::staticClass() {
	return hkRangeRealAttributeClass;
}

const hkClass& hkRangeInt32Attribute::staticClass() {
	return hkRangeInt32AttributeClass;
}

const hkClass& hkUiAttribute::staticClass() {
	return hkUiAttributeClass;
}

const hkClass& hkGizmoAttribute::staticClass() {
	return hkGizmoAttributeClass;
}

const hkClass& hkModelerNodeTypeAttribute::staticClass() {
	return hkModelerNodeTypeAttributeClass;
}

const hkClass& hkLinkAttribute::staticClass() {
	return hkLinkAttributeClass;
}

const hkClass& hkSemanticsAttribute::staticClass() {
	return hkSemanticsAttributeClass;
}

const hkClass& hkDescriptionAttribute::staticClass() {
	return hkDescriptionAttributeClass;
}

const hkClass& hkArrayTypeAttribute::staticClass() {
	return hkArrayTypeAttributeClass;
}

const hkClass& hkDataObjectTypeAttribute::staticClass() {
	return hkDataObjectTypeAttributeClass;
}

const hkClass& hkDocumentationAttribute::staticClass() {
	return hkDocumentationAttributeClass;
}
