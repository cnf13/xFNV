#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

#include <stdint.h>

class hkMatrix4;
class hkQuaternion;
class hkxAnimatedFloat;
class hkxAnimatedMatrix;
class hkxAnimatedQuaternion;
class hkxAnimatedVector;
class hkxSparselyAnimatedBool;
class hkxSparselyAnimatedEnum;
class hkxSparselyAnimatedInt;
class hkxSparselyAnimatedString;

// A named group of attributes of a scene object. Not polymorphic.
struct hkxAttributeGroup {
	~hkxAttributeGroup();

	// Copies other's name and attributes; returns this. Used by hkxAttributeHolder's copy.
	hkxAttributeGroup& operator=(const hkxAttributeGroup& other);
	// Typed lookups: each calls findAttributeObjectByName with the type's class.
	hkxSparselyAnimatedString* findStringAttributeByName(const char* name) const;
	hkxAnimatedFloat* findFloatAttributeByName(const char* name) const;
	hkxAnimatedVector* findVectorAttributeByName(const char* name) const;
	hkxAnimatedQuaternion* findQuaternionAttributeByName(const char* name) const;
	hkxAnimatedMatrix* findMatrixAttributeByName(const char* name) const;
	// Writes the string attribute name (or the name of its enum attribute's value) into stringOut; returns 0 on
	// success, and warns "String attribute ... not found in" when warnIfNotFound and neither exists.
	hkResult getStringValue(const char* name, bool warnIfNotFound, const char*& stringOut);
	// Havok SDK signature (hkSceneData.lib, tied by bytes).
	hkResult getFloatValue(const char* name, bool warnIfNotFound, float& floatOut) const;
	int32_t findAttributeIndexByName(const char* name) const;
	hkxSparselyAnimatedBool* findBoolAttributeByName(const char* name) const;
	hkxSparselyAnimatedInt* findIntAttributeByName(const char* name) const;
	hkxSparselyAnimatedEnum* findEnumAttributeByName(const char* name) const;
	hkResult getBoolValue(const char* name, bool warnIfNotFound, hkBool& boolOut);
	// Reads the int attribute name (or its bool or enum form) into intOut.
	hkResult getIntValue(const char* name, bool warnIfNotFound, int32_t& intOut) const;
	// The getIntValue(const char*, bool, hkUint32&) const overload, renamed. Its body only forwards to getIntValue.
	hkResult getIntValue_00E5C8A0(const char* name, bool warnIfNotFound, uint32_t& intOut) const;
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
