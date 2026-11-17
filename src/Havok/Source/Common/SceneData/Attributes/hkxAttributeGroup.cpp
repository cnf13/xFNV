#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroup.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkGsk.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedFloat.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedMatrix.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedQuaternion.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedVector.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxEnum.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedBool.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedEnum.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedInt.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedString.h"

// The members destroy the attributes, free their storage and the group name.
hkxAttributeGroup::~hkxAttributeGroup() {}

// The name is assigned, then the attributes: hkArray's operator= empties the array (each attribute's value reference
// released and its name destroyed, a constructor run for each of a negative count of new elements), then inserts a copy
// of other's attributes when it has any. The reserve that setSize(0) would do when the masked capacity is below zero
// can never run, so it is left out.
hkxAttributeGroup& hkxAttributeGroup::operator=(const hkxAttributeGroup& other) {
	m_name = other.m_name;
	// EXE-INLINED none: hkArray<hkxAttribute>::operator=
	hkMemoryRouter::getInstance();
	if (m_attributes.m_size > 0) {
		hkxAttribute* pAttribute = m_attributes.m_data;
		int32_t iCount = m_attributes.m_size;
		do {
			// EXE-INLINED none: hkxAttribute::~hkxAttribute
			hkReferencedObject* pValue = pAttribute->m_value.m_pntr;
			if (pValue) {
				pValue->removeReference();
			}
			pAttribute->m_value.m_pntr = nullptr;
			pAttribute->m_name.~hkStringPtr();
			++pAttribute;
			--iCount;
		} while (iCount);
	}
	hkxAttribute* pNew = m_attributes.m_data + m_attributes.m_size;
	for (int32_t i = -m_attributes.m_size; i > 0; --i) {
		if (pNew) {
			::new (pNew) hkxAttribute;
		}
		++pNew;
	}
	m_attributes.m_size = 0;
	const int32_t iCount = other.m_attributes.m_size;
	if (iCount) {
		hkxAttribute* pSource = other.m_attributes.m_data;
		m_attributes._insertAt(*hkMemoryRouter::getInstance().m_heap, 0, pSource, iCount);
	}
	return *this;
}

hkxSparselyAnimatedString* hkxAttributeGroup::findStringAttributeByName(const char* name) const {
	return static_cast<hkxSparselyAnimatedString*>(findAttributeObjectByName(name, &hkxSparselyAnimatedStringClass));
}

hkxAnimatedFloat* hkxAttributeGroup::findFloatAttributeByName(const char* name) const {
	return static_cast<hkxAnimatedFloat*>(findAttributeObjectByName(name, &hkxAnimatedFloatClass));
}

hkxAnimatedVector* hkxAttributeGroup::findVectorAttributeByName(const char* name) const {
	return static_cast<hkxAnimatedVector*>(findAttributeObjectByName(name, &hkxAnimatedVectorClass));
}

hkxAnimatedQuaternion* hkxAttributeGroup::findQuaternionAttributeByName(const char* name) const {
	return static_cast<hkxAnimatedQuaternion*>(findAttributeObjectByName(name, &hkxAnimatedQuaternionClass));
}

hkxAnimatedMatrix* hkxAttributeGroup::findMatrixAttributeByName(const char* name) const {
	return static_cast<hkxAnimatedMatrix*>(findAttributeObjectByName(name, &hkxAnimatedMatrixClass));
}

// The string attribute's first string, else the name of the enum attribute's first value; a warning when
// warnIfNotFound and neither exists.
hkResult hkxAttributeGroup::getStringValue(const char* name, bool warnIfNotFound, const char*& stringOut) {
	const hkxSparselyAnimatedString* pString = findStringAttributeByName(name);
	if (pString) {
		stringOut = pString->m_strings.m_data[0].cString();
		return HK_SUCCESS;
	}
	const hkxSparselyAnimatedEnum* pEnum = findEnumAttributeByName(name);
	if (pEnum) {
		pEnum->m_enum.m_pntr->getNameOfValue(pEnum->m_ints.m_data[0], &stringOut);
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "String attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 128);
	}
	return HK_FAILURE;
}

