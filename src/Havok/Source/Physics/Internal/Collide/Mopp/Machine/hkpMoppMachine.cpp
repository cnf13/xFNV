#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppEarlyExitObbVirtualMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppKDopGeometriesVirtualMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifyVirtualMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppSphereVirtualMachine.h"

#include <xmmintrin.h>

void hkMoppModifyVirtualMachine_queryAabb(const hkpMoppCode* code, const hkAabb& aabb, hkpMoppModifier* modifierOut) {
    hkpMoppModifyVirtualMachine kMachine;
    kMachine.queryAabb(code, aabb, modifierOut);
}

// Walks the MOPP code from PC with the node's box as floats (aabb, narrowed in place: it is the caller's local) and
// asks the query object about the box of each child: a child it misses is dropped, one it includes whole it
// collects itself, one it hits is walked. query holds the code's integer offsets and shift; a rescale, a terminal
// offset or a property command moves it into scaledQuery first, which the recursive calls are given.
// KEEP: on a property command (96 to 107) the exe stores the value into scaledQuery's slot for that property
// (properties 1 to 3 land on m_offset_x to m_offset_z), reads scaledQuery.m_properties[0], copies the query into
// scaledQuery when query is not scaledQuery yet, and writes the value it read back. With the copy taken, that read
// is of a local nothing wrote yet, and the value goes on to the recursive calls and to the query object's
// collection in scaledQuery: src stores property 0 only.
template <class QueryType, class QueryObject>
void hkpMoppUsingFloatAabbVirtualMachine<QueryType, QueryObject>::queryMoppTree(
    const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
    const uint8_t* PC,
    hkpMoppTempAabb& fQueryL
) {
    hkpMoppUsingFloatAabbVirtualMachineQueryInt scaledQuery;
    // The right child's box at a split (the left child's is aabb).
    hkpMoppTempAabb rightAabb;
    while (true) {
        const int32_t command = *PC;
        // A split node: the offsets of the left and the right child from PC.
        int32_t leftOffset;
        int32_t rightOffset;
        int32_t terminal;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0: return;
            case 1:
            case 2:
            case 3:
            case 4:
                // Rescale: the node's space is finer by command bits.
                scaledQuery.m_offset_x = query->m_offset_x + (PC[1] << query->m_shift);
                scaledQuery.m_offset_y = query->m_offset_y + (PC[2] << query->m_shift);
                scaledQuery.m_offset_z = query->m_offset_z + (PC[3] << query->m_shift);
                scaledQuery.m_shift = query->m_shift - command;
                scaledQuery.m_properties[0] = query->m_properties[0];
                scaledQuery.m_primitiveOffset = query->m_primitiveOffset;
                query = &scaledQuery;
                PC += 4;
                continue;
            case 5: PC += PC[1] + 2; continue;
            case 6: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case 7: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case 9: {
                const int32_t offset = PC[1];
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                scaledQuery.m_primitiveOffset += offset;
                PC += 2;
                continue;
            }
            case 10: {
                const int32_t offset = (PC[1] << 8) + PC[2];
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                scaledQuery.m_primitiveOffset += offset;
                PC += 3;
                continue;
            }
            case 11: {
                const int32_t offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                scaledQuery.m_primitiveOffset = offset;
                PC += 5;
                continue;
            }
            case 16:
            case 17:
            case 18: {
                // A split along one axis, the right child an 8-bit offset away.
                const int32_t axis = command - 16;
                rightAabb = fQueryL;
                fQueryL.m_aabb.m_max.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[1] << query->m_shift)) * m_ItoFScale
                );
                rightAabb.m_aabb.m_min.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[2] << query->m_shift)) * m_ItoFScale
                );
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            }
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28: {
                // A split along two or three axes: both children are walked with the node's box.
                const int32_t offset = PC[3];
                rightAabb = fQueryL;
                PC += 4;
                queryMoppTree(query, PC, rightAabb);
                PC += offset;
                continue;
            }
            case 32:
            case 33:
            case 34: {
                // A single plane along one axis: the right child an 8-bit offset away.
                const int32_t axis = command - 32;
                rightAabb = fQueryL;
                rightAabb.m_aabb.m_min.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[1] << query->m_shift)) * m_ItoFScale
                );
                fQueryL.m_aabb.m_max.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + ((PC[1] + 1) << query->m_shift)) * m_ItoFScale
                );
                leftOffset = 0;
                rightOffset = PC[2];
                PC += 3;
                break;
            }
            case 35:
            case 36:
            case 37: {
                // A split along one axis with 16-bit offsets to both children.
                const int32_t axis = command - 35;
                rightAabb = fQueryL;
                fQueryL.m_aabb.m_max.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[1] << query->m_shift)) * m_ItoFScale
                );
                rightAabb.m_aabb.m_min.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[2] << query->m_shift)) * m_ItoFScale
                );
                leftOffset = (PC[3] << 8) + PC[4];
                rightOffset = (PC[5] << 8) + PC[6];
                PC += 7;
                break;
            }
            case 38:
            case 39:
            case 40: {
                // Cuts the box to the node's bounds along one axis.
                const int32_t axis = command - 38;
                fQueryL.m_aabb.m_min.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[1] << query->m_shift)) * m_ItoFScale
                );
                fQueryL.m_aabb.m_max.m_quad[axis] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[axis] + (PC[2] << query->m_shift)) * m_ItoFScale
                );
                PC += 3;
                goto testCut;
            }
            case 41:
            case 42:
            case 43: {
                // Cuts the box to 24-bit bounds of the unscaled space along one axis.
                const int32_t axis = command - 41;
                fQueryL.m_aabb.m_min.m_quad[axis] =
                    static_cast<float>(static_cast<double>((((PC[1] << 8) + PC[2]) << 8) + PC[3]) * m_ItoFScale);
                fQueryL.m_aabb.m_max.m_quad[axis] =
                    static_cast<float>(static_cast<double>((((PC[4] << 8) + PC[5]) << 8) + PC[6]) * m_ItoFScale);
                PC += 7;
            }
            testCut:
                // After the last cut the query object tests the box.
                if (*PC >= 38 && *PC <= 43) {
                    continue;
                }
                if (m_queryObject.checkCollision(fQueryL.m_aabb, query, PC) != hkpMoppQueryObject::HK_HIT) {
                    return;
                }
                continue;
            case 48:
            case 49:
            case 50:
            case 51:
            case 52:
            case 53:
            case 54:
            case 55:
            case 56:
            case 57:
            case 58:
            case 59:
            case 60:
            case 61:
            case 62:
            case 63:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
            case 71:
            case 72:
            case 73:
            case 74:
            case 75:
            case 76:
            case 77:
            case 78:
            case 79: terminal = command - 48; goto addTerminal;
            case 80: terminal = PC[1]; goto addTerminal;
            case 81: terminal = (PC[1] << 8) + PC[2]; goto addTerminal;
            case 82: terminal = (((PC[1] << 8) + PC[2]) << 8) + PC[3]; goto addTerminal;
            case 83:
                terminal = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
            addTerminal:
                {
                    hkArray<hkpMoppPrimitiveInfo>* primitives = m_primitives_out;
                    const uint32_t id = query->m_primitiveOffset + terminal;
                    primitives->pushBack(hkpMoppPrimitiveInfo{id});
                    return;
                }
            case 96:
            case 97:
            case 98:
            case 99:
                property = command - 96;
                propertyValue = PC[1];
                PC += 2;
                goto setProperty;
            case 100:
            case 101:
            case 102:
            case 103:
                property = command - 100;
                propertyValue = (PC[1] << 8) + PC[2];
                PC += 3;
                goto setProperty;
            case 104:
            case 105:
            case 106:
            case 107:
                property = command - 104;
                propertyValue = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                PC += 5;
            setProperty:
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                if (property == 0) {
                    scaledQuery.m_properties[0] = propertyValue;
                }
                continue;
            default: {
                // HK_ERROR; the same command is then read again.
                alignas(16) char buf[512];
                hkErrStream kStream(buf, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
                        "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Physics/Internal/Collide/Mopp/Machine/"
                        "hkpMoppUsingFloatAabbVirtualMachine.inl",
                        301
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
        // The splits along one axis (16 to 18, 32 to 37). The cuts that start each child narrow its box first.
        while (PC[leftOffset] >= 38 && PC[leftOffset] <= 43) {
            const int32_t cut = PC[leftOffset];
            if (cut >= 41) {
                fQueryL.m_aabb.m_min.m_quad[cut - 41] = static_cast<float>(
                    static_cast<double>((((PC[leftOffset + 1] << 8) + PC[leftOffset + 2]) << 8) + PC[leftOffset + 3]) *
                    m_ItoFScale
                );
                fQueryL.m_aabb.m_max.m_quad[cut - 41] = static_cast<float>(
                    static_cast<double>((((PC[leftOffset + 4] << 8) + PC[leftOffset + 5]) << 8) + PC[leftOffset + 6]) *
                    m_ItoFScale
                );
                leftOffset += 7;
            }
            else {
                fQueryL.m_aabb.m_min.m_quad[cut - 38] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[cut - 38] + (PC[leftOffset + 1] << query->m_shift)) *
                    m_ItoFScale
                );
                fQueryL.m_aabb.m_max.m_quad[cut - 38] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[cut - 38] + (PC[leftOffset + 2] << query->m_shift)) *
                    m_ItoFScale
                );
                leftOffset += 3;
            }
        }
        while (PC[rightOffset] >= 38 && PC[rightOffset] <= 43) {
            const int32_t cut = PC[rightOffset];
            if (cut >= 41) {
                rightAabb.m_aabb.m_min.m_quad[cut - 41] = static_cast<float>(
                    static_cast<double>(
                        (((PC[rightOffset + 1] << 8) + PC[rightOffset + 2]) << 8) + PC[rightOffset + 3]
                    ) *
                    m_ItoFScale
                );
                rightAabb.m_aabb.m_max.m_quad[cut - 41] = static_cast<float>(
                    static_cast<double>(
                        (((PC[rightOffset + 4] << 8) + PC[rightOffset + 5]) << 8) + PC[rightOffset + 6]
                    ) *
                    m_ItoFScale
                );
                rightOffset += 7;
            }
            else {
                rightAabb.m_aabb.m_min.m_quad[cut - 38] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[cut - 38] + (PC[rightOffset + 1] << query->m_shift)) *
                    m_ItoFScale
                );
                rightAabb.m_aabb.m_max.m_quad[cut - 38] = static_cast<float>(
                    static_cast<double>((&query->m_offset_x)[cut - 38] + (PC[rightOffset + 2] << query->m_shift)) *
                    m_ItoFScale
                );
                rightOffset += 3;
            }
        }
        // The right child is tested first.
        const hkpMoppQueryObject::hkpCheckCollisionResult rightResult =
            m_queryObject.checkCollision(rightAabb.m_aabb, query, PC + rightOffset);
        const hkpMoppQueryObject::hkpCheckCollisionResult leftResult =
            m_queryObject.checkCollision(fQueryL.m_aabb, query, PC + leftOffset);
        if (rightResult == hkpMoppQueryObject::HK_HIT) {
            if (leftResult == hkpMoppQueryObject::HK_HIT) {
                queryMoppTree(query, PC + rightOffset, rightAabb);
                PC += leftOffset;
            }
            else {
                fQueryL = rightAabb;
                PC += rightOffset;
            }
            continue;
        }
        if (leftResult == hkpMoppQueryObject::HK_HIT) {
            PC += leftOffset;
            continue;
        }
        return;
    }
}

