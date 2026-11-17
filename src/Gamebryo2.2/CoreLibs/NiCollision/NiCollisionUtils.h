#pragma once

class NiAVObject;
class NiPoint3;

// Gamebryo's collision helpers. Only some of the functions are declared.
namespace NiCollisionUtils {
    // Whether the ray from kOrigin along kDir meets pkObj's world bound.
    bool FindBoundIntersect(NiAVObject* pkObj, const NiPoint3& kOrigin, const NiPoint3& kDir);
    char IntersectTriangle(
        const NiPoint3* a1,
        const NiPoint3* a2,
        const NiPoint3* a3,
        const NiPoint3* a4,
        const float* a5,
        char a6,
        NiPoint3* a7,
        float* a8,
        float* a9,
        float* a10
    );
} // namespace NiCollisionUtils