// The float attribute's first value; a warning when warnIfNotFound and it does not exist.
hkResult hkxAttributeGroup::getFloatValue(const char* name, bool warnIfNotFound, float& floatOut) const {
	const hkxAnimatedFloat* pFloat = findFloatAttributeByName(name);
	if (pFloat) {
		float fValue = pFloat->m_floats.m_data[0];
		floatOut = fValue;
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "Float attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 145);
	}
	return HK_FAILURE;
}

int32_t hkxAttributeGroup::findAttributeIndexByName(const char* name) const {
	int32_t v3 = 0;
	if (m_attributes.m_size <= 0) {
		return -1;
	}
	while (hkString::strCasecmp(reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_attributes.m_data[v3]
																	  .m_name.m_stringAndFlag) &
									~uintptr_t(1)),
		name)) {
		if (++v3 >= m_attributes.m_size) {
			return -1;
		}
	}
	return v3;
}

hkxSparselyAnimatedBool* hkxAttributeGroup::findBoolAttributeByName(const char* name) const {
	return static_cast<hkxSparselyAnimatedBool*>(findAttributeObjectByName(name, &hkxSparselyAnimatedBoolClass));
}

hkxSparselyAnimatedInt* hkxAttributeGroup::findIntAttributeByName(const char* name) const {
	return static_cast<hkxSparselyAnimatedInt*>(findAttributeObjectByName(name, &hkxSparselyAnimatedIntClass));
}

hkxSparselyAnimatedEnum* hkxAttributeGroup::findEnumAttributeByName(const char* name) const {
	return static_cast<hkxSparselyAnimatedEnum*>(findAttributeObjectByName(name, &hkxSparselyAnimatedEnumClass));
}

// The bool attribute's first value, else whether the int attribute's first value is not 0; a warning when
// warnIfNotFound and neither exists.
hkResult hkxAttributeGroup::getBoolValue(const char* name, bool warnIfNotFound, hkBool& boolOut) {
	const hkxSparselyAnimatedBool* pBool = findBoolAttributeByName(name);
	if (pBool) {
		boolOut.m_bool = pBool->m_bools.m_data[0].m_bool;
		return HK_SUCCESS;
	}
	const hkxSparselyAnimatedInt* pInt = findIntAttributeByName(name);
	if (pInt) {
		boolOut.m_bool = pInt->m_ints.m_data[0] != 0;
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "Bool attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 50);
	}
	return HK_FAILURE;
}

// The int attribute's first value, else the bool attribute's first value as 0 or 1, else the enum attribute's first
// value; a warning when warnIfNotFound and none exists.
hkResult hkxAttributeGroup::getIntValue(const char* name, bool warnIfNotFound, int32_t& intOut) const {
	const hkxSparselyAnimatedInt* pInt = findIntAttributeByName(name);
	if (pInt) {
		intOut = pInt->m_ints.m_data[0];
		return HK_SUCCESS;
	}
	const hkxSparselyAnimatedBool* pBool = findBoolAttributeByName(name);
	if (pBool) {
		intOut = pBool->m_bools.m_data[0].m_bool != 0;
		return HK_SUCCESS;
	}
	pInt = findEnumAttributeByName(name);
	if (pInt) {
		intOut = pInt->m_ints.m_data[0];
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "Integer attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 90);
	}
	return HK_FAILURE;
}

hkResult hkxAttributeGroup::getIntValue_00E5C8A0(const char* name, bool warnIfNotFound, uint32_t& intOut) const {
	return getIntValue(name, warnIfNotFound, *reinterpret_cast<int32_t*>(&intOut));
}

