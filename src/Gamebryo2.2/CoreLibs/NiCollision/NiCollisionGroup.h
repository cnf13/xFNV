#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

#include <stdint.h>

class NiAVObject;

// Gamebryo's collision group: the colliders tested against the collidees. It holds two record arrays.
class NiCollisionGroup {
public:
    class Record;

    // What a collide callback is told of one collision: the two trees' roots, the two objects, the time, the point, the
    // two normals and the two triangles.
    class Intersect {
    public:
        NiAVObject* pkRoot0;
        NiAVObject* pkRoot1;
        NiAVObject* pkObj0;
        NiAVObject* pkObj1;
        float fTime;
        NiPoint3 kPoint;
        NiPoint3 kNormal0;
        NiPoint3 kNormal1;
        const NiPoint3** appkTri1;
        const NiPoint3** appkTri2;
    };

    // What a collide callback returns.
    enum {
        CONTINUE_COLLISIONS = 0,
        TERMINATE_COLLISIONS = 1,
        BREAKOUT_COLLISIONS = 2,
    };

    // The collide callback: what to do after the collision.
    typedef int32_t (*Callback)(Intersect& arIntersect);
    typedef NiTPrimitiveArray<Record*> NiRecordArray;

protected:
    NiRecordArray m_kColliders;
    NiRecordArray m_kCollidees;
};
