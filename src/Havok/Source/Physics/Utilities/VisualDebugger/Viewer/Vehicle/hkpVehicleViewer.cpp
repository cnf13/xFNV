#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Vehicle/hkpVehicleViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "SDK/Windows/winbase.h"

#include <xmmintrin.h>

char* getHkVehicleInstanceName() {
    return const_cast<char*>("hkpVehicleInstance");
}

hkpVehicleViewer::hkpVehicleViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts) {}

// Each watched world's removal takes the viewer's listeners off it; the vehicle array frees itself.
hkpVehicleViewer::~hkpVehicleViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; i++) {
            hkpVehicleViewer::worldRemovedCallback(m_context->m_worlds.m_data[i]);
        }
    }
}

void hkpVehicleViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        int32_t v3 = 0;
        if (m_context->m_worlds.m_size > 0) {
            hkpPhysicsContextWorldListener* v4 =
                &static_cast<hkpPhysicsContextWorldListener&>(static_cast<hkpWorldViewerBase&>(*this));
            do {
                worldAddedCallback(m_context->m_worlds.m_data[v3]);
                m_context = this->m_context;
                ++v3;
            }
            while (v3 < m_context->m_worlds.m_size);
        }
    }
}

void hkpVehicleViewer::worldAddedCallback(hkpWorld* w) {
    hkpActionListener* v4;
    hkpWorldPostSimulationListener* v5;
    hkpVehicleViewer* v3 = this;
    if (this == nullptr) {
        v4 = 0;
    }
    else {
        v4 = static_cast<hkpActionListener*>(this);
    }
    w->addActionListener(v4);
    if (v3) {
        v5 = static_cast<hkpWorldPostSimulationListener*>(this);
    }
    else {
        v5 = 0;
    }
    w->addWorldPostSimulationListener(v5);
    hkpPhysicsSystem* WorldAsOneSystem = w->getWorldAsOneSystem();
    int32_t v7 = 0;
    if (WorldAsOneSystem->m_actions.m_size > 0) {
        hkpActionListener* v8 = static_cast<hkpActionListener*>(this);
        do {
            v8->actionAddedCallback(WorldAsOneSystem->m_actions.m_data[v7++]);
        }
        while (v7 < WorldAsOneSystem->m_actions.m_size);
    }
    WorldAsOneSystem->removeReference();
}

void hkpVehicleViewer::worldRemovedCallback(hkpWorld* w) {
    hkpActionListener* v4;
    hkpVehicleViewer* v3 = this;
    if (this == nullptr) {
        v4 = 0;
    }
    else {
        v4 = static_cast<hkpActionListener*>(this);
    }
    w->removeActionListener(v4);
    if (v3) {
        w->removeWorldPostSimulationListener(this);
    }
    else {
        w->removeWorldPostSimulationListener(0);
    }
}

