#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class hkResourceContainer;
class hkStringBuf;

// Destroys the two hkStringPtr at p.
void cleanupLoadedObjecthkMemoryResourceHandleExternalLink(void* p);
// The finish constructor of the pair of string pointers at p (cdecl, 0xDFB390; in hkaAnnotationTrackClass.cpp).
void finishLoadedObjecthkMemoryResourceHandleExternalLink(void* p, int32_t finishing);

// A lookup of objects by name, for resolving links between resources. Not a reference counted object: its vtable
// holds findObjectByName (slot 0, as MSVC lays virtuals out in declaration order) and the destructor.
class hkResourceMap {
public:
	virtual void* findObjectByName(const char* objectName, const hkClass** klassOut) const = 0;

protected:
	virtual ~hkResourceMap();

public:
	// The deleting destructor's free (ourFree, 0xE2DEF0).
	static void operator delete(void* apMemory);
};

// A named node of a resource database: either a resource (an object and its class) or a container of them.
class hkResourceBase : public hkReferencedObject {
public:
	// The node kinds (the values of the exe's reflection enum 0x10DC748); both of the exe's getType slots hold the
	// shared `return 0` (0xE7CD40).
	enum Type : int32_t {
		TYPE_RESOURCE = 0,
		TYPE_CONTAINER = 1,
	};

	hkResourceBase() = default;
	// The SDK's finish constructor: a loaded object keeps what was loaded.
	hkResourceBase(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

	virtual Type getType() const = 0;
	// The node's name; buffer may hold it when the name has to be built.
	virtual const char* getName(hkStringBuf& buffer) const = 0;
};

// One object of a resource database and the links from its members to other resources.
class hkResourceHandle : public hkResourceBase {
public:
	// One unresolved pointer: the member that holds it and the name of the resource it points at.
	struct Link;

	hkResourceHandle() = default;
	// The SDK's finish constructor (hkMemoryResourceHandle's names it).
	hkResourceHandle(hkFinishLoadedObjectFlag flag) : hkResourceBase(flag) {}

	Type getType() const override;
	virtual void setName(const char* name) = 0;
	virtual void* getObject() const = 0;
	virtual const hkClass* getClass() const = 0;
	virtual void setObject(void* object, const hkClass* klass) = 0;
	virtual void addExternalLink(const char* memberName, const char* externalId) = 0;
	virtual void removeExternalLink(const char* memberName) = 0;
	virtual void getExternalLinks(hkArray<hkResourceHandle::Link>& linksOut) = 0;
	virtual void clearExternalLinks() = 0;
	// The exe has no hkResourceHandle vtable; this body is slot 13 of hkMemoryResourceHandle's.
	virtual void tryToResolveLinks(hkResourceMap& map);
	// Protected in the PDB.
	~hkResourceHandle() override;
};

struct hkResourceHandle::Link {
	const char* m_memberName;
	const char* m_externalId;
	hkClassMemberAccessor m_memberAccessor;
};

// A resource handle that keeps its object, name and links in memory.
class hkMemoryResourceHandle : public hkResourceHandle {
public:
	// One link as the handle stores it.
	struct ExternalLink {
		// The reflection class hkMemoryResourceHandleExternalLinkClass.
		static const hkClass& staticClass();

		hkStringPtr m_memberName;
		hkStringPtr m_externalId;
	};

	// The reflection's member table (the PDB's hkMemoryResourceHandle::Members; .rdata).
	using MembersArray = const hkClassMember[3];

	// No object, name or links.
	hkMemoryResourceHandle();
	// The finish constructor: the loaded members kept as loaded.
	hkMemoryResourceHandle(hkFinishLoadedObjectFlag flag);
	~hkMemoryResourceHandle() override;

	// The name without the string pointer's ownership flag, or "null" for no name; buffer is not used.
	const char* getName(hkStringBuf& buffer) const override;
	void setName(const char* name) override;
	// The object m_variant points at.
	void* getObject() const override;
	const hkClass* getClass() const override;
	void setObject(void* object, const hkClass* klass) override;
	void addExternalLink(const char* memberName, const char* externalId) override;
	void removeExternalLink(const char* memberName) override;
	void getExternalLinks(hkArray<hkResourceHandle::Link>& linksOut) override;
	void clearExternalLinks() override;

	// The reflection class hkMemoryResourceHandleClass.
	static const hkClass& staticClass();

	hkRefVariant m_variant;
	hkStringPtr m_name;
	hkArray<ExternalLink> m_references;

	static MembersArray Members;
};

// A node of a resource database that holds resources and child containers.
class hkResourceContainer : public hkResourceBase {
public:
	hkResourceContainer() = default;
	// The SDK's finish constructor (hkMemoryResourceContainer's names it).
	hkResourceContainer(hkFinishLoadedObjectFlag flag) : hkResourceBase(flag) {}