template void hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput, hkMoppPlanesQueryObject>::queryMoppTree(
    const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
    const uint8_t* PC,
    hkpMoppTempAabb& aabb
);
using hkpMoppUsingFloatAabbVirtualMachine_lhkpMoppPlanesQueryInput_chkMoppPlanesQueryObject =
    hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput, hkMoppPlanesQueryObject>;

template void
hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput, hkMoppPlanesQueryObjectOptimized>::queryMoppTree(
    const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
    const uint8_t* PC,
    hkpMoppTempAabb& aabb
);
using hkpMoppUsingFloatAabbVirtualMachine_lhkpMoppPlanesQueryInput_chkMoppPlanesQueryObjectOptimized =
    hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput, hkMoppPlanesQueryObjectOptimized>;

template void hkpMoppUsingFloatAabbVirtualMachine<hkSphere, hkMoppSphereQueryObject>::queryMoppTree(
    const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
    const uint8_t* PC,
    hkpMoppTempAabb& aabb
);
using hkpMoppUsingFloatAabbVirtualMachine_lhkSphere_chkMoppSphereQueryObject =
    hkpMoppUsingFloatAabbVirtualMachine<hkSphere, hkMoppSphereQueryObject>;

template void hkpMoppUsingFloatAabbVirtualMachine<hkSphere, hkMoppSphereQueryObjectOptimized>::queryMoppTree(
    const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
    const uint8_t* PC,
    hkpMoppTempAabb& aabb
);
using hkpMoppUsingFloatAabbVirtualMachine_lhkSphere_chkMoppSphereQueryObjectOptimized =
    hkpMoppUsingFloatAabbVirtualMachine<hkSphere, hkMoppSphereQueryObjectOptimized>;

