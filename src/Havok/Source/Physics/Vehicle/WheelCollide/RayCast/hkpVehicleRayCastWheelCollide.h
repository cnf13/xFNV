#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/RejectChassisListener/hkpRejectChassisListener.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include <stdint.h>

class hkAabb;
class hkpAabbPhantom;
class hkpCollisionFilter;
struct hkpShapeRayCastCommand;

// A wheel collide that casts one ray per wheel along the suspension, in one AABB phantom around the wheels.
class hkpVehicleRayCastWheelCollide : public hkpVehicleWheelCollide {
public:
    hkpVehicleRayCastWheelCollide();

    hkpVehicleRayCastWheelCollide(hkFinishLoadedObjectFlag aFlag)
      : hkpVehicleWheelCollide(aFlag), m_rejectRayChassisListener(aFlag) {
        if (aFlag.m_finishing) {
            m_type.m_storage = RAY_CAST_WHEEL_COLLIDE;
        }
    }

    ~hkpVehicleRayCastWheelCollide() override;

    void init(const hkpVehicleInstance* vehicle) override;
    void collideWheels(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        CollisionDetectionWheelOutput* cdInfoOut
    ) override;
    void getPhantoms(hkArray<hkpPhantom*>& phantomsOut) override;
    void updateBeforeCollisionDetection(const hkpVehicleInstance* vehicle) override;
    hkpVehicleWheelCollide*
    clone(const hkpRigidBody& newChassis, const hkArray<hkpPhantom*>& newPhantoms) const override;
    void addToWorld(hkpWorld* world) override;
    void removeFromWorld() override;
    void setCollisionFilterInfo(uint32_t filterInfo) override;
    virtual int32_t buildRayCastCommands(
        const hkpVehicleInstance* vehicle,
        const hkpCollisionFilter* collisionFilter,
        int32_t filterSize,
        hkpShapeRayCastCommand* commandStorage,
        hkpWorldRayCastOutput* outputStorage
    ) const;
    virtual void calcWheelsAABB(const hkpVehicleInstance* vehicle, hkAabb& aabbOut) const;
    virtual void castSingleWheel(const hkpVehicleInstance::WheelInfo& wheelInfo, hkpWorldRayCastOutput& output) const;
    virtual void getCollisionOutputFromCastResult(
        const hkpVehicleInstance* vehicle,
        uint8_t wheelInfoNum,
        const hkpWorldRayCastOutput& rayCastOutput,
        CollisionDetectionWheelOutput& cdInfo
    ) const;
    virtual void getCollisionOutputWithoutHit(
        const hkpVehicleInstance* vehicle,
        uint8_t wheelNum,
        CollisionDetectionWheelOutput& cdInfo
    ) const;

    static const hkClass& staticClass();

    uint32_t m_wheelCollisionFilterInfo;
    hkpAabbPhantom* m_phantom;
    hkpRejectChassisListener m_rejectRayChassisListener;
};

extern hkClass hkpVehicleRayCastWheelCollideClass;
extern hkTypeInfo hkpVehicleRayCastWheelCollideTypeInfo;
