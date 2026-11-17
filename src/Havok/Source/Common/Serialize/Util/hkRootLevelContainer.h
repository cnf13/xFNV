#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"

#include <stdint.h>

// The top object of a Havok file: a list of named objects.
class hkRootLevelContainer {
public:
	// One named object and its class name.
	class NamedVariant {
	public:
		// The reflection's member table.
		using MembersArray = const hkClassMember[3];

		NamedVariant(const char* name, void* object, const hkClass* klass);
		NamedVariant(const char* name, const hkVariant& v);
		NamedVariant(hkFinishLoadedObjectFlag flag);

		// The class of the object, from its variant's class when it has one, else the stored name.
		const char* getTypeName() const {
			return m_variant.getClass() ? m_variant.getClass()->getName() : m_className.cString();
		}

		void* getObject() const { return m_variant.m_pntr; }

		static const hkClass& staticClass();

		hkStringPtr m_name;
		hkStringPtr m_className;
		hkRefVariant m_variant;

		static MembersArray Members;
	};

	hkRootLevelContainer() {}
	// The finish constructor of a loaded container: the array keeps what was loaded.
	hkRootLevelContainer(hkFinishLoadedObjectFlag flag);

	// The first object after prevObject (from the start when it is null) whose class is typeName, or null.
	void* findObjectByType(const char* typeName, const void* prevObject) const;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void* findObjectByName(const char* objectName, const void* prevObject) const;

	// DeletingDestruct's free: the heap's blockFree of the object's size.
	static void operator delete(void* apMemory, size_t aSize);
	// The reflection class hkRootLevelContainerClass.
	static const hkClass& staticClass();

	hkArray<NamedVariant> m_namedVariants;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkRootLevelContainerNamedVariantTypeInfo;
extern hkTypeInfo hkRootLevelContainerTypeInfo;
extern hkClass hkRootLevelContainerClass;
