#pragma once

#include <stdint.h>

class hkClass;
class hkClassEnum;

// The constant forms the generated *Class.cpp tables are written in (each class's Members, EnumItems and Enums
// arrays and its attributes): the same layouts as hkCustomAttributes, hkClassMember, hkClassEnum::Item, hkClassEnum
// and hkClass, which read them in place.

// A class's or member's attributes: m_numAttributes named values, each with the class describing its data.
struct hkInternalCustomAttributes {
	struct Attribute {
		const char* m_name;
		const void* m_data;
		const hkClass* m_klass;
	};

	const Attribute* m_attributes;
	int32_t m_numAttributes;
};

// One member of a reflected class: its name, the class or enum of its type, the type codes, the C array size, the
// flags and its offset in the object.
struct hkInternalClassMember {
	const char* m_name;
	const hkClass* m_class;
	const hkClassEnum* m_enum;
	uint8_t m_type;
	uint8_t m_subtype;
	uint16_t m_cArraySize;
	uint16_t m_flags;
	uint16_t m_offset;
	const hkInternalCustomAttributes* m_attributes;
};

// One value of a reflected enum and its name.
struct hkInternalClassEnumItem {
	int32_t m_value;
	const char* m_name;
};

// A reflected enum: its name and m_numItems values.
struct hkInternalClassEnum {
	const char* m_name;
	const hkInternalClassEnumItem* m_items;
	int32_t m_numItems;
	const hkInternalCustomAttributes* m_attributes;
	uint32_t m_flags;
};

// A reflected class: its name, parent, object size, interfaces, declared enums and members, default values,
// attributes, flags and the version it describes.
struct hkInternalClass {
	const char* m_name;
	const hkInternalClass* m_parent;
	int32_t m_objectSize;
	int32_t m_numImplementedInterfaces;
	const hkInternalClassEnum* m_declaredEnums;
	int32_t m_numDeclaredEnums;
	const hkInternalClassMember* m_declaredMembers;
	int32_t m_numDeclaredMembers;
	const void* m_defaults;
	const hkInternalCustomAttributes* m_attributes;
	int32_t m_flags;
	int32_t m_describedVersion;
};