void sub_E5CEF0() {
    ;
}

void hkMoppSphereVirtualMachine_querySphere(
    const hkpMoppCode* code,
    const hkSphere& sphere,
    hkArray<hkpMoppPrimitiveInfo>* primitives_out
) {
    alignas(16) hkpMoppSphereVirtualMachine machine;
    machine.querySphere(code, sphere, primitives_out);
}

// Entry point: a DOP machine built and destroyed by its constructor and destructor runs the query.
void hkMoppKDopGeometriesVirtualMachine_query(
    const hkpMoppCode* code,
    const hkpMoppKDopQuery& query,
    hkpMoppInfo* kDopGeometries
) {
    hkpMoppKDopGeometriesVirtualMachine machine;
    machine.queryMopp(code, query, kDopGeometries);
}

// The SDK's entry point: a find-all machine walks the whole code from offset 0 with no properties.
void hkMoppFindAllVirtualMachine_getAllKeys(const hkpMoppCode* code, hkArray<hkpMoppPrimitiveInfo>* primitives_out) {
    hkpMoppFindAllVirtualMachine::hkpMoppFindAllVirtualMachineQuery scaledQuery;
    hkpMoppFindAllVirtualMachine machine;
    const uint8_t* pCommands = code->m_data.m_data;
    machine.m_primitives_out = primitives_out;
    scaledQuery.m_primitiveOffset = 0;
    scaledQuery.m_properties[0] = 0;
    machine.queryOnTree(&scaledQuery, pCommands);
}

