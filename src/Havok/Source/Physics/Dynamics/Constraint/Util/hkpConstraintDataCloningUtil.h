#pragma once

class hkpConstraintData;

// Clones constraint data that holds motors (deepClone, cloneIfCanHaveMotors). Statics only.
class hkpConstraintDataCloningUtil {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static hkpConstraintData* deepClone(const hkpConstraintData* data);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static hkpConstraintData* cloneIfCanHaveMotors(const hkpConstraintData* data);
};
