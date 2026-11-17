#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpPhantomDisplayViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/hkpShapeDisplayBuilder.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

const int32_t s_phantomColor = hkColor::rgbFromChars(240, 200, 0, 200);

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkpUserShapePhantomTypeIdentifier(
    "hkpUserShapePhantomTypeIdentifier",
    &hkpUserShapePhantomTypeIdentifiercreate,
    reinterpret_cast<void**>(&hkpUserShapePhantomTypeIdentifier::s_instance)
);

// The SDK's singleton create function (HK_SINGLETON_IMPLEMENTATION): a new registry with an empty type list.
hkReferencedObject* hkpUserShapePhantomTypeIdentifiercreate() {
    hkMemoryRouter* router = &hkMemoryRouter::getInstance();
    hkpUserShapePhantomTypeIdentifier* identifier = static_cast<hkpUserShapePhantomTypeIdentifier*>(
        router->m_heap->blockAlloc(sizeof(hkpUserShapePhantomTypeIdentifier))
    );
    identifier->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpUserShapePhantomTypeIdentifier));
    return ::new (identifier) hkpUserShapePhantomTypeIdentifier;
}

hkpPhantomDisplayViewer::hkpPhantomDisplayViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpPhantomListener(), hkpWorldPostSimulationListener(), m_phantomShapesCreated() {}

// Removes the viewer from the context's worlds; the shapes list goes with its member.
hkpPhantomDisplayViewer::~hkpPhantomDisplayViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            removeWorld(m_context->m_worlds.m_data[i]);
        }
    }
}

void hkpPhantomDisplayViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            addWorld(m_context->m_worlds.m_data[i]);
            m_context = this->m_context;
        }
    }
}

// Only shape phantoms (simple, caching, or a user type registered as a shape phantom) that have a shape are drawn.
// Display geometries that are convex but came without geometry data are reported and dropped, the rest are added under
// the collidable's address as id and coloured, then deleted. The geometry list starts empty. The builder and its
// environment are constructed on raw storage and neither is destroyed, as the game does.
void hkpPhantomDisplayViewer::phantomAddedCallback(hkpPhantom* phantom) {
    const hkpShape* shape = phantom->m_collidable.m_shape;
    const int32_t type = phantom->getType();
    FNVD_INLINED_LOCAL(hkArray<hkDisplayGeometry*>, displayGeometries, std::nullopt, std::nullopt);
    displayGeometries.m_data = nullptr;
    displayGeometries.m_size = 0;
    displayGeometries.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
    bool isShapePhantom = type == HK_PHANTOM_SIMPLE_SHAPE || type == HK_PHANTOM_CACHING_SHAPE;
    if (!isShapePhantom) {
        const hkArray<hkpPhantomType>& userTypes = hkpUserShapePhantomTypeIdentifier::s_instance->m_shapePhantomTypes;
        const int32_t numUserTypes = userTypes.m_size;
        int32_t index = 0;
        for (; index < numUserTypes; ++index) {
            if (userTypes.m_data[index] == type) {
                break;
            }
        }
        if (index >= numUserTypes) {
            index = -1;
        }
        isShapePhantom = index != -1;
    }
    if (isShapePhantom && shape) {
        alignas(4) uint8_t aEnvironmentStorage[sizeof(hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment)];
        hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment* pEnvironment =
            ::new (aEnvironmentStorage) hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment;
        alignas(4) uint8_t aBuilderStorage[sizeof(hkpShapeDisplayBuilder)];
        hkpShapeDisplayBuilder* pBuilder = ::new (aBuilderStorage) hkpShapeDisplayBuilder(*pEnvironment);
        pBuilder->buildDisplayGeometries(shape, displayGeometries);

        for (int32_t i = displayGeometries.m_size - 1; i >= 0; --i) {
            hkDisplayGeometry* geometry = displayGeometries.m_data[i];
            if (geometry->m_type == HK_DISPLAY_CONVEX && !geometry->m_geometry) {
                alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
                char acBuffer[0x200];
                hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
                *pStream << "Unable to build display geometry from hkpShape geometry data";
                hkError::s_instance->message(
                    hkError::MESSAGE_REPORT, -1, acBuffer,
                    ".\\VisualDebugger\\Viewer\\Dynamics\\hkpPhantomDisplayViewer.cpp", 0x91
                );
                FNVD_DESTRUCT(hkOstream)(pStream);
                const int32_t last = displayGeometries.m_size - 1;
                displayGeometries.m_size = static_cast<uint32_t>(last);
                if (last != i) {
                    displayGeometries.m_data[i] = displayGeometries.m_data[last];
                }
            }
        }

        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        if (m_phantomShapesCreated.m_size ==
            (m_phantomShapesCreated.m_capacityAndFlags & hkArrayBase<hkpWorldObject*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(heap, &m_phantomShapesCreated, sizeof(hkpWorldObject*));
        }
        m_phantomShapesCreated.m_data[m_phantomShapesCreated.m_size] = phantom;
        ++m_phantomShapesCreated.m_size;

        hkpCollidable* collidable = &phantom->m_collidable;
        const hkUlong id = reinterpret_cast<hkUlong>(collidable);
        m_displayHandler->addGeometry(
            displayGeometries, *collidable->m_motion.pMotion, id, m_tag, reinterpret_cast<hkUlong>(shape)
        );
        m_displayHandler->setGeometryColor(s_phantomColor, id, m_tag);
        for (int32_t i = 0; i < displayGeometries.m_size; ++i) {
            delete displayGeometries.m_data[i];
        }
    }

    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    const int32_t capacityAndFlags = displayGeometries.m_capacityAndFlags;
    displayGeometries.m_size = 0;
    if (capacityAndFlags >= 0) {
        heap.bufFree(
            displayGeometries.m_data,
            (capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkDisplayGeometry*))
        );
    }
}