// Entry point: a 16-aligned box machine on the stack runs the query.
void hkMoppObbVirtualMachine_queryAabb(
    const hkpMoppCode* code,
    const hkAabb& aabb,
    hkArray<hkpMoppPrimitiveInfo>* primitives_out
) {
    alignas(16) hkpMoppObbVirtualMachine machine;
    machine.queryAabb(code, aabb, primitives_out);
}

// The SDK's entry point: the planes, transposed four at a time into the query object's transforms (a last group of
// one to three planes padded with copies of the group's first plane, then its second), then, last transform first,
// each transform's absolute values and its distances moved into the code's space and doubled; then the walk over the
// whole code, the box from 0 to 2^24 code units. Partial hits go to the machine's array, fully included ones to the
// query object's.
void hkMoppUsingFloatAabbVirtualMachine_queryPlanes(
    const hkpMoppCode* code,
    const hkpMoppPlanesQueryInput& query,
    hkArray<hkpMoppPrimitiveInfo>* partialHitsOut,
    hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut
) {
    using Machine = hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput, hkMoppPlanesQueryObject>;
    Machine::hkpMoppUsingFloatAabbVirtualMachineQueryInt kQuery;
    Machine::hkpMoppTempAabb kAabb;
    Machine machine;
    const int32_t numPlanes = query.m_numPlanes;
    const hkVector4* pPlane = query.m_planes;
    const uint8_t* pCommands = code->m_data.m_data;
    machine.m_primitives_out = partialHitsOut;
    kQuery.m_primitiveOffset = 0;
    kQuery.m_properties[0] = 0;
    kQuery.m_offset_x = 0;
    kQuery.m_offset_y = 0;
    kQuery.m_offset_z = 0;
    kQuery.m_shift = 16;
    kAabb.m_aabb.m_min.m_quad = _mm_setzero_ps();
    machine.m_queryObject.m_numTransforms = 0;
    machine.m_ItoFScale = 1.0 / code->m_info.m_offset.m_quad[3];
    const float fMax = machine.m_ItoFScale * 16777216.0;
    kAabb.m_aabb.m_max.m_quad = _mm_set1_ps(fMax);
    hkTransform* pRotated = machine.m_queryObject.m_rotatedPlanes;
    int32_t numGrouped = 0;
    if ((numPlanes & ~3) > 0) {
        uint32_t numGroups = ((static_cast<uint32_t>(numPlanes & ~3) - 1) >> 2) + 1;
        numGrouped = static_cast<int32_t>(numGroups * 4);
        do {
            pRotated->m_rotation.m_col0.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[0], pPlane[1].m_quad[0], pPlane[2].m_quad[0], pPlane[3].m_quad[0]);
            pRotated->m_rotation.m_col1.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[1], pPlane[1].m_quad[1], pPlane[2].m_quad[1], pPlane[3].m_quad[1]);
            pRotated->m_rotation.m_col2.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[2], pPlane[1].m_quad[2], pPlane[2].m_quad[2], pPlane[3].m_quad[2]);
            pRotated->m_translation.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[3], pPlane[1].m_quad[3], pPlane[2].m_quad[3], pPlane[3].m_quad[3]);
            ++pRotated;
            pPlane += 4;
            ++machine.m_queryObject.m_numTransforms;
        }
        while (--numGroups);
    }
    const int32_t numLeft = numPlanes - numGrouped;
    if (numLeft == 1 || numLeft == 2 || numLeft == 3) {
        const hkVector4* pSecond = numLeft == 1 ? pPlane : pPlane + 1;
        const hkVector4* pThird = numLeft == 3 ? pPlane + 2 : pPlane;
        pRotated->m_rotation.m_col0.m_quad =
            _mm_setr_ps(pPlane->m_quad[0], pSecond->m_quad[0], pThird->m_quad[0], pPlane->m_quad[0]);
        pRotated->m_rotation.m_col1.m_quad =
            _mm_setr_ps(pPlane->m_quad[1], pSecond->m_quad[1], pThird->m_quad[1], pPlane->m_quad[1]);
        pRotated->m_rotation.m_col2.m_quad =
            _mm_setr_ps(pPlane->m_quad[2], pSecond->m_quad[2], pThird->m_quad[2], pPlane->m_quad[2]);
        pRotated->m_translation.m_quad =
            _mm_setr_ps(pPlane->m_quad[3], pSecond->m_quad[3], pThird->m_quad[3], pPlane->m_quad[3]);
        ++machine.m_queryObject.m_numTransforms;
    }
    for (int32_t i = machine.m_queryObject.m_numTransforms - 1; i >= 0; --i) {
        hkTransform& rotated = machine.m_queryObject.m_rotatedPlanes[i];
        hkTransform& absolute = machine.m_queryObject.m_absRotPlanes[i];
        absolute.m_rotation.m_col0.setAbs4(rotated.m_rotation.m_col0);
        absolute.m_rotation.m_col1.setAbs4(rotated.m_rotation.m_col1);
        absolute.m_rotation.m_col2.setAbs4(rotated.m_rotation.m_col2);
        absolute.m_translation.m_quad = _mm_setzero_ps();
        const __m128 offset = code->m_info.m_offset.m_quad;
        __m128 distance = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(offset, offset, 0x55), rotated.m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(offset, offset, 0x00), rotated.m_rotation.m_col0.m_quad)
        );
        distance =
            _mm_add_ps(distance, _mm_mul_ps(_mm_shuffle_ps(offset, offset, 0xAA), rotated.m_rotation.m_col2.m_quad));
        rotated.m_translation.m_quad = _mm_add_ps(distance, rotated.m_translation.m_quad);
        rotated.m_translation.m_quad = _mm_add_ps(rotated.m_translation.m_quad, rotated.m_translation.m_quad);
    }
    machine.m_queryObject.m_primitives_out = fullyIncludedHitsOut;
    machine.queryMoppTree(&kQuery, pCommands, kAabb);
}

