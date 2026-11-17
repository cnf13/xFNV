#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"

int32_t hkAlgorithm::quickSort(hkpCollidable** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    hkpAabbPhantom::OrderByUid cmpLess;
    int32_t d = aiLow;
    for (;;) {
        int32_t i = d;
        int32_t j = aiHigh;
        hkpCollidable* str = apArr[(d + j) >> 1];
        do {
            while (cmpLess(apArr[i], str)) {
                i++;
            }
            while (cmpLess(str, apArr[j])) {
                j--;
            }
            if (j < i) {
                break;
            }
            if (j != i) {
                hkpCollidable* tmp = apArr[j];
                apArr[j] = apArr[i];
                apArr[i] = tmp;
            }
            --j;
            ++i;
        }
        while (i <= j);
        if (d < j) {
            hkAlgorithm::quickSort(apArr, d, j, aiLess);
        }
        if (i >= aiHigh) {
            return aiHigh;
        }
        d = i;
    }
}
