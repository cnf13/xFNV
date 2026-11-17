#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"

#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkRangeRealAttributeTypeInfo = {"hkRangeRealAttribute", nullptr, cleanupLoadedObjecthkRangeRealAttribute,
	nullptr, &typeid(hkRangeRealAttribute), sizeof(hkRangeRealAttribute)};
hkTypeInfo hkRangeInt32AttributeTypeInfo = {"hkRangeInt32Attribute", nullptr, cleanupLoadedObjecthkRangeInt32Attribute,
	nullptr, &typeid(hkRangeInt32Attribute), sizeof(hkRangeInt32Attribute)};
hkTypeInfo hkUiAttributeTypeInfo = {"hkUiAttribute", nullptr, cleanupLoadedObjecthkUiAttribute, nullptr,
	&typeid(hkUiAttribute), sizeof(hkUiAttribute)};
hkTypeInfo hkGizmoAttributeTypeInfo = {"hkGizmoAttribute", nullptr, cleanupLoadedObjecthkGizmoAttribute, nullptr,
	&typeid(hkGizmoAttribute), sizeof(hkGizmoAttribute)};
hkTypeInfo hkModelerNodeTypeAttributeTypeInfo = {"hkModelerNodeTypeAttribute", nullptr,
	cleanupLoadedObjecthkModelerNodeTypeAttribute, nullptr, &typeid(hkModelerNodeTypeAttribute),
	sizeof(hkModelerNodeTypeAttribute)};
hkTypeInfo hkLinkAttributeTypeInfo = {"hkLinkAttribute", nullptr, cleanupLoadedObjecthkLinkAttribute, nullptr,
	&typeid(hkLinkAttribute), sizeof(hkLinkAttribute)};
hkTypeInfo hkSemanticsAttributeTypeInfo = {"hkSemanticsAttribute", nullptr, cleanupLoadedObjecthkSemanticsAttribute,
	nullptr, &typeid(hkSemanticsAttribute), sizeof(hkSemanticsAttribute)};
hkTypeInfo hkDescriptionAttributeTypeInfo = {"hkDescriptionAttribute", nullptr,
	cleanupLoadedObjecthkDescriptionAttribute, nullptr, &typeid(hkDescriptionAttribute),
	sizeof(hkDescriptionAttribute)};
hkTypeInfo hkArrayTypeAttributeTypeInfo = {"hkArrayTypeAttribute", nullptr, cleanupLoadedObjecthkArrayTypeAttribute,
	nullptr, &typeid(hkArrayTypeAttribute), sizeof(hkArrayTypeAttribute)};
hkTypeInfo hkDataObjectTypeAttributeTypeInfo = {"hkDataObjectTypeAttribute", nullptr,
	cleanupLoadedObjecthkDataObjectTypeAttribute, nullptr, &typeid(hkDataObjectTypeAttribute),
	sizeof(hkDataObjectTypeAttribute)};
hkTypeInfo hkDocumentationAttributeTypeInfo = {"hkDocumentationAttribute", nullptr,
	cleanupLoadedObjecthkDocumentationAttribute, nullptr, &typeid(hkDocumentationAttribute),
	sizeof(hkDocumentationAttribute)};