// A phantom whose shape geometry was added is dropped from the list and its geometry removed. The ids are the
// collidable's and the shape's addresses, which hkDebugDisplayHandler takes as uint32_t, as phantomAddedCallback passes
// them.
void hkpPhantomDisplayViewer::phantomRemovedCallback(hkpPhantom* phantom) {
    int32_t m_size = m_phantomShapesCreated.m_size;
    int32_t v3 = 0;
    if (m_size > 0) {
        for (hkpWorldObject** i = m_phantomShapesCreated.begin(); *i != phantom; ++i) {
            if (++v3 >= m_size) {
                return;
            }
        }
        if (v3 >= 0) {
            int32_t v5 = --m_phantomShapesCreated.m_size;
            if (v5 != v3) {
                m_phantomShapesCreated.m_data[v3] = m_phantomShapesCreated.m_data[v5];
            }
            m_displayHandler->removeGeometry(
                reinterpret_cast<hkUlong>(&phantom->m_collidable), hkpPhantomDisplayViewer::m_tag,
                reinterpret_cast<hkUlong>(phantom->m_collidable.m_shape)
            );
        }
    }
}

// Each phantom's AABB is drawn as an immediate display geometry, from two local arrays on the thread's stack allocator
// (the AABBs and pointers to them), then each shape phantom's geometry follows its transform. The stack allocator's
// bytes are untyped memory the arrays keep their elements in (the reinterpret_casts).
void hkpPhantomDisplayViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpPhantomDisplayViewer", this);

    alignas(16) hkLocalArray<hkDisplayAABB> phantomAabbs;
    {
        // EXE-INLINED none: hkLocalArray<hkDisplayAABB>::hkLocalArray(phantoms.getSize()) (hkAllocateStack)
        const int32_t capacity = world->m_phantoms.m_size;
        phantomAabbs.m_data = nullptr;
        phantomAabbs.m_size = 0;
        phantomAabbs.m_capacityAndFlags = hkArrayBase<hkDisplayAABB>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + ((capacity * static_cast<int32_t>(sizeof(hkDisplayAABB)) + 127) & ~127);
        phantomAabbs.m_capacityAndFlags = capacity | hkArrayBase<hkDisplayAABB>::DONT_DEALLOCATE_FLAG;
        phantomAabbs.m_data = reinterpret_cast<hkDisplayAABB*>(pBlock);
        phantomAabbs.m_localMemory = phantomAabbs.m_data;
    }
    {
        // EXE-INLINED none: hkArray<hkDisplayAABB>::setSize(phantoms.getSize())
        const int32_t size = world->m_phantoms.m_size;
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        const int32_t capacity = phantomAabbs.m_capacityAndFlags & hkArrayBase<hkDisplayAABB>::CAPACITY_MASK;
        if (capacity < size) {
            const int32_t doubled = 2 * capacity;
            hkArrayUtil::_reserve(
                heap, &phantomAabbs, size < doubled ? doubled : size, static_cast<int32_t>(sizeof(hkDisplayAABB))
            );
        }
        // EXE-INLINED none: hkArrayUtil::destruct<hkDisplayAABB> (each element's virtual destructor)
        for (int32_t i = size; i < phantomAabbs.m_size; ++i) {
            hkDisplayAABB* pAabb = &phantomAabbs.m_data[i];
            pAabb->~hkDisplayAABB();
        }
        // EXE-INLINED none: hkArrayUtil::construct<hkDisplayAABB> (placement new, null-tested)
        for (int32_t i = phantomAabbs.m_size; i < size; ++i) {
            hkDisplayAABB* pAabb = &phantomAabbs.m_data[i];
            if (pAabb) {
                ::new (pAabb) hkDisplayAABB;
            }
        }
        phantomAabbs.m_size = size;
    }

    alignas(16) hkLocalArray<hkDisplayGeometry*> phantomAabbPtrs;
    {
        // EXE-INLINED none: hkLocalArray<hkDisplayGeometry*>::hkLocalArray(phantoms.getSize()) (hkAllocateStack)
        const int32_t capacity = world->m_phantoms.m_size;
        phantomAabbPtrs.m_data = nullptr;
        phantomAabbPtrs.m_size = 0;
        phantomAabbPtrs.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + ((capacity * static_cast<int32_t>(sizeof(hkDisplayGeometry*)) + 127) & ~127);
        phantomAabbPtrs.m_capacityAndFlags = capacity | hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
        phantomAabbPtrs.m_data = reinterpret_cast<hkDisplayGeometry**>(pBlock);
        phantomAabbPtrs.m_localMemory = phantomAabbPtrs.m_data;
    }
    {
        // EXE-INLINED none: hkArray<hkDisplayGeometry*>::setSize(phantoms.getSize())
        const int32_t size = world->m_phantoms.m_size;
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        const int32_t capacity = phantomAabbPtrs.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK;
        if (capacity < size) {
            const int32_t doubled = 2 * capacity;
            hkArrayUtil::_reserve(
                heap, &phantomAabbPtrs, size < doubled ? doubled : size,
                static_cast<int32_t>(sizeof(hkDisplayGeometry*))
            );
        }
        phantomAabbPtrs.m_size = size;
    }

    for (int32_t i = 0; i < world->m_phantoms.m_size; ++i) {
        hkAabb aabb;
        world->m_phantoms.m_data[i]->calcAabb(aabb);
        phantomAabbs.m_data[i].setExtents(aabb.m_min, aabb.m_max);
        phantomAabbPtrs.m_data[i] = &phantomAabbs.m_data[i];
    }
    m_displayHandler->displayGeometry(phantomAabbPtrs, s_phantomColor, 0, m_tag);

    for (int32_t j = 0; j < m_phantomShapesCreated.m_size; ++j) {
        hkpWorldObject* phantom = m_phantomShapesCreated.m_data[j];
        m_displayHandler->updateGeometry(
            phantom->m_collidable.m_motion.pState->m_transform, reinterpret_cast<hkUlong>(&phantom->m_collidable),
            hkpPhantomDisplayViewer::m_tag
        );
    }

    HK_TIMER_END();
}

