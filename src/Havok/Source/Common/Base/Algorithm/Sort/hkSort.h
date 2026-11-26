#pragma once

#include <stdint.h>

class hkpEntity;
struct hkpRootCdPoint;
class hkpSimulationIsland;

namespace hkAlgorithm {

    template <typename T>
    inline void swap(T& x, T& y) {
        T t(x);
        x = y;
        y = t;
    }

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
                    T tmp = pArr[j];
                    pArr[j] = pArr[i];
                    pArr[i] = tmp;
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

    // Sorts the iSize elements of pArr in place; fewer than two are left as they are.
    template <typename T, typename L>
    inline void quickSort(T* pArr, int32_t iSize, L cmpLess) {
        if (iSize > 1) {
            quickSortRecursive(pArr, 0, iSize - 1, cmpLess);
        }
    }

    template <typename T>
    inline void quickSort(T* pArr, int32_t iSize) {
        quickSort(pArr, iSize, less<T>());
    }

} // namespace hkAlgorithm
