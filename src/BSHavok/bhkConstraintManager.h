#pragma once

#include <cstdint>

class hkConstraintCinfo;
class hkpConstraintData;
class NiCloningProcess;
class NiStream;

// The static helpers that create, clone, load and save the constraints by their type. It has no fields and no vtable.
class bhkConstraintManager {
public:
    // The class name of the constraint type.
    static const char* GetTypeName(uint32_t aiType);
    // The cinfo the constraint type needs, allocated and given its vtable by the type.
    static hkConstraintCinfo* GetInfo(uint32_t aiType);
    // A copy of apData: a cinfo of its type, filled from it, whose Havok data is returned.
    static hkpConstraintData* Clone(hkpConstraintData* apData, NiCloningProcess& arCloneData);
    // The constraint data whose cinfo comes next in the stream, as a new reference.
    static hkpConstraintData* LoadBinary(NiStream& arStream);
    // Writes apData's type and then the constraint data itself.
    static void SaveBinary(NiStream& arStream, hkpConstraintData* apData);
};
