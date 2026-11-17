#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Reconstruction.hpp"

class hkClassNameRegistry;
class hkTypeInfoRegistry;

// Havok SDK: a resource wrapping one top-level object. stealContentsPointer is its own slot 8.
class hkObjectResource : public hkResource {
public:
	// Holds v and references on the builtin class name and type info registries.
	hkObjectResource(const hkVariant& v);
	~hkObjectResource() override;

	const char* getName() const override;
	void callDestructors() override {}
	void getImportsExports(hkArray<hkResource::Import>& impOut, hkArray<hkResource::Export>& expOut) const override;
	void* getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const override;
	const char* getContentsTypeName() const override;
	virtual void* stealContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry);

	// Takes a reference on classReg, releases the old registry and stores it in
	// m_classRegistry (thiscall, ret 4, 0xE04EB0; Copier::toObject 0xDC4710 calls it on the new resource).
	void setClassNameRegistry(const hkClassNameRegistry* classReg);
	// The same for m_typeRegistry.
	void setTypeInfoRegistry(const hkTypeInfoRegistry* typeReg);

	hkVariant m_topLevelObject;
	hkRefPtr<const hkClassNameRegistry> m_classRegistry;
	hkRefPtr<const hkTypeInfoRegistry> m_typeRegistry;
};
inline constexpr fnvd::Vtable hkObjectResource_vftable{};
