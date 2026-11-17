#pragma once

#include <cstdint>

class bhkWorld;

// One value of a world-operation walk's payload: a count, flag set, float or pointer, as the operation's handler
// reads it.
union UDATA {
    uint32_t i;
    float f;
    void* p;

    struct {
        uint16_t top;
        uint16_t bottom;
    } s;
};

namespace bhkWorldNS {

    enum OBJECTACTION : int32_t {
        OA_ADD = 0,
        OA_REMOVE = 1,
        OA_NI2HK = 2,
        OA_ENABLE = 3,
        OA_KILLHAVOK = 4,
        OA_MOTION = 5,
        OA_DEBUGDISPLAY = 6,
        OA_GROUP = 7,
        OA_QACTIVE = 8,
        OA_CHANGELAYER = 9,
        OA_FIXEDTOKEYFRAMED = 10,
        OA_SHIFT = 11,
        OA_ACTIVATE = 12,
        OA_FORCEADD = 13,
        OA_ENABLECOLLISION = 14,
        OA_SETRESTITUTION = 15,
        OA_SETFRICTION = 16,
        OA_DISABLE = 17,
        OA_TOTAL = 18,
    };

    // Payload passed through recursive world-object operations.
    struct ObjectRecData {
        bhkWorld* pWorld;
        bool bRecurse;
        uint8_t padding05[3];
        OBJECTACTION eAction;
        UDATA uDataA[4];
    };

} // namespace bhkWorldNS
