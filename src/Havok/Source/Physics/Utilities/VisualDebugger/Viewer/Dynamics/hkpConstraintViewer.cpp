#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpConstraintViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpBallSocketDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintChainDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpHingeDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpHingeLimitsDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpLimitedHingeDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPointToPathDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPointToPlaneDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrismaticDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPulleyDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpRagdollDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpRagdollLimitsDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpStiffSpringDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpWheelDrawer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// Adds the viewer to the context's worlds as a post-simulation listener.
hkpConstraintViewer::hkpConstraintViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpWorldPostSimulationListener(), m_constraints() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->addWorldPostSimulationListener(this);
        }
    }
}

// Removes the viewer from the context's worlds; the constraint list goes with its member.
hkpConstraintViewer::~hkpConstraintViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->removeWorldPostSimulationListener(this);
        }
    }
}

// Every constraint each entity of the active, then the inactive islands is master of is drawn. The island is reloaded
// from the world's array for each entity.
void hkpConstraintViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpConstraintViewer", this);
    for (int32_t i = 0; i < world->m_activeSimulationIslands.m_size; ++i) {
        for (int32_t j = 0; j < world->m_activeSimulationIslands.m_data[i]->m_entities.m_size; ++j) {
            hkpEntity* masterEntity = world->m_activeSimulationIslands.m_data[i]->m_entities.m_data[j];
            for (int32_t k = 0; k < masterEntity->m_constraintsMaster.m_size; ++k) {
                draw(masterEntity->m_constraintsMaster.m_data[k].m_constraint, masterEntity, m_displayHandler);
            }
        }
    }
    for (int32_t i = 0; i < world->m_inactiveSimulationIslands.m_size; ++i) {
        for (int32_t j = 0; j < world->m_inactiveSimulationIslands.m_data[i]->m_entities.m_size; ++j) {
            hkpEntity* masterEntity = world->m_inactiveSimulationIslands.m_data[i]->m_entities.m_data[j];
            for (int32_t k = 0; k < masterEntity->m_constraintsMaster.m_size; ++k) {
                draw(masterEntity->m_constraintsMaster.m_data[k].m_constraint, masterEntity, m_displayHandler);
            }
        }
    }
    HK_TIMER_END();
}

void hkpConstraintViewer::worldAddedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->addWorldPostSimulationListener(nullptr);
    }
    else {
        world->addWorldPostSimulationListener(this);
    }
}

void hkpConstraintViewer::worldRemovedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->removeWorldPostSimulationListener(nullptr);
    }
    else {
        world->removeWorldPostSimulationListener(this);
    }
}

// Draws one constraint with the drawer for its data's type, at the viewer's scale; the id is the master entity's
// collidable. A breakable or malleable constraint is drawn as a temporary instance over the data it wraps.
void hkpConstraintViewer::draw(
    hkpConstraintInstance* constraint,
    const hkpEntity* masterEntity,
    hkDebugDisplayHandler* displayHandler
) {
    HK_TIMER_BEGIN("draw", nullptr);
    int32_t type = constraint->getData()->getType();
    hkReferencedObject::lockAll();
    // The id is the int display id: the low 32 bits of the collidable's address.
    const int32_t id = static_cast<int32_t>(reinterpret_cast<hkUlong>(masterEntity->getCollidable()));
    switch (type) {
        case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
            hkpBallSocketDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE: {
            hkpHingeDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            XFNV_INLINED_LOCAL(hkpLimitedHingeDrawer, drawer, std::nullopt, std::nullopt);
            ::new (&drawer) hkpLimitedHingeDrawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            XFNV_DESTRUCT(hkDisplayGeometry)(&drawer.m_angularLimit);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPATH: {
            hkpPointToPathDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC: {
            hkpPrismaticDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            hkpRagdollDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: {
            hkpStiffSpringDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_WHEEL: {
            hkpWheelDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE: {
            hkpBreakableConstraintData* breakable = static_cast<hkpBreakableConstraintData*>(constraint->getDataRw());
            hkpConstraintInstance instance(
                constraint->m_entities[0], constraint->m_entities[1], breakable->m_constraintData,
                hkpConstraintInstance::PRIORITY_PSI
            );
            draw(&instance, masterEntity, displayHandler);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE: {
            hkpMalleableConstraintData* malleable = static_cast<hkpMalleableConstraintData*>(constraint->getDataRw());
            hkpConstraintInstance instance(
                constraint->m_entities[0], constraint->m_entities[1], malleable->getWrappedConstraintData(),
                hkpConstraintInstance::PRIORITY_PSI
            );
            draw(&instance, masterEntity, displayHandler);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE: {
            hkpPointToPlaneDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_PULLEY: {
            hkpPulleyDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE_LIMITS: {
            hkpHingeLimitsDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            // The drawer's own destructor destroys the semicircle natively.
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL_LIMITS: {
            hkpRagdollLimitsDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_STIFF_SPRING_CHAIN:
        case hkpConstraintData::CONSTRAINT_TYPE_BALL_SOCKET_CHAIN:
        case hkpConstraintData::CONSTRAINT_TYPE_POWERED_CHAIN: {
            hkpConstraintChainDrawer drawer;
            drawer.setScale(m_scale);
            drawer.drawConstraint(constraint, displayHandler, id, m_tag);
            break;
        }
        default: break;
    }
    hkReferencedObject::unlockAll();
    HK_TIMER_END();
}

hkProcess* hkpConstraintViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpConstraintViewer* v2 =
        static_cast<hkpConstraintViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpConstraintViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpConstraintViewer);
    hkpConstraintViewer* v3 = ::new (v2) hkpConstraintViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }

    return nullptr;
}

void hkpConstraintViewer::registerViewer() {
    hkpConstraintViewer::m_tag =
        hkProcessFactory::s_instance->registerProcess("Constraints", &hkpConstraintViewer::create);
}

hkpRagdollDrawer::hkpRagdollDrawer()
  : hkpConstraintDrawer(), m_twistCone(), m_planeCone1(), m_planeCone2(), m_plane() {}

hkpLimitedHingeDrawer::hkpLimitedHingeDrawer() : hkpConstraintDrawer(), m_angularLimit() {}

hkpRagdollLimitsDrawer::hkpRagdollLimitsDrawer()
  : hkpConstraintDrawer(), m_twistCone(), m_planeCone1(), m_planeCone2(), m_plane() {}

// The plane and the cones go with their members.
hkpRagdollLimitsDrawer::~hkpRagdollLimitsDrawer() {}