	Type getType() const override;
	virtual hkResourceHandle* createResource(const char* name, void* object, const hkClass* klass) = 0;
	virtual void destroyResource(hkResourceHandle* handle) = 0;
	virtual int32_t getNumResources() = 0;
	virtual hkResourceHandle* findResourceByName(const char* objectName, const hkClass* klass,
		const hkResourceHandle* prevObject) const = 0;
	virtual hkResourceContainer* getParent() = 0;
	virtual hkResourceContainer* createContainer(const char* name) = 0;
	virtual void destroyContainer(hkResourceContainer* container) = 0;
	virtual int32_t getNumContainers() = 0;
	virtual hkResourceContainer* findContainerByName(const char* containerName,
		const hkResourceContainer* prevContainer) const = 0;
	virtual hkResult parentTo(hkResourceContainer* newParent) = 0;
	// Resolves the links of every resource in this container and its children (the exe's slot 15, 0xE2E960; the
	// PDB names it for hkResourceContainer only, so hkMemoryResourceContainer's table holds the base's body).
	virtual void tryToResolveLinks(hkResourceMap& map);
	// Protected in the PDB.
	~hkResourceContainer() override;

	// Appends the handles of this container's children (recursively, children first) and
	// then its own to handle (thiscall, ret 4, 0xE2E2E0). The parameter name is the PDB's.
	void findAllResourceRecursively(hkArray<hkResourceHandle*>& resourcesOut);
	void findAllContainersRecursively(hkArray<hkResourceContainer*, hkContainerHeapAllocator>& resourcesOut);
	void getPath(hkStringBuf& pathOut);
};

// A resource container that keeps its handles and children in memory.
class hkMemoryResourceContainer : public hkResourceContainer {
public:
	// The reflection's member table (the PDB's hkMemoryResourceContainer::Members; .rdata).
	using MembersArray = const hkClassMember[4];

	// A container with no name, parent, handles or children, safe to destroy: the getVtable function declares the
	// local, then constructs it again in place.
	hkMemoryResourceContainer() : m_parent(nullptr) {}
	// An empty container named a1, with no parent.
	hkMemoryResourceContainer(const char* a1);
	// The finish constructor: when finishing, points each child back at this container.
	hkMemoryResourceContainer(hkFinishLoadedObjectFlag flag);
	~hkMemoryResourceContainer() override;

	// The name without the string pointer's ownership flag; buffer is not used.
	const char* getName(hkStringBuf& buffer) const override;
	hkResourceHandle* createResource(const char* name, void* object, const hkClass* klass) override;
	void destroyResource(hkResourceHandle* resourceHandle) override;
	int32_t getNumResources() override { return m_resourceHandles.m_size; }
	hkResourceHandle* findResourceByName(const char* objectName, const hkClass* klass,
		const hkResourceHandle* prevObject) const override;
	hkResourceContainer* getParent() override { return m_parent; }
	hkResourceContainer* createContainer(const char* name) override;
	void destroyContainer(hkResourceContainer* container2) override;
	// The number of child containers.
	int32_t getNumContainers() override;
	hkResourceContainer* findContainerByName(const char* containerName,
		const hkResourceContainer* prevContainer) const override;
	hkResult parentTo(hkResourceContainer* newParent) override;

	// The reflection class hkMemoryResourceContainerClass.
	static const hkClass& staticClass();

	hkStringPtr m_name;
	hkMemoryResourceContainer* m_parent;
	hkArray<hkMemoryResourceHandle*> m_resourceHandles;
	hkArray<hkMemoryResourceContainer*> m_children;

	static MembersArray Members;
};

// A resource map that looks objects up by name in the handles of a container (the SDK's class, in this header).
class hkContainerResourceMap : public hkResourceMap {
public:
	// The map of every resource in container and its children, by name.
	hkContainerResourceMap(hkResourceContainer* container);

	void* findObjectByName(const char* objectName, const hkClass** klassOut) const override;

	hkStringMap<hkResourceHandle*> m_resources;
};

extern hkClass hkMemoryResourceHandleClass;
extern hkClass hkMemoryResourceContainerClass;
inline constexpr fnvd::Vtable hkResourceMap_vftable{};

// The class's reflection record.
extern hkClass hkResourceBaseClass;

// The reflection class of hkResourceHandle (hkResourceHandleClass.cpp); hkBuiltinTypeRegistry's class table holds it.
extern hkClass hkResourceHandleClass;
inline constexpr fnvd::Vtable hkMemoryResourceHandle_vftable{};

// The class's reflection record.
extern hkClass hkResourceContainerClass;
inline constexpr fnvd::Vtable hkMemoryResourceContainer_vftable{};
inline constexpr fnvd::Vtable hkContainerResourceMap_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkMemoryResourceHandleExternalLinkTypeInfo;