// Writes `index >= 0 ? m_attributes[index].m_value : HK_NULL` into apResult with a new reference. The null alternative
// is a temporary variant built from a null hkVariant and destroyed after the copy.
hkRefVariant* hkxAttributeGroup::findAttributeVariantByName(hkRefVariant* apResult, const char* name) const {
	FNVD_INLINED_LOCAL(hkRefVariant, nullValue, std::nullopt, std::nullopt);
	bool bNullValueBuilt = false;
	const int32_t index = findAttributeIndexByName(name);
	hkVariant nullVariant = {nullptr, nullptr};
	const hkRefVariant* pValue;
	if (index >= 0) {
		pValue = &m_attributes.m_data[index].m_value;
	} else {
		bNullValueBuilt = true;
		nullValue.assignVariant(nullVariant);
		pValue = &nullValue;
	}
	if (pValue->m_pntr) {
		pValue->m_pntr->addReference();
	}
	apResult->m_pntr = pValue->m_pntr;
	if (bNullValueBuilt) {
		// EXE-INLINED 0xC6CCA0
		if (nullValue.m_pntr) {
			nullValue.m_pntr->removeReference();
		}
	}
	return apResult;
}

// The attribute's object when type is null or its class has type's name, else null. The variant's reference is released
// before the object is returned.
hkReferencedObject* hkxAttributeGroup::findAttributeObjectByName(const char* name, const hkClass* type) const {
	FNVD_INLINED_LOCAL(hkRefVariant, variant, std::nullopt, std::nullopt);
	findAttributeVariantByName(&variant, name);
	if (!variant.m_pntr) {
		return nullptr;
	}
	if (type) {
		const char* pcVariantClassName = variant.getClass()->getName();
		if (hkString::strCasecmp(type->getName(), pcVariantClassName) != 0) {
			// EXE-INLINED 0xC6CCA0
			if (variant.m_pntr) {
				variant.m_pntr->removeReference();
			}
			return nullptr;
		}
	}
	hkReferencedObject* pObject = variant.m_pntr;
	// EXE-INLINED 0xC6CCA0
	if (pObject) {
		pObject->removeReference();
	}
	return pObject;
}

// The vector attribute's first value; a warning when warnIfNotFound and it does not exist. The warning names it a
// "Float attribute", as the game does.
hkResult hkxAttributeGroup::getVectorValue(const char* name, bool warnIfNotFound, hkVector4& vectorOut) {
	const hkxAnimatedVector* pVector = findVectorAttributeByName(name);
	if (pVector) {
		vectorOut.m_quad = pVector->m_vectors.m_data[0].m_quad;
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		alignas(16) char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "Float attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 162);
	}
	return HK_FAILURE;
}

// The quaternion attribute's first value; a warning when warnIfNotFound and it does not exist.
hkResult hkxAttributeGroup::getQuaternionValue(const char* name, bool warnIfNotFound, hkQuaternion& quaternionOut) {
	const hkxAnimatedQuaternion* pQuaternion = findQuaternionAttributeByName(name);
	if (pQuaternion) {
		quaternionOut.m_vec.m_quad = pQuaternion->m_quaternions.m_data[0].m_vec.m_quad;
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		alignas(16) char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "Quaternion attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 179);
	}
	return HK_FAILURE;
}

// The matrix attribute's first value; a warning when warnIfNotFound and it does not exist.
hkResult hkxAttributeGroup::getMatrixValue(const char* name, bool warnIfNotFound, hkMatrix4& matrixOut) {
	const hkxAnimatedMatrix* pMatrix = findMatrixAttributeByName(name);
	if (pMatrix) {
		matrixOut = pMatrix->m_matrices.m_data[0];
		return HK_SUCCESS;
	}
	if (warnIfNotFound) {
		alignas(16) char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "Matrix attribute " << name << " not found in " << m_name << " attribute group";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAB81), buf,
			".\\Attributes\\hkxAttributeGroup.cpp", 196);
	}
	return HK_FAILURE;
}