// The SDK's entry point: hkMoppUsingFloatAabbVirtualMachine_queryPlanes with the optimized query object,
// whose walk also reports fully included boxes.
void hkMoppUsingFloatAabbVirtualMachine_queryPlanesOptimized(
    const hkpMoppCode* code,
    const hkpMoppPlanesQueryInput& query,
    hkArray<hkpMoppPrimitiveInfo>* partialHitsOut,
    hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut
) {
    using Machine = hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput, hkMoppPlanesQueryObjectOptimized>;
    Machine::hkpMoppUsingFloatAabbVirtualMachineQueryInt kQuery;
    Machine::hkpMoppTempAabb kAabb;
    Machine machine;
    const int32_t numPlanes = query.m_numPlanes;
    const hkVector4* pPlane = query.m_planes;
    const uint8_t* pCommands = code->m_data.m_data;
    machine.m_primitives_out = partialHitsOut;
    kQuery.m_primitiveOffset = 0;
    kQuery.m_properties[0] = 0;
    kQuery.m_offset_x = 0;
    kQuery.m_offset_y = 0;
    kQuery.m_offset_z = 0;
    kQuery.m_shift = 16;
    kAabb.m_aabb.m_min.m_quad = _mm_setzero_ps();
    machine.m_queryObject.m_numTransforms = 0;
    machine.m_ItoFScale = 1.0 / code->m_info.m_offset.m_quad[3];
    const float fMax = machine.m_ItoFScale * 16777216.0;
    kAabb.m_aabb.m_max.m_quad = _mm_set1_ps(fMax);
    hkTransform* pRotated = machine.m_queryObject.m_rotatedPlanes;
    int32_t numGrouped = 0;
    if ((numPlanes & ~3) > 0) {
        uint32_t numGroups = ((static_cast<uint32_t>(numPlanes & ~3) - 1) >> 2) + 1;
        numGrouped = static_cast<int32_t>(numGroups * 4);
        do {
            pRotated->m_rotation.m_col0.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[0], pPlane[1].m_quad[0], pPlane[2].m_quad[0], pPlane[3].m_quad[0]);
            pRotated->m_rotation.m_col1.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[1], pPlane[1].m_quad[1], pPlane[2].m_quad[1], pPlane[3].m_quad[1]);
            pRotated->m_rotation.m_col2.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[2], pPlane[1].m_quad[2], pPlane[2].m_quad[2], pPlane[3].m_quad[2]);
            pRotated->m_translation.m_quad =
                _mm_setr_ps(pPlane[0].m_quad[3], pPlane[1].m_quad[3], pPlane[2].m_quad[3], pPlane[3].m_quad[3]);
            ++pRotated;
            pPlane += 4;
            ++machine.m_queryObject.m_numTransforms;
        }
        while (--numGroups);
    }
    const int32_t numLeft = numPlanes - numGrouped;
    if (numLeft == 1 || numLeft == 2 || numLeft == 3) {
        const hkVector4* pSecond = numLeft == 1 ? pPlane : pPlane + 1;
        const hkVector4* pThird = numLeft == 3 ? pPlane + 2 : pPlane;
        pRotated->m_rotation.m_col0.m_quad =
            _mm_setr_ps(pPlane->m_quad[0], pSecond->m_quad[0], pThird->m_quad[0], pPlane->m_quad[0]);
        pRotated->m_rotation.m_col1.m_quad =
            _mm_setr_ps(pPlane->m_quad[1], pSecond->m_quad[1], pThird->m_quad[1], pPlane->m_quad[1]);
        pRotated->m_rotation.m_col2.m_quad =
            _mm_setr_ps(pPlane->m_quad[2], pSecond->m_quad[2], pThird->m_quad[2], pPlane->m_quad[2]);
        pRotated->m_translation.m_quad =
            _mm_setr_ps(pPlane->m_quad[3], pSecond->m_quad[3], pThird->m_quad[3], pPlane->m_quad[3]);
        ++machine.m_queryObject.m_numTransforms;
    }
    for (int32_t i = machine.m_queryObject.m_numTransforms - 1; i >= 0; --i) {
        hkTransform& rotated = machine.m_queryObject.m_rotatedPlanes[i];
        hkTransform& absolute = machine.m_queryObject.m_absRotPlanes[i];
        absolute.m_rotation.m_col0.setAbs4(rotated.m_rotation.m_col0);
        absolute.m_rotation.m_col1.setAbs4(rotated.m_rotation.m_col1);
        absolute.m_rotation.m_col2.setAbs4(rotated.m_rotation.m_col2);
        absolute.m_translation.m_quad = _mm_setzero_ps();
        const __m128 offset = code->m_info.m_offset.m_quad;
        __m128 distance = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(offset, offset, 0x55), rotated.m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(offset, offset, 0x00), rotated.m_rotation.m_col0.m_quad)
        );
        distance =
            _mm_add_ps(distance, _mm_mul_ps(_mm_shuffle_ps(offset, offset, 0xAA), rotated.m_rotation.m_col2.m_quad));
        rotated.m_translation.m_quad = _mm_add_ps(distance, rotated.m_translation.m_quad);
        rotated.m_translation.m_quad = _mm_add_ps(rotated.m_translation.m_quad, rotated.m_translation.m_quad);
    }
    machine.m_queryObject.m_primitives_out = fullyIncludedHitsOut;
    machine.queryMoppTree(&kQuery, pCommands, kAabb);
}

