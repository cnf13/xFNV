#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The base of the motors that drive a constraint axis (position, velocity, spring damper, callback).
class hkpConstraintMotor : public hkReferencedObject {
public:
    enum MotorType : int32_t {
        TYPE_INVALID = 0x0,
        TYPE_POSITION = 0x1,
        TYPE_VELOCITY = 0x2,
        TYPE_SPRING_DAMPER = 0x3,
        TYPE_CALLBACK = 0x4,
        TYPE_MAX = 0x5,
    };

    // The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
    // classes' finish constructors call it): it keeps the fields the load wrote.
    hkpConstraintMotor() = default;
    hkpConstraintMotor(hkFinishLoadedObjectFlag f) : hkReferencedObject(f) {}
    ~hkpConstraintMotor() override;

    virtual hkpConstraintMotor* clone() const = 0;

    MotorType getType() const {
        return static_cast<MotorType>(m_type.m_storage);
    }

    hkEnum<MotorType, int8_t> m_type;
    uint8_t padding09[3];
};

// The class's reflection record.
extern hkClass hkpConstraintMotorClass;
