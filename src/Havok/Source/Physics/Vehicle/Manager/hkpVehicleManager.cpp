#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleManager.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

static void finishLoadedObjecthkpVehicleManager(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpVehicleManager(void* p);

// The reflected members of hkpVehicleManager.
using hkpVehicleManagerClass_MembersArray = const hkClassMember[1];
static hkpVehicleManagerClass_MembersArray hkpVehicleManagerClass_Members{
    {"registeredVehicles",
     &hkpVehicleInstanceClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x8,
     nullptr}
};
hkClass hkpVehicleManagerClass(
    "hkpVehicleManager",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleManagerClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleManagerTypeInfo = {
    "hkpVehicleManager",          finishLoadedObjecthkpVehicleManager, cleanupLoadedObjecthkpVehicleManager,
    getVtablehkpVehicleManager(), &typeid(hkpVehicleManager),          sizeof(hkpVehicleManager)
};

// The type info's finish-loaded-object callback for hkpVehicleManager: the null check, then its finish constructor.
static void finishLoadedObjecthkpVehicleManager(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleManager(kFlag);
    }
}

static void cleanupLoadedObjecthkpVehicleManager(void* p) {
    static_cast<hkpVehicleManager*>(p)->~hkpVehicleManager();
}

const void* getVtablehkpVehicleManager() {
    return hkGetVtableOf<hkpVehicleManager>();
}

// Releases the registered vehicles; the array frees itself.
hkpVehicleManager::~hkpVehicleManager() {
    for (hkpVehicleInstance* vehicle : m_registeredVehicles) {
        vehicle->removeReference();
    }
}

void hkpVehicleManager::addVehicle(hkpVehicleInstance* vehicle) {
    vehicle->addReference();
    hkArray<hkpVehicleInstance*, hkContainerHeapAllocator>* p_m_registeredVehicles = &m_registeredVehicles;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_registeredVehicles->m_size ==
        (p_m_registeredVehicles->m_capacityAndFlags & hkArrayBase<hkpVehicleInstance*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_registeredVehicles, sizeof(hkpVehicleInstance*));
    }
    p_m_registeredVehicles->m_data[p_m_registeredVehicles->m_size++] = vehicle;
}

void hkpVehicleManager::removeVehicle(hkpVehicleInstance* vehicle) {
    int32_t m_size = m_registeredVehicles.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    LABEL_5:;
        v3 = -1;
    }
    else {
        hkpVehicleInstance** m_data = m_registeredVehicles.begin();
        while (*m_data != vehicle) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto LABEL_5;
            }
        }
    }
    uint32_t v5 = --m_registeredVehicles.m_size;
    if (v5 != v3) {
        m_registeredVehicles.m_data[v3] = m_registeredVehicles.m_data[v5];
    }
    vehicle->removeReference();
}

// Steps each vehicle: its wheel collision results go into a local array of 16 outputs taken from the thread's stack
// allocator, sized to the vehicle's wheel count, then the vehicle is prepared, its wheels collided (collideWheels,
// through the vtable) and simulated.
void hkpVehicleManager::stepVehicles(const hkStepInfo& stepInfo) {
    hkLocalArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput> cdInfo(16);

    const int32_t numVehicles = m_registeredVehicles.m_size;
    for (int32_t i = 0; i < numVehicles; ++i) {
        hkpVehicleInstance* pVehicle = m_registeredVehicles.m_data[i];
        const int32_t numWheels = static_cast<uint8_t>(pVehicle->m_data->m_numWheels);
        // EXE-INLINED none: hkArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>::setSize
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        const int32_t capacity = static_cast<int32_t>(
            cdInfo.m_capacityAndFlags &
            hkArrayBase<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>::CAPACITY_MASK
        );
        if (capacity < numWheels) {
            int32_t request = 2 * capacity;
            if (numWheels >= request) {
                request = numWheels;
            }
            hkArrayUtil::_reserve(
                pHeap, &cdInfo, request, sizeof(hkpVehicleWheelCollide::CollisionDetectionWheelOutput)
            );
        }
        cdInfo.m_size = static_cast<uint32_t>(numWheels);
        pVehicle->updateBeforeCollisionDetection();
        pVehicle->m_wheelCollide->collideWheels(stepInfo.m_deltaTime, pVehicle, cdInfo.m_data);
        pVehicle->stepVehicleUsingWheelCollideOutput(stepInfo, cdInfo.m_data);
    }
}

void hkpVehicleManager::addToWorld(hkpWorld* a1) {
    int32_t m_size;
    int32_t i;
    hkpVehicleInstance* v5;
    m_size = m_registeredVehicles.m_size;
    for (i = 0; i < m_size; ++i) {
        v5 = m_registeredVehicles.m_data[i];
        v5->addToWorld(a1);
    }
}

void hkpVehicleManager::removeFromWorld() {
    int32_t m_size = m_registeredVehicles.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        hkpVehicleInstance* v4 = m_registeredVehicles.m_data[i];
        v4->removeFromWorld();
    }
}

const hkClass& hkpVehicleManager::staticClass() {
    return hkpVehicleManagerClass;
}