// Draws each vehicle's wheels (the suspension line, a spoke and a 12-sided rim) and a line along each axle through its
// wheels' centres. The wheel centres sit in a block of the thread's stack memory, taken and given back inline.
void hkpVehicleViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpVehicleViewer::step", nullptr);
    const int32_t numVehicles = m_vehicles.m_size;
    for (int32_t i = 0; i < numVehicles; ++i) {
        hkpVehicleInstance* vehicle = m_vehicles.m_data[i];
        const int32_t numWheels = vehicle->m_wheelsInfo.m_size;
        // EXE-INLINED none: hkAllocateStack<hkVector4>
        hkMemoryRouter* router = &hkMemoryRouter::getInstance();
        char* stackBlock = router->m_stack.m_next.m_storage;
        router->m_stack.m_next.m_storage =
            stackBlock + ((numWheels * static_cast<int32_t>(sizeof(hkVector4)) + 127) & ~127);
        // The stack allocator hands out untyped bytes; they are used as the centres' array.
        hkVector4* centers = reinterpret_cast<hkVector4*>(stackBlock);
        for (int32_t w = 0; w < numWheels; ++w) {
            const hkpVehicleData::WheelComponentParams& params = vehicle->m_data->m_wheelParams.m_data[w];
            const hkpVehicleInstance::WheelInfo& wheelInfo = vehicle->m_wheelsInfo.m_data[w];
            // The id is an int display id: the low 32 bits of the collidable address.
            const int32_t id = wheelInfo.m_contactBody
                ? static_cast<int32_t>(reinterpret_cast<hkUlong>(wheelInfo.m_contactBody->getCollidable()))
                : 0;
            m_displayHandler->displayLine(
                wheelInfo.m_contactPoint.m_position, wheelInfo.m_hardPointWs,
                static_cast<int32_t>(wheelInfo.m_contactBody ? hkColor::RED : hkColor::ROSYBROWN), id, m_tag
            );

            // The centre: the contact point moved up the suspension by the radius.
            // EXE-INLINED none: hkVector4::setAddMul4(const hkVector4&, const hkVector4&, const hkSimdReal&)
            const float negRadius = -params.m_radius;
            centers[w].m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_set1_ps(negRadius), wheelInfo.m_suspensionDirectionWs.m_quad),
                wheelInfo.m_contactPoint.m_position.m_quad
            );

            // The spoke: the suspension direction turned by the wheel's spin angle, scaled to the radius.
            hkQuaternion spin;
            spin.setAxisAngle(wheelInfo.m_spinAxisWs, -wheelInfo.m_spinAngle);
            hkVector4 spoke;
            spoke.setRotatedDir(spin, wheelInfo.m_suspensionDirectionWs);
            // EXE-INLINED 0x62AEA0
            {
                const hkQuadReal product = _mm_mul_ps(spoke.m_quad, spoke.m_quad);
                const hkQuadReal xySum = _mm_add_ss(_mm_shuffle_ps(product, product, _MM_SHUFFLE(1, 1, 1, 1)), product);
                const hkQuadReal len2 = _mm_add_ss(_mm_shuffle_ps(product, product, _MM_SHUFFLE(2, 2, 2, 2)), xySum);
                const hkQuadReal approx = _mm_rsqrt_ss(len2);
                const hkQuadReal len2EqualsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
                const hkQuadReal refined = _mm_mul_ss(
                    _mm_mul_ss(_mm_set_ss(0.5f), approx),
                    _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
                );
                const hkQuadReal lenInv = _mm_andnot_ps(len2EqualsZero, refined);
                spoke.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, _MM_SHUFFLE(0, 0, 0, 0)), spoke.m_quad);
            }
            // EXE-INLINED 0x627920
            spoke.m_quad = _mm_mul_ps(spoke.m_quad, _mm_set1_ps(params.m_radius));
            hkVector4 spokeStart;
            spokeStart.setAdd4(centers[w], spoke);
            hkVector4 spokeEnd;
            spokeEnd.setSub4(centers[w], spoke);
            m_displayHandler->displayLine(
                spokeStart, spokeEnd,
                static_cast<int32_t>(wheelInfo.m_contactBody ? hkColor::WHITESMOKE : hkColor::ROSYBROWN), id, m_tag
            );

            // The rim: twelve points 30 degrees apart around the centre, each joined to the one before it.
            hkVector4 rim[12];
            // EXE-INLINED 0x630AE0
            rim[0].m_quad = _mm_mul_ps(_mm_set1_ps(params.m_radius), wheelInfo.m_suspensionDirectionWs.m_quad);
            hkQuaternion step;
            step.setAxisAngle(wheelInfo.m_spinAxisWs, 0.5235988f);
            for (int32_t s = 1; s < 14; ++s) {
                if (s < 12) {
                    rim[s].setRotatedDir(step, rim[s - 1]);
                }
                if (s <= 12) {
                    rim[s - 1].add4(centers[w]);
                }
                if (s >= 2) {
                    m_displayHandler->displayLine(
                        rim[(s - 2) % 12], rim[(s - 1) % 12],
                        static_cast<int32_t>(wheelInfo.m_contactBody ? hkColor::WHITESMOKE : hkColor::ROSYBROWN), id,
                        m_tag
                    );
                }
            }
        }

        // Each axle: a line from each of its wheels' centres to the next one's, until all its wheels are found.
        const int32_t numAxles = vehicle->m_data->m_numWheelsPerAxle.m_size;
        for (int32_t axle = 0; axle < numAxles; ++axle) {
            bool first = true;
            const int32_t numWheelsOnAxle = vehicle->m_data->m_numWheelsPerAxle.m_data[axle];
            int32_t numFound = 0;
            // previous is not set from the current centre for the axle's first wheel; it is never read there, because
            // the first wheel draws nothing.
            hkVector4 previous;
            hkVector4 current;
            for (int32_t w = 0; w < numWheels; ++w) {
                const hkpVehicleInstance::WheelInfo& wheelInfo = vehicle->m_wheelsInfo.m_data[w];
                // The id is an int display id: the low 32 bits of the collidable address.
                const int32_t id = wheelInfo.m_contactBody
                    ? static_cast<int32_t>(reinterpret_cast<hkUlong>(wheelInfo.m_contactBody->getCollidable()))
                    : 0;
                if (vehicle->m_data->m_wheelParams.m_data[w].m_axle == axle) {
                    ++numFound;
                    if (first) {
                        current = centers[w];
                        first = false;
                    }
                    else {
                        previous = current;
                        current = centers[w];
                        m_displayHandler->displayLine(
                            previous, current, static_cast<int32_t>(hkColor::YELLOWGREEN), id, m_tag
                        );
                    }
                    if (numFound == numWheelsOnAxle) {
                        break;
                    }
                }
            }
        }
        // EXE-INLINED 0x5E0B60
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = stackBlock;
    }
    HK_TIMER_END();
}

