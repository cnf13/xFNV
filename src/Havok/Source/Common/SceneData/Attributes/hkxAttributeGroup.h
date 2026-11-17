#pragma once

#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkBool;
class hkClass;
class hkMatrix4;
class hkQuaternion;
class hkRefVariant;
class hkReferencedObject;
class hkStringPtr;
class hkVector4;
class hkxAnimatedFloat;
class hkxAnimatedMatrix;
class hkxAnimatedQuaternion;
class hkxAnimatedVector;
class hkxSparselyAnimatedBool;
class hkxSparselyAnimatedEnum;
class hkxSparselyAnimatedInt;
class hkxSparselyAnimatedString;

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

// A named group of attributes of a scene object. Not polymorphic.
struct hkxAttributeGroup {
	~hkxAttributeGroup();

	// Copies other's name and attributes; returns this (thiscall, ret 4, 0xE5CCE0). The PDB's hkxAttributeGroup's
	// operator=, which the exe calls out of line from hkxAttributeHolder's copy.
	hkxAttributeGroup& operator=(const hkxAttributeGroup& other);
	// The Havok SDK's typed lookups (hkSceneData.lib's mangled names): findAttributeObjectByName with the type's
	// class (thiscall, ret 4; 0xE5C610, 0xE5C630, 0xE5C650, 0xE5C670, 0xE5C690).
	hkxSparselyAnimatedString* findStringAttributeByName(const char* name) const;
	hkxAnimatedFloat* findFloatAttributeByName(const char* name) const;
	hkxAnimatedVector* findVectorAttributeByName(const char* name) const;
	hkxAnimatedQuaternion* findQuaternionAttributeByName(const char* name) const;
	hkxAnimatedMatrix* findMatrixAttributeByName(const char* name) const;
	// The string attribute name (or the name of its enum attribute's value) into value; 0 on success, a warning when
	// warnIfNotFound and neither exists (thiscall, ret 0xC, 0xE5C8B0). The PDB's getStringValue: its warning reads
	// "String attribute ... not found in", as the exe's.
	hkResult getStringValue(const char* name, bool warnIfNotFound, const char*& stringOut);
	// Havok SDK signature (hkSceneData.lib, tied by bytes).
	hkResult getFloatValue(const char* name, bool warnIfNotFound, float& floatOut) const;
	// The PDB's (hkSceneData.lib's mangled names): const members (thiscall, ret 4; 0xE5C490, 0xE5C5B0, 0xE5C5D0,
	// 0xE5C5F0).
	int32_t findAttributeIndexByName(const char* name) const;
	hkxSparselyAnimatedBool* findBoolAttributeByName(const char* name) const;
	hkxSparselyAnimatedInt* findIntAttributeByName(const char* name) const;
	hkxSparselyAnimatedEnum* findEnumAttributeByName(const char* name) const;
	hkResult getBoolValue(const char* name, bool warnIfNotFound, hkBool& boolOut);
	// The int attribute name (or its bool or enum form) into intOut (thiscall, ret 0xC, 0xE5C7A0). The PDB's
	// hkUint32& overload is the exe's 5-byte jump to this body (0xE5C8A0).
	hkResult getIntValue(const char* name, bool warnIfNotFound, int32_t& intOut) const;
	// The PDB's getIntValue(const char*, bool, hkUint32&) const overload, renamed: a second
	// getIntValue would make the registration of 0xE5C7A0 ambiguous. The exe's body is one jmp to 0xE5C7A0
	// (thiscall, ret 0xC, 0xE5C8A0).
	hkResult getIntValue_00E5C8A0(const char* name, bool warnIfNotFound, uint32_t& intOut) const;
	// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
	// Havok SDK: result through the hidden result pointer apResult.
	hkRefVariant* findAttributeVariantByName(hkRefVariant* apResult, const char* name) const;
	// The attribute's object when its class is type's (by name), else null; a null type takes any.
	// The Havok SDK's hkSceneData.lib name and signature (const).
	hkReferencedObject* findAttributeObjectByName(const char* name, const hkClass* type) const;
	hkResult getVectorValue(const char* name, bool warnIfNotFound, hkVector4& vectorOut);
	hkResult getQuaternionValue(const char* name, bool warnIfNotFound, hkQuaternion& quaternionOut);
	hkResult getMatrixValue(const char* name, bool warnIfNotFound, hkMatrix4& matrixOut);

	static const hkClass& staticClass();

	hkStringPtr m_name;
	hkArray<hkxAttribute> m_attributes;
};

extern hkClass hkxSparselyAnimatedEnumClass;
extern hkTypeInfo hkxSparselyAnimatedEnumTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxAttributeGroupTypeInfo;
