#pragma once

#include <stdint.h>

class hkpCollidable;
class hkpEntity;
struct hkpMoppCompilerPrimitive;
struct hkpRootCdPoint;
class hkpSimulationIsland;
struct hkpWorldRayCastOutput;
class hkShapeKeyPair;

namespace hkAlgorithm {

    // Orders two elements with their operator<. The result is Havok's hkBool32, a plain int.
    template <typename T>
    class less {
    public:
        int32_t operator()(const T& a, const T& b) {
            return a < b;
        }
    };

    // quickSortRecursive's comparison: the comparator's own result.
    template <typename L, typename A, typename B>
    inline decltype(auto) lessThan(L& cmpLess, A& a, B& b) {
        return cmpLess(a, b);
    }

    // Sorts pArr[d..h] in place (inclusive bounds): Hoare partition around the middle element, recursing on the left
    // part and looping on the right one.
    template <typename T, typename L>
    void quickSortRecursive(T* pArr, int32_t d, int32_t h, L cmpLess) {
        for (;;) {
            int32_t i = d;
            int32_t j = h;
            const T str = pArr[(d + h) >> 1];
            do {
                while (lessThan(cmpLess, pArr[i], str)) {
                    i++;
                }
                while (lessThan(cmpLess, str, pArr[j])) {
                    j--;
                }
                if (i > j) {
                    break;
                }
                if (i != j) {
                    T tmp = pArr[i];
                    pArr[i] = pArr[j];
                    pArr[j] = tmp;
                }
                i++;
                j--;
            }
            while (i <= j);
            if (d < j) {
                quickSortRecursive(pArr, d, j, cmpLess);
            }
            if (i >= h) {
                return;
            }
            d = i;
        }
    }
    // quickSortRecursive for the phantoms' collidables, ordering as hkpAabbPhantom::OrderByUid does: sorts
    // apArr[aiLow..aiHigh] in place (inclusive bounds) by broad-phase id. The comparator is an empty class, so aiLess
    // is a single byte standing in for it. Returns aiHigh. hkpSimpleShapePhantom's details (one collidable pointer
    // each) are sorted the same way; its overload is declared in hkpSimpleShapePhantom.h.
    int32_t quickSort(hkpCollidable** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

    // Sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by key A, then key B. The comparator is an empty class
    // whose value is passed on whole by the recursive call, so it is a word here. Returns the last right index, or the
    // last recursive call's result.
    int32_t quickSortRecursive(hkShapeKeyPair* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess);

    // quickSortRecursive for hkpMoppCompilerPrimitive with hkAlgorithm::less, using the primitive's operator< (its
    // extent's minimum): sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by the extents' minimum. The comparator
    // is an empty class, so it is passed as the byte aiLess, as quickSort's aiLess is, and handed on to the recursive
    // call. Defined in hkpMoppDefaultSplitter.cpp.
    void quickSortRecursive(hkpMoppCompilerPrimitive* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

    // quickSortRecursive<hkpWorldRayCastOutput, hkAlgorithm::less<hkpWorldRayCastOutput>>, with the hit's operator<
    // (its m_hitFraction): sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by hit fraction. The comparator (an
    // empty class) is passed as the byte aiLess, as quickSort's aiLess, and passed on to the recursive call: an empty
    // class argument carries no bytes in C++. Defined in hkpAllRayHitCollector.cpp.
    void quickSortRecursive(hkpWorldRayCastOutput* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

    // quickSortRecursive<hkReal, hkAlgorithm::less<hkReal>>: sorts apArr[aiLow..aiHigh] in place (inclusive bounds),
    // ascending. The comparator (an empty class) is passed as a byte, aiLess, passed on to the recursive call, as
    // quickSort's aiLess. Defined in hkaWavelet.cpp.
    void quickSortRecursive(float* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

} // namespace hkAlgorithm
