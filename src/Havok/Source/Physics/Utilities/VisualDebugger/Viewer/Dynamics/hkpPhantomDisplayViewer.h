#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Reconstruction.hpp"

class hkpWorldObject;

// The singleton's create function (HK_SINGLETON_IMPLEMENTATION's; cdecl, 0xD66730), which the singleton init node
// (0x11B5F40) points at.
hkReferencedObject* hkpUserShapePhantomTypeIdentifiercreate();

// The viewer that draws the shapes of the phantoms of the worlds it watches (the phantoms it hears being added and
// removed). The SDK derives the listeners protected; the phantom listener is public here so that the registration of
// its slot can convert from it (the layout is the same).
class hkpPhantomDisplayViewer : public hkpWorldViewerBase,
								public hkpPhantomListener,
								protected hkpWorldPostSimulationListener {
public:
	hkpPhantomDisplayViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpPhantomDisplayViewer() override;

	int32_t getProcessTag() override { return m_tag; }
	void init() override;
	void phantomAddedCallback(hkpPhantom* phantom) override;
	void phantomRemovedCallback(hkpPhantom* phantom) override;
	void postSimulationCallback(hkpWorld* world) override;
	void worldAddedCallback(hkpWorld* world) override;
	void worldRemovedCallback(hkpWorld* world) override;

	void removeAllGeometries();
	void addWorld(hkpWorld* world);
	void removeWorld(hkpWorld* world);

	// Makes the viewer for the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();
	static const char* getName() { return "Phantoms"; }
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<hkpWorldObject*> m_phantomShapesCreated;

	// The viewer's process tag.
	static inline int32_t m_tag;
};

// The registry of the user phantom types that are shape phantoms (the viewer draws their shapes). The exe's vtable
// overrides only the destructor.
class hkpUserShapePhantomTypeIdentifier : public hkReferencedObject,
										  public hkSingleton<hkpUserShapePhantomTypeIdentifier> {
public:
	~hkpUserShapePhantomTypeIdentifier() override;

	void registerShapePhantomType(hkpPhantomType t);

	hkArray<hkpPhantomType> m_shapePhantomTypes;

	// hkSingleton<hkpUserShapePhantomTypeIdentifier>::s_instance: the exe's singleton init node (0x11B5F40) for the
	// create function 0xD66730 points at it.
	static inline hkpUserShapePhantomTypeIdentifier* s_instance;
};

inline constexpr fnvd::Vtable hkpPhantomDisplayViewer_vftable{};
inline constexpr fnvd::Vtable hkpPhantomDisplayViewer_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkpPhantomDisplayViewer_vftable_hkpPhysicsContextWorldListener{};
inline constexpr fnvd::Vtable hkpPhantomDisplayViewer_vftable_hkpPhantomListener{};
inline constexpr fnvd::Vtable hkpPhantomDisplayViewer_vftable_hkpWorldPostSimulationListener{};
inline constexpr fnvd::Vtable hkpUserShapePhantomTypeIdentifier_vftable{};

// The colour the viewer gives phantom geometries: the file's own copy, which the exe reads.
extern const int32_t s_phantomColor;
