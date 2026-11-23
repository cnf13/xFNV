#pragma once

class NiAVObject;
class NiPoint3;

// Gamebryo's collision helpers. Only some of the functions are declared.
namespace NiCollisionUtils {
    // Whether the ray from kOrigin along kDir meets pkObj's world bound.
    bool FindBoundIntersect(NiAVObject* pkObj, const NiPoint3& kOrigin, const NiPoint3& kDir);
    char IntersectTriangle(
        const NiPoint3* kOrigin,
        const NiPoint3* kDir,
        const NiPoint3* kV1,
        const NiPoint3* kV2,
        const float* kV3,
        char bCull,
        NiPoint3* kIntersect,
        float* r,
        float* s,
        float* t
    );
} // namespace NiCollisionUtils
