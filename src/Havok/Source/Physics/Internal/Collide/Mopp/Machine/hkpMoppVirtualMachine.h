#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

// One primitive a MOPP query hits: its shape key. hkMoppBvTreeShapeBase::queryAabbImpl collects them in an
// hkArray<hkpMoppPrimitiveInfo>.
class hkpMoppPrimitiveInfo {
public:
    uint32_t ID;
};

// The base of the MOPP code interpreters: the array the queries append the hit primitives to. Not polymorphic.
class hkpMoppVirtualMachine {
public:
    hkArray<hkpMoppPrimitiveInfo>* m_primitives_out;
    int32_t padding[3];
};
