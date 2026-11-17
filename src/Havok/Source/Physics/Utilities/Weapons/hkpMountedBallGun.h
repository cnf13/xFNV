#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpBallGun.h"

#include <stdint.h>

const void* getVtablehkpMountedBallGun();
void finishLoadedObjecthkpMountedBallGun(void* p, int32_t finishing);
void cleanupLoadedObjecthkpMountedBallGun(void* p);

// A ball gun mounted at m_position. It overrides only fireGun; the destructor is hkpBallGun's.
class hkpMountedBallGun : public hkpBallGun {
public:
    hkpMountedBallGun(int32_t numBullets);
    hkpMountedBallGun(hkFinishLoadedObjectFlag flag);

    // Slot 4.
    void fireGun(hkpWorld* world, const hkTransform& viewTransform) override;

    // The class's reflection record, hkpMountedBallGunClass.
    static const hkClass& staticClass();

    hkVector4 m_position;
};

extern hkClass hkpMountedBallGunClass;
extern hkTypeInfo hkpMountedBallGunTypeInfo;