void hkpVehicleViewer::actionAddedCallback(hkpAction* action) {
    hkBuiltinTypeRegistry* v2 = hkBuiltinTypeRegistry::s_instance;
    hkClassNameRegistry* v4 = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
    const hkClass* v5 = v4->getClassByName("hkpVehicleInstance");
    hkVtableClassRegistry* v6 = v2->getVtableClassRegistry();
    hkpVehicleInstance* v7 = static_cast<hkpVehicleInstance*>(action);
    const hkClass* v8 = v6->getClassFromVirtualInstance(action);
    if (v5) {
        if (v8) {
            hkBool isVehicle;
            if (v5->isSuperClass(&isVehicle, *v8)->m_bool) {
                v7->addReference();
                hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                if (m_vehicles.m_size ==
                    (m_vehicles.m_capacityAndFlags & hkArrayBase<hkpVehicleInstance*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*Instance->m_heap, &m_vehicles, sizeof(hkpVehicleInstance*));
                }
                m_vehicles.m_data[m_vehicles.m_size++] = v7;
            }
        }
    }
}

void hkpVehicleViewer::actionRemovedCallback(hkpAction* action) {
    int32_t m_size = m_vehicles.m_size;
    int32_t v3 = 0;
    if (m_size > 0) {
        for (hkpVehicleInstance** i = m_vehicles.begin(); *i != action; ++i) {
            if (++v3 >= m_size) {
                return;
            }
        }
        if (v3 != -1) {
            int32_t v5 = --m_vehicles.m_size;
            if (v5 != v3) {
                m_vehicles.m_data[v3] = m_vehicles.m_data[v5];
            }
            action->removeReference();
        }
    }
}

hkProcess* hkpVehicleViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpVehicleViewer* v2 = static_cast<hkpVehicleViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpVehicleViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpVehicleViewer);
    hkpVehicleViewer* v3 = ::new (v2) hkpVehicleViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }
    else {
        return 0;
    }
}

void hkpVehicleViewer::registerViewer() {
    hkpVehicleViewer::m_tag = hkProcessFactory::s_instance->registerProcess("Vehicles", &hkpVehicleViewer::create);
}