// Runs an early-exit box machine and returns whether the query hit anything.
int32_t hkMoppEarlyExitObbVirtualMachine_queryObb(
    const hkpMoppCode* code,
    const hkTransform& BvToWorld,
    const hkVector4& extent,
    const float& radius
) {
    alignas(16) hkpMoppEarlyExitObbVirtualMachine machine;
    return machine.queryObb(code, BvToWorld, extent, radius).m_bool != 0;
}

void hkMoppObbVirtualMachine_queryObb(
    const hkpMoppCode* code,
    const hkTransform& BvToWorld,
    const hkVector4& extent,
    float radius,
    hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitives_out
) {
    alignas(16) hkpMoppObbVirtualMachine v5;
    float fRadius = radius;
    v5.queryObb(code, BvToWorld, extent, fRadius, primitives_out);
}

// The SDK's entry point: the sphere moved into the code's space and doubled (centre and radius), then the walk over
// the whole code, the box from 0 to 2^24 code units. Partial hits go to the machine's array, fully included ones to
// the query object's.
void hkMoppUsingFloatAabbVirtualMachine_querySphere(
    const hkpMoppCode* code,
    const hkSphere& query,
    hkArray<hkpMoppPrimitiveInfo>* partialHitsOut,
    hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut
) {
    using Machine = hkpMoppUsingFloatAabbVirtualMachine<hkSphere, hkMoppSphereQueryObject>;
    Machine::hkpMoppUsingFloatAabbVirtualMachineQueryInt kQuery;
    Machine::hkpMoppTempAabb kAabb;
    Machine machine;
    const uint8_t* pCommands = code->m_data.m_data;
    const __m128 offset = code->m_info.m_offset.m_quad;
    kAabb.m_aabb.m_min.m_quad = _mm_setzero_ps();
    machine.m_primitives_out = partialHitsOut;
    kQuery.m_primitiveOffset = 0;
    kQuery.m_properties[0] = 0;
    kQuery.m_offset_x = 0;
    kQuery.m_offset_y = 0;
    kQuery.m_offset_z = 0;
    kQuery.m_shift = 16;
    machine.m_ItoFScale = 1.0 / code->m_info.m_offset.m_quad[3];
    const float fMax = machine.m_ItoFScale * 16777216.0;
    kAabb.m_aabb.m_max.m_quad = _mm_set1_ps(fMax);
    const float fRadius = query.m_pos.m_quad[3] + query.m_pos.m_quad[3];
    const __m128 centre = _mm_sub_ps(query.m_pos.m_quad, offset);
    machine.m_queryObject.m_sphere2.m_pos.m_quad = _mm_add_ps(centre, centre);
    machine.m_queryObject.m_sphere2.m_pos.m_quad[3] = fRadius;
    machine.m_queryObject.m_primitives_out = fullyIncludedHitsOut;
    machine.queryMoppTree(&kQuery, pCommands, kAabb);
}

