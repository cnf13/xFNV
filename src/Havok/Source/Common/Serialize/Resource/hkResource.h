#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkRootLevelContainer;
class hkTypeInfoRegistry;

// Havok SDK: a loaded serialized resource. The slots after hkReferencedObject's, in order: getName (3),
// callDestructors, getImportsExports, getContentsPointer, getContentsTypeName (7).
class hkResource : public hkReferencedObject {
public:
	// A pointer the resource needs from outside, and one it offers. Both are two pointer-sized words.
	struct Import {
		const char* name;
		// The slot the imported pointer is written to.
		void** location;
	};

	struct Export {
		const char* name;
		void* data;
	};

	~hkResource() override;

	virtual const char* getName() const = 0;
	virtual void callDestructors() {}
	virtual void getImportsExports(hkArray<hkResource::Import>& impOut, hkArray<hkResource::Export>& expOut) const = 0;
	virtual void* getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const = 0;
	virtual const char* getContentsTypeName() const = 0;

	// The contents as a T: getContentsPointer with T's class name and the built-in type registry. Each instantiation
	// used gets an explicit specialization below.
	template <typename T> T* getContents() const;
};
// hkpHavokSnapshot::load's; the native build uses the same specialization.
template <> hkRootLevelContainer* hkResource::getContents<hkRootLevelContainer>() const;