void hkpPhantomDisplayViewer::worldAddedCallback(hkpWorld* world) {
    addWorld(world);
}

void hkpPhantomDisplayViewer::worldRemovedCallback(hkpWorld* world) {
    removeWorld(world);
}

// The full object as this: init calls it per world.
void hkpPhantomDisplayViewer::addWorld(hkpWorld* world) {
    hkpPhantomListener* v3;
    hkpWorldPostSimulationListener* v4;
    if (this) {
        v3 = static_cast<hkpPhantomListener*>(this);
    }
    else {
        v3 = nullptr;
    }
    world->addPhantomListener(v3);
    if (this) {
        v4 = static_cast<hkpWorldPostSimulationListener*>(this);
    }
    else {
        v4 = nullptr;
    }
    world->addWorldPostSimulationListener(v4);
    int32_t v5 = 0;
    if (world->m_phantoms.m_size > 0) {
        hkpPhantomListener* v6 = static_cast<hkpPhantomListener*>(this);
        do {
            v6->phantomAddedCallback(world->m_phantoms.m_data[v5++]);
        }
        while (v5 < world->m_phantoms.m_size);
    }
}

// The full object as this: the destructor's world loop calls it.
void hkpPhantomDisplayViewer::removeWorld(hkpWorld* world) {
    hkpPhantomListener* v3;
    hkpWorldPostSimulationListener* v4;
    if (this) {
        v3 = static_cast<hkpPhantomListener*>(this);
    }
    else {
        v3 = nullptr;
    }
    world->removePhantomListener(v3);
    if (this) {
        v4 = static_cast<hkpWorldPostSimulationListener*>(this);
    }
    else {
        v4 = nullptr;
    }
    world->removeWorldPostSimulationListener(v4);
    int32_t v5 = 0;
    if (world->m_phantoms.m_size > 0) {
        hkpPhantomListener* v6 = static_cast<hkpPhantomListener*>(this);
        do {
            v6->phantomRemovedCallback(world->m_phantoms.m_data[v5++]);
        }
        while (v5 < world->m_phantoms.m_size);
    }
}

hkProcess* hkpPhantomDisplayViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpPhantomDisplayViewer* v2 =
        static_cast<hkpPhantomDisplayViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpPhantomDisplayViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpPhantomDisplayViewer);
    hkpPhantomDisplayViewer* v3 = ::new (v2) hkpPhantomDisplayViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }

    return nullptr;
}

void hkpPhantomDisplayViewer::registerViewer() {
    hkpPhantomDisplayViewer::m_tag =
        hkProcessFactory::s_instance->registerProcess("Phantoms", &hkpPhantomDisplayViewer::create);
}

// The phantom types array goes with its member.
hkpUserShapePhantomTypeIdentifier::~hkpUserShapePhantomTypeIdentifier() {}

void hkpUserShapePhantomTypeIdentifier::registerShapePhantomType(hkpPhantomType t) {
    hkArray<hkpPhantomType, hkContainerHeapAllocator>* p_m_shapePhantomTypes = &m_shapePhantomTypes;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_shapePhantomTypes->m_size ==
        (p_m_shapePhantomTypes->m_capacityAndFlags & hkArrayBase<hkpPhantomType>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_shapePhantomTypes, sizeof(hkpPhantomType));
    }
    hkpPhantomType* v4 = &p_m_shapePhantomTypes->m_data[p_m_shapePhantomTypes->m_size];
    if (v4) {
        *v4 = t;
    }
    ++p_m_shapePhantomTypes->m_size;
}