// The SDK's entry point: hkMoppUsingFloatAabbVirtualMachine_querySphere with the optimized query object,
// whose walk also reports fully included boxes.
void hkMoppUsingFloatAabbVirtualMachine_querySphereOptimized(
    const hkpMoppCode* code,
    const hkSphere& query,
    hkArray<hkpMoppPrimitiveInfo>* partialHitsOut,
    hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut
) {
    using Machine = hkpMoppUsingFloatAabbVirtualMachine<hkSphere, hkMoppSphereQueryObjectOptimized>;
    Machine::hkpMoppUsingFloatAabbVirtualMachineQueryInt kQuery;
    Machine::hkpMoppTempAabb kAabb;
    Machine machine;
    const uint8_t* pCommands = code->m_data.m_data;
    const __m128 offset = code->m_info.m_offset.m_quad;
    kAabb.m_aabb.m_min.m_quad = _mm_setzero_ps();
    machine.m_primitives_out = partialHitsOut;
    kQuery.m_primitiveOffset = 0;
    kQuery.m_properties[0] = 0;
    kQuery.m_offset_x = 0;
    kQuery.m_offset_y = 0;
    kQuery.m_offset_z = 0;
    kQuery.m_shift = 16;
    machine.m_ItoFScale = 1.0 / code->m_info.m_offset.m_quad[3];
    const float fMax = machine.m_ItoFScale * 16777216.0;
    kAabb.m_aabb.m_max.m_quad = _mm_set1_ps(fMax);
    const float fRadius = query.m_pos.m_quad[3] + query.m_pos.m_quad[3];
    const __m128 centre = _mm_sub_ps(query.m_pos.m_quad, offset);
    machine.m_queryObject.m_sphere2.m_pos.m_quad = _mm_add_ps(centre, centre);
    machine.m_queryObject.m_sphere2.m_pos.m_quad[3] = fRadius;
    machine.m_queryObject.m_primitives_out = fullyIncludedHitsOut;
    machine.queryMoppTree(&kQuery, pCommands, kAabb);
}
