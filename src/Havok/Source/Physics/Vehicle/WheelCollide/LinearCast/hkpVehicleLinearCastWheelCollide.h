#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/RejectChassisListener/hkpRejectChassisListener.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkAabb;
class hkpAabbPhantom;
class hkpCollidable;
class hkpCollisionFilter;
class hkpCylinderShape;
struct hkpPairLinearCastCommand;
struct hkpRootCdPoint;
class hkpShape;
class hkpVehicleInstance;

void cleanupLoadedObjecthkpVehicleLinearCastWheelCollide(void* p);

// A wheel collide that sweeps a shape (a cylinder) per wheel along the suspension, in an AABB phantom per wheel.
class hkpVehicleLinearCastWheelCollide : public hkpVehicleWheelCollide {
public:
    // The phantom and the swept shape of one wheel, with where the sweep starts and ends.
    struct WheelState {
        HK_DECLARE_POD_TYPE();

        static const hkClass& staticClass();

        hkpAabbPhantom* m_phantom;
        const hkpShape* m_shape;
        hkTransform m_transform;
        hkVector4 m_to;
    };

    hkpVehicleLinearCastWheelCollide();

    hkpVehicleLinearCastWheelCollide(hkFinishLoadedObjectFlag aFlag)
      : hkpVehicleWheelCollide(aFlag), m_wheelStates(aFlag), m_rejectChassisListener(aFlag) {
        if (aFlag.m_finishing) {
            m_type.m_storage = LINEAR_CAST_WHEEL_COLLIDE;
        }
    }

    ~hkpVehicleLinearCastWheelCollide() override;

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
    void wheelCollideCallback(
        const hkpVehicleInstance* vehicle,
        uint8_t wheelIndex,
        CollisionDetectionWheelOutput& cdInfo
    ) override;
    virtual int32_t getTotalNumCommands() const;
    virtual int32_t getNumCommands(uint8_t numWheel) const;
    virtual int32_t buildLinearCastCommands(
        const hkpVehicleInstance* vehicle,
        const hkpCollisionFilter* collisionFilter,
        hkpCollidable* collidableStorage,
        hkpPairLinearCastCommand* commandStorage,
        hkpRootCdPoint* outputStorage
    ) const;
    virtual const hkpRootCdPoint*
    determineNearestHit(uint8_t numWheel, const hkpPairLinearCastCommand* commandStorageOutput) const;
    virtual hkBool
    castSingleWheel(const hkpVehicleInstance* vehicle, uint8_t wheelNum, hkpRootCdPoint& linearCastOutput) const;
    virtual void getCollisionOutputFromCastResult(
        const hkpVehicleInstance* vehicle,
        uint8_t wheelNum,
        const hkpRootCdPoint& linearCastOutput,
        CollisionDetectionWheelOutput& output
    ) const;
    virtual void getCollisionOutputWithoutHit(
        const hkpVehicleInstance* vehicle,
        uint8_t wheelNum,
        CollisionDetectionWheelOutput& cdInfo
    ) const;

    // Havok SDK signature (hkpVehicle.lib, tied by reloc).
    void updateWheelState(const hkpVehicleInstance* vehicle, uint8_t wheelNum);
    // Havok SDK signature (hkpVehicle.lib, tied by reloc).
    void centerWheelContactPoint(
        const hkpVehicleInstance* vehicle,
        uint8_t wheelIndex,
        hkpVehicleWheelCollide::CollisionDetectionWheelOutput& cdInfo
    ) const;
    // Sizes m_wheelStates to the vehicle's wheels and references each wheel's shape.
    void setWheelShapes(const hkpVehicleInstance* vehicle, const hkArray<hkpShape*, hkContainerHeapAllocator>& shapes);
    // Havok SDK signature (hkpVehicle.lib, tied by reloc).
    hkpCylinderShape* createWheelShape(float width, float radius);
    // Havok SDK signature (hkpVehicle.lib, tied by reloc).
    void calcAabbOfWheel(const hkpVehicleInstance* vehicle, uint8_t wheelNum, hkAabb& aabbOut) const;

    static const hkClass& staticClass();

    uint32_t m_wheelCollisionFilterInfo;
    hkArray<WheelState> m_wheelStates;
    hkpRejectChassisListener m_rejectChassisListener;
    float m_maxExtraPenetration;
    float m_startPointTolerance;
};

extern hkClass hkpVehicleLinearCastWheelCollideWheelStateClass;
extern hkClass hkpVehicleLinearCastWheelCollideClass;
extern hkTypeInfo hkpVehicleLinearCastWheelCollideTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleLinearCastWheelCollideWheelStateTypeInfo;
