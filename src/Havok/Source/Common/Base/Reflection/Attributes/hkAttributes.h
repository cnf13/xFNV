#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

void cleanupLoadedObjecthkGizmoAttribute(void* p);
void cleanupLoadedObjecthkSemanticsAttribute(void* p);
void cleanupLoadedObjecthkArrayTypeAttribute(void* p);
void cleanupLoadedObjecthkLinkAttribute(void* p);
void cleanupLoadedObjecthkUiAttribute(void* p);
void cleanupLoadedObjecthkDescriptionAttribute(void* p);
void cleanupLoadedObjecthkRangeRealAttribute(void* p);
void cleanupLoadedObjecthkRangeInt32Attribute(void* p);
void cleanupLoadedObjecthkModelerNodeTypeAttribute(void* p);
void cleanupLoadedObjecthkDataObjectTypeAttribute(void* p);
void cleanupLoadedObjecthkDocumentationAttribute(void* p);

// A reflection attribute bounding a real member: the hard and the suggested range (the SDK's members).
struct hkRangeRealAttribute {
    // The reflection class hkRangeRealAttributeClass.
    static const hkClass& staticClass();

    float m_absmin;
    float m_absmax;
    float m_softmin;
    float m_softmax;
};

// The same for an integer member.
struct hkRangeInt32Attribute {
    // The reflection class hkRangeInt32AttributeClass.
    static const hkClass& staticClass();

    int32_t m_absmin;
    int32_t m_absmax;
    int32_t m_softmin;
    int32_t m_softmax;
};

// How a reflected member shows in the tools' user interface.
struct hkUiAttribute {
    enum HideInModeler : int32_t {
        NONE = 0,
        MAX = 1,
        MAYA = 2,
    };

    // The reflection class hkUiAttributeClass.
    static const hkClass& staticClass();

    hkBool m_visible;
    hkEnum<HideInModeler, int8_t> m_hideInModeler;
    const char* m_label;
    const char* m_group;
    const char* m_hideBaseClassMembers;
    hkBool m_endGroup;
    hkBool m_endGroup2;
    hkBool m_advanced;
};

// A gizmo the tools draw for a reflected member.
struct hkGizmoAttribute {
    enum GizmoType : int32_t {
        POINT = 0,
        SPHERE = 1,
        PLANE = 2,
        ARROW = 3,
    };

    // The reflection class hkGizmoAttributeClass.
    static const hkClass& staticClass();

    hkBool m_visible;
    const char* m_label;
    hkEnum<GizmoType, int8_t> m_type;
};

// The kind of modeler node a reflected class becomes.
struct hkModelerNodeTypeAttribute {
    enum ModelerType : int32_t {
        DEFAULT = 0,
        LOCATOR = 1,
    };

    // The reflection class hkModelerNodeTypeAttributeClass.
    static const hkClass& staticClass();

    hkEnum<ModelerType, int8_t> m_type;
};

// How a reflected member links to another object.
struct hkLinkAttribute {
    enum Link : int32_t {
        NONE = 0,
        DIRECT_LINK = 1,
        CHILD = 2,
        MESH = 3,
        PARENT_NAME = 4,
    };

    // The reflection class hkLinkAttributeClass.
    static const hkClass& staticClass();

    hkEnum<Link, int8_t> m_type;
};

// What a reflected member's value means.
struct hkSemanticsAttribute {
    enum Semantics : int32_t {
        UNKNOWN = 0,
        DISTANCE = 1,
        ANGLE = 2,
        NORMAL = 3,
        POSITION = 4,
    };

    // The reflection class hkSemanticsAttributeClass.
    static const hkClass& staticClass();

    hkEnum<Semantics, int8_t> m_type;
};

// A reflected member's description text.
struct hkDescriptionAttribute {
    // The reflection class hkDescriptionAttributeClass.
    static const hkClass& staticClass();

    char* m_string;
};

// What a reflected array holds for the tools.
struct hkArrayTypeAttribute {
    enum ArrayType : int32_t {
        NONE = 0,
        POINTSOUP = 1,
        ENTITIES = 2,
    };

    // The reflection class hkArrayTypeAttributeClass.
    static const hkClass& staticClass();

    hkEnum<ArrayType, int8_t> m_type;
};

// The data object type name of a reflected member.
struct hkDataObjectTypeAttribute {
    // The reflection class hkDataObjectTypeAttributeClass.
    static const hkClass& staticClass();

    char* m_typeName;
};

// The documentation section of a reflected class.
struct hkDocumentationAttribute {
    // The reflection class hkDocumentationAttributeClass.
    static const hkClass& staticClass();

    const char* m_docsSectionTag;
};

extern hkClass hkRangeRealAttributeClass;
extern hkClass hkRangeInt32AttributeClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkRangeRealAttributeTypeInfo;
extern hkTypeInfo hkRangeInt32AttributeTypeInfo;
extern hkTypeInfo hkUiAttributeTypeInfo;
extern hkTypeInfo hkGizmoAttributeTypeInfo;
extern hkTypeInfo hkModelerNodeTypeAttributeTypeInfo;
extern hkTypeInfo hkLinkAttributeTypeInfo;
extern hkTypeInfo hkSemanticsAttributeTypeInfo;
extern hkTypeInfo hkDescriptionAttributeTypeInfo;
extern hkTypeInfo hkArrayTypeAttributeTypeInfo;
extern hkTypeInfo hkDataObjectTypeAttributeTypeInfo;
extern hkTypeInfo hkDocumentationAttributeTypeInfo;
extern hkClass hkUiAttributeClass;
extern hkClass hkGizmoAttributeClass;
extern hkClass hkModelerNodeTypeAttributeClass;
extern hkClass hkLinkAttributeClass;
extern hkClass hkSemanticsAttributeClass;
extern hkClass hkDescriptionAttributeClass;
extern hkClass hkArrayTypeAttributeClass;
extern hkClass hkDataObjectTypeAttributeClass;
extern hkClass hkDocumentationAttributeClass;
