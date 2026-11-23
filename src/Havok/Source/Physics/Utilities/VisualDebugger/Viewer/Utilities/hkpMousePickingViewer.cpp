#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Utilities/hkpMousePickingViewer.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Serialize/hkDisplaySerializeIStream.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

hkpMousePickingViewer::hkpMousePickingViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), m_currentWorld(nullptr), m_mouseSpring(nullptr),
    m_mouseSpringMaxRelativeForce(1000.0f) {}

hkpMousePickingViewer::~hkpMousePickingViewer() {
    releaseObject();
}

// The three command bytes getConsumableCommands hands out.
static uint8_t _cmds[] = {
    hkpMousePickingViewer::HK_PICK_OBJECT,
    hkpMousePickingViewer::HK_DRAG_OBJECT,
    hkpMousePickingViewer::HK_RELEASE_OBJECT
};

void hkpMousePickingViewer::getConsumableCommands(uint8_t*& commands, int32_t& numCommands) {
    commands = &_cmds[0];
    numCommands = 3;
}

void hkpMousePickingViewer::consumeCommand(uint8_t command) {
    switch (command) {
        case HK_PICK_OBJECT: {
            hkVector4 worldPosition;
            m_inStream->readQuadVector4(worldPosition);
            const uint64_t id = m_inStream->read64u();
            if (m_inStream->isOk().m_bool) {
                pickObject(id, worldPosition);
            }
            break;
        }
        case HK_DRAG_OBJECT: {
            hkVector4 newWorldSpacePoint;
            m_inStream->readQuadVector4(newWorldSpacePoint);
            if (m_inStream->isOk().m_bool) {
                dragObject(newWorldSpacePoint);
            }
            break;
        }
        case HK_RELEASE_OBJECT: {
            releaseObject();
            break;
        }
    }
}

void hkpMousePickingViewer::worldRemovedCallback(hkpWorld* world) {
    if (world == m_currentWorld) {
        releaseObject();
    }
}

void hkpMousePickingViewer::dragObject(const hkVector4& newWorldSpacePoint) {
    hkpMouseSpringAction* m_mouseSpring = this->m_mouseSpring;
    if (m_mouseSpring && m_mouseSpring->m_world) {
        m_mouseSpring->setMousePosition(newWorldSpacePoint);
    }
}

// The id is a collidable's address with its low two bits as a tag: 3 is a collidable, 0 is taken as one too, any
// other tag picks nothing.
hkBool hkpMousePickingViewer::pickObject(uint64_t id, const hkVector4& worldPosition) {
    uintptr_t address = static_cast<uintptr_t>(id);
    if ((id & 3) == 3) {
        address &= ~static_cast<uintptr_t>(3);
    }
    else if ((id & 3) != 0) {
        return false;
    }
    // The id is the collidable's address as an integer, and the collidable is read through it.
    hkpCollidable* collidable = reinterpret_cast<hkpCollidable*>(address);
    hkpRigidBody* rb = hkpGetRigidBody(collidable);
    if (rb) {
        hkpWorld* world = rb->m_world;
        m_currentWorld = world;
        if (rb->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED && rb->m_world == world) {
            hkVector4 positionInRbLocal;
            positionInRbLocal.setTransformedInversePos(rb->m_motion.m_motionState.m_transform, worldPosition);
            hkpMouseSpringAction* mouseSpring = new hkpMouseSpringAction(
                positionInRbLocal,
                worldPosition,
                0.5f,
                0.3f,
                0.95f,
                rb,
                nullptr
            );
            float maxRelativeForce = m_mouseSpringMaxRelativeForce;
            m_mouseSpring = mouseSpring;
            mouseSpring->setMaxRelativeForce(maxRelativeForce);
            m_currentWorld->addAction(m_mouseSpring);
        }
    }
    return true;
}

void hkpMousePickingViewer::releaseObject() {
    hkpMouseSpringAction* m_mouseSpring = this->m_mouseSpring;
    if (m_mouseSpring) {
        if (m_mouseSpring->m_world) {
            m_currentWorld->removeAction(m_mouseSpring);
            this->m_mouseSpring->m_entity->activate();
            m_mouseSpring = this->m_mouseSpring;
        }
        m_mouseSpring->removeReference();
        this->m_mouseSpring = nullptr;
    }
    m_currentWorld = nullptr;
}

hkProcess* hkpMousePickingViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpMousePickingViewer* pViewer = new hkpMousePickingViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpMousePickingViewer::registerViewer() {
    hkpMousePickingViewer::m_tag = hkProcessFactory::s_instance->registerProcess(
        "Mouse Picking",
        &hkpMousePickingViewer::create
    );
}
