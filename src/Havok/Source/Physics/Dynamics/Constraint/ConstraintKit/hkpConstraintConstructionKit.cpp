#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpConstraintConstructionKit.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintDataScheme.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

void hkpConstraintConstructionKit::begin(hkpGenericConstraintData* constraint) {
    m_scheme = constraint->getScheme();
    m_constraint = constraint;
    m_stiffnessReference = 0;
    m_dampingReference = 0;
    m_linearDofSpecifiedA[0u].m_bool = 0;
    m_linearDofSpecifiedA[1].m_bool = 0;
    m_linearDofSpecifiedA[2].m_bool = 0;
    m_linearDofSpecifiedB[0u].m_bool = 0;
    m_linearDofSpecifiedB[1].m_bool = 0;
    m_linearDofSpecifiedB[2].m_bool = 0;
    m_pivotSpecifiedA.m_bool = 0;
    m_pivotSpecifiedB.m_bool = 0;
    m_angularBasisSpecifiedA.m_bool = 0;
    m_angularBasisSpecifiedB.m_bool = 0;
}

int32_t hkpConstraintConstructionKit::setPivotA(const hkVector4& pivot) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 1;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v8 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v8->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = pivot.m_quad;
    m_pivotSpecifiedA.m_bool = 1;
    return m_size;
}

int32_t hkpConstraintConstructionKit::setPivotB(const hkVector4& pivot) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 2;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v8 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v8->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = pivot.m_quad;
    m_pivotSpecifiedB.m_bool = 1;
    return m_size;
}

int32_t hkpConstraintConstructionKit::setAngularBasisA(const hkMatrix3& dofBasis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 8;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v7->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++] = dofBasis.m_col0;
    hkArray<hkVector4, hkContainerHeapAllocator>* v8 = &this->m_scheme->m_data;
    hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
    if (v8->m_size == (v8->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v9->m_heap, v8, sizeof(hkVector4));
    }
    v8->m_data[v8->m_size++] = dofBasis.m_col1;
    hkArray<hkVector4, hkContainerHeapAllocator>* v10 = &this->m_scheme->m_data;
    hkMemoryRouter* v11 = &hkMemoryRouter::getInstance();
    if (v10->m_size == (v10->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v11->m_heap, v10, sizeof(hkVector4));
    }
    v10->m_data[v10->m_size++] = dofBasis.m_col2;
    m_angularBasisSpecifiedA.m_bool = 1;
    return m_size;
}

int32_t hkpConstraintConstructionKit::setAngularBasisB(const hkMatrix3& dofBasis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 9;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v7->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++] = dofBasis.m_col0;
    hkArray<hkVector4, hkContainerHeapAllocator>* v8 = &this->m_scheme->m_data;
    hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
    if (v8->m_size == (v8->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v9->m_heap, v8, sizeof(hkVector4));
    }
    v8->m_data[v8->m_size++] = dofBasis.m_col1;
    hkArray<hkVector4, hkContainerHeapAllocator>* v10 = &this->m_scheme->m_data;
    hkMemoryRouter* v11 = &hkMemoryRouter::getInstance();
    if (v10->m_size == (v10->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v11->m_heap, v10, sizeof(hkVector4));
    }
    v10->m_data[v10->m_size++] = dofBasis.m_col2;
    m_angularBasisSpecifiedB.m_bool = 1;
    return m_size;
}

void hkpConstraintConstructionKit::constrainAllLinearDof() {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 7;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    ++m_scheme->m_info.m_numSolverResults;
    ++m_scheme->m_info.m_numSolverElemTemps;
    m_scheme->m_info.m_sizeOfSchemas += 48;
    hkpGenericConstraintDataScheme* v5 = this->m_scheme;
    v5->m_info.m_sizeOfSchemas += 48;
    ++v5->m_info.m_numSolverResults;
    ++v5->m_info.m_numSolverElemTemps;
    hkpGenericConstraintDataScheme* v6 = this->m_scheme;
    v6->m_info.m_sizeOfSchemas += 48;
    ++v6->m_info.m_numSolverResults;
    ++v6->m_info.m_numSolverElemTemps;
}

void hkpConstraintConstructionKit::constrainAllAngularDof() {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 13;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    ++m_scheme->m_info.m_numSolverResults;
    ++m_scheme->m_info.m_numSolverElemTemps;
    m_scheme->m_info.m_sizeOfSchemas += 48;
    hkpGenericConstraintDataScheme* v5 = this->m_scheme;
    v5->m_info.m_sizeOfSchemas += 48;
    ++v5->m_info.m_numSolverResults;
    ++v5->m_info.m_numSolverElemTemps;
    hkpGenericConstraintDataScheme* v6 = this->m_scheme;
    v6->m_info.m_sizeOfSchemas += 48;
    ++v6->m_info.m_numSolverResults;
    ++v6->m_info.m_numSolverElemTemps;
}

int32_t hkpConstraintConstructionKit::setStrength(float strength) {
    __m128 v2 = _mm_undefined_ps();
    __m128 v3 = _mm_undefined_ps();
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    ++m_stiffnessReference;
    hkArray<int32_t>* p_m_commands = &m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    v2[0] = 0.0f;
    v3[0] = 0.0f;
    p_m_commands->m_data[p_m_commands->m_size++] = hkpGenericConstraintDataScheme::e_setStrength;
    hkpGenericConstraintDataScheme* v8 = this->m_scheme;
    int32_t m_size = v8->m_data.m_size;
    __m128 v12 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(strength))), v3),
        _mm_unpacklo_ps(v2, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
    if (v8->m_data.m_size == (v8->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v10->m_heap, &v8->m_data, sizeof(hkVector4));
    }
    v8->m_data.m_data[v8->m_data.m_size++].m_quad = v12;
    return m_size;
}

void hkpConstraintConstructionKit::restoreStrength() {
    --m_stiffnessReference;
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 23;
}

void hkpConstraintConstructionKit::end() {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 0;
}

// The cone limit command and its axis go into the scheme's commands, then the angle's cosine, 100.0f and two zero lanes
// into its data; the scheme is charged 0x30 schema bytes and one solver result and element temporary. Returns the data
// index the vector was stored at, read before the append. m_scheme is reloaded before every append and the router is
// fetched before each capacity test, and the axis index is truncated to a byte though the parameter is an int.
int32_t hkpConstraintConstructionKit::setConeLimit(int32_t axis, float angle) {
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rCommandRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rCommandRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = hkpGenericConstraintDataScheme::e_setConeLimit;
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(axis);
    m_scheme->m_commands.m_size += 1;
    // The CRT's cosine, rounded to float.
    const float fCos = static_cast<float>(std::cos(angle));
    const int32_t iDataIndex = m_scheme->m_data.m_size;
    hkVector4 kData;
    kData.m_quad[0] = fCos;
    kData.m_quad[1] = 100.0f;
    kData.m_quad[2] = 0.0f;
    kData.m_quad[3] = 0.0f;
    // EXE-INLINED none: hkArray<hkVector4>::pushBack
    hkMemoryRouter& rDataRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_data.m_size == (m_scheme->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rDataRouter.m_heap, &m_scheme->m_data, sizeof(hkVector4));
    }
    m_scheme->m_data.m_data[m_scheme->m_data.m_size] = kData;
    m_scheme->m_data.m_size += 1;
    m_scheme->m_info.add(0x30, 1, 1);
    return iDataIndex;
}

// The twist limit command and its two angular basis axes go into the scheme's commands, then the sines of the limit's
// lower and upper angle and two zero lanes into its data; the scheme is charged 0x30 schema bytes and one solver result
// and element temporary. Returns the data index the vector was stored at, read before the append. As in setConeLimit,
// each axis index is truncated to a byte.
int32_t hkpConstraintConstructionKit::setTwistLimit(int32_t twistAxis, int32_t planeAxis, float min, float max) {
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rCommandRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rCommandRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = hkpGenericConstraintDataScheme::e_setTwistLimit;
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rTwistAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rTwistAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(twistAxis);
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rPlaneAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rPlaneAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(planeAxis);
    m_scheme->m_commands.m_size += 1;
    // The upper angle's sine is computed first; both come from the CRT's std::sin, rounded to float.
    const float fSinMax = static_cast<float>(std::sin(max));
    const float fSinMin = static_cast<float>(std::sin(min));
    const int32_t iDataIndex = m_scheme->m_data.m_size;
    hkVector4 kData;
    kData.m_quad[0] = fSinMin;
    kData.m_quad[1] = fSinMax;
    kData.m_quad[2] = 0.0f;
    kData.m_quad[3] = 0.0f;
    // EXE-INLINED none: hkArray<hkVector4>::pushBack
    hkMemoryRouter& rDataRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_data.m_size == (m_scheme->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rDataRouter.m_heap, &m_scheme->m_data, sizeof(hkVector4));
    }
    m_scheme->m_data.m_data[m_scheme->m_data.m_size] = kData;
    m_scheme->m_data.m_size += 1;
    m_scheme->m_info.add(0x30, 1, 1);
    return iDataIndex;
}

int32_t hkpConstraintConstructionKit::setLinearDofA(const hkVector4& dof, int32_t axis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 3;
    hkArray<int32_t, hkContainerHeapAllocator>* v6 = &this->m_scheme->m_commands;
    hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
    if (v6->m_size == (v6->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v7->m_heap, v6, sizeof(int32_t));
    }
    v6->m_data[v6->m_size++] = axis;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v10->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = dof.m_quad;
    m_linearDofSpecifiedA[axis].m_bool = 1;
    return m_size;
}

int32_t hkpConstraintConstructionKit::setLinearDofB(const hkVector4& dof, int32_t axis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 4;
    hkArray<int32_t, hkContainerHeapAllocator>* v6 = &this->m_scheme->m_commands;
    hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
    if (v6->m_size == (v6->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v7->m_heap, v6, sizeof(int32_t));
    }
    v6->m_data[v6->m_size++] = axis;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v10->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = dof.m_quad;
    m_linearDofSpecifiedB[axis].m_bool = 1;
    return m_size;
}

int32_t hkpConstraintConstructionKit::setLinearDofWorld(const hkVector4& dof, int32_t axis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 5;
    hkArray<int32_t, hkContainerHeapAllocator>* v6 = &this->m_scheme->m_commands;
    hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
    if (v6->m_size == (v6->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v7->m_heap, v6, sizeof(int32_t));
    }
    v6->m_data[v6->m_size++] = axis;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v10->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = dof.m_quad;
    m_linearDofSpecifiedA[axis].m_bool = 1;
    m_linearDofSpecifiedB[axis].m_bool = 1;
    return m_size;
}

void hkpConstraintConstructionKit::constrainLinearDof(int32_t axis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 6;
    hkArray<int32_t, hkContainerHeapAllocator>* v5 = &this->m_scheme->m_commands;
    hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
    if (v5->m_size == (v5->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v6->m_heap, v5, sizeof(int32_t));
    }
    v5->m_data[v5->m_size++] = axis;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    ++m_scheme->m_info.m_numSolverResults;
    ++m_scheme->m_info.m_numSolverElemTemps;
    m_scheme->m_info.m_sizeOfSchemas += 48;
}

void hkpConstraintConstructionKit::setAngularBasisABodyFrame() {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 10;
    m_angularBasisSpecifiedA.m_bool = 1;
}

void hkpConstraintConstructionKit::setAngularBasisBBodyFrame() {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 11;
    m_angularBasisSpecifiedB.m_bool = 1;
}

void hkpConstraintConstructionKit::constrainToAngularDof(int32_t axis) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 12;
    hkArray<int32_t, hkContainerHeapAllocator>* v5 = &this->m_scheme->m_commands;
    hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
    if (v5->m_size == (v5->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v6->m_heap, v5, sizeof(int32_t));
    }
    v5->m_data[v5->m_size++] = axis;
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    ++m_scheme->m_info.m_numSolverResults;
    ++m_scheme->m_info.m_numSolverElemTemps;
    m_scheme->m_info.m_sizeOfSchemas += 48;
    hkpGenericConstraintDataScheme* v8 = this->m_scheme;
    ++v8->m_info.m_numSolverResults;
    ++v8->m_info.m_numSolverElemTemps;
    v8->m_info.m_sizeOfSchemas += 48;
}

// The linear limit command and its linear axis go into the scheme's commands, then the limit's minimum and maximum and
// two zero lanes into its data; the scheme is charged 0x40 schema bytes and one solver result and element temporary.
// Returns the data index the vector was stored at, read before the append. As in setConeLimit, m_scheme is reloaded
// before each command append, the router is fetched before each capacity test and the axis is truncated to a byte.
int32_t hkpConstraintConstructionKit::setLinearLimit(int32_t axis, float min, float max) {
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rCommandRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rCommandRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = hkpGenericConstraintDataScheme::e_setLinearLimit;
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(axis);
    m_scheme->m_commands.m_size += 1;
    hkpGenericConstraintDataScheme* pScheme = m_scheme;
    const int32_t iDataIndex = pScheme->m_data.m_size;
    hkVector4 kData;
    kData.m_quad[0] = min;
    kData.m_quad[1] = max;
    kData.m_quad[2] = 0.0f;
    kData.m_quad[3] = 0.0f;
    // EXE-INLINED none: hkArray<hkVector4>::pushBack
    hkMemoryRouter& rDataRouter = hkMemoryRouter::getInstance();
    if (pScheme->m_data.m_size == (pScheme->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rDataRouter.m_heap, &pScheme->m_data, sizeof(hkVector4));
    }
    pScheme->m_data.m_data[pScheme->m_data.m_size] = kData;
    pScheme->m_data.m_size += 1;
    hkpGenericConstraintDataScheme* pInfoScheme = m_scheme;
    pInfoScheme->m_info.m_sizeOfSchemas += 64;
    ++pInfoScheme->m_info.m_numSolverResults;
    ++pInfoScheme->m_info.m_numSolverElemTemps;
    return iDataIndex;
}

int32_t hkpConstraintConstructionKit::setAngularLimit(int32_t axis, float min, float max) {
    __m128 v4 = _mm_undefined_ps();
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 17;
    hkArray<int32_t, hkContainerHeapAllocator>* v8 = &this->m_scheme->m_commands;
    hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
    if (v8->m_size == (v8->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v9->m_heap, v8, sizeof(int32_t));
    }
    v4[0] = 0.0f;
    v8->m_data[v8->m_size++] = static_cast<uint8_t>(axis);
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    __m128 v16 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(min))), v4),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(max))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v13 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v13->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = v16;
    hkpGenericConstraintDataScheme* v14 = this->m_scheme;
    v14->m_info.m_sizeOfSchemas += 48;
    ++v14->m_info.m_numSolverResults;
    ++v14->m_info.m_numSolverElemTemps;
    return m_size;
}

// The angular motor command and its angular basis axis go into the scheme's commands, then the index the motor will
// have in the scheme's motors (as a float) and three zero lanes into its data; the motor is referenced and appended to
// the motors; the scheme is charged 0x40 schema bytes, one solver result and three element temporaries. Returns the
// data index the vector was stored at, read before the append. The axis is truncated to a byte.
int32_t hkpConstraintConstructionKit::setAngularMotor(int32_t axis, hkpConstraintMotor* motor) {
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rCommandRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rCommandRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = hkpGenericConstraintDataScheme::e_setAngularMotor;
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(axis);
    m_scheme->m_commands.m_size += 1;
    hkpGenericConstraintDataScheme* pScheme = m_scheme;
    hkVector4 kData;
    kData.m_quad[0] = static_cast<float>(pScheme->m_motors.m_size);
    kData.m_quad[1] = 0.0f;
    kData.m_quad[2] = 0.0f;
    kData.m_quad[3] = 0.0f;
    const int32_t iDataIndex = pScheme->m_data.m_size;
    motor->addReference();
    // EXE-INLINED none: hkArray<hkVector4>::pushBack
    hkMemoryRouter& rDataRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_data.m_size == (m_scheme->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rDataRouter.m_heap, &m_scheme->m_data, sizeof(hkVector4));
    }
    m_scheme->m_data.m_data[m_scheme->m_data.m_size] = kData;
    m_scheme->m_data.m_size += 1;
    // EXE-INLINED none: hkArray<hkpConstraintMotor*>::pushBack
    hkMemoryRouter& rMotorRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_motors.m_size ==
        (m_scheme->m_motors.m_capacityAndFlags & hkArrayBase<hkpConstraintMotor*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rMotorRouter.m_heap, &m_scheme->m_motors, sizeof(hkpConstraintMotor*));
    }
    m_scheme->m_motors.m_data[m_scheme->m_motors.m_size] = motor;
    m_scheme->m_motors.m_size += 1;
    hkpGenericConstraintDataScheme* pInfoScheme = m_scheme;
    ++pInfoScheme->m_info.m_numSolverResults;
    pInfoScheme->m_info.m_sizeOfSchemas += 64;
    pInfoScheme->m_info.m_numSolverElemTemps += 3;
    return iDataIndex;
}

// The linear motor command, as setAngularMotor, charged 0x50 schema bytes.
int32_t hkpConstraintConstructionKit::setLinearMotor(int32_t axis, hkpConstraintMotor* motor) {
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rCommandRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rCommandRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = hkpGenericConstraintDataScheme::e_setLinearMotor;
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(axis);
    m_scheme->m_commands.m_size += 1;
    hkpGenericConstraintDataScheme* pScheme = m_scheme;
    hkVector4 kData;
    kData.m_quad[0] = static_cast<float>(pScheme->m_motors.m_size);
    kData.m_quad[1] = 0.0f;
    kData.m_quad[2] = 0.0f;
    kData.m_quad[3] = 0.0f;
    const int32_t iDataIndex = pScheme->m_data.m_size;
    motor->addReference();
    // EXE-INLINED none: hkArray<hkVector4>::pushBack
    hkMemoryRouter& rDataRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_data.m_size == (m_scheme->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rDataRouter.m_heap, &m_scheme->m_data, sizeof(hkVector4));
    }
    m_scheme->m_data.m_data[m_scheme->m_data.m_size] = kData;
    m_scheme->m_data.m_size += 1;
    // EXE-INLINED none: hkArray<hkpConstraintMotor*>::pushBack
    hkMemoryRouter& rMotorRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_motors.m_size ==
        (m_scheme->m_motors.m_capacityAndFlags & hkArrayBase<hkpConstraintMotor*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rMotorRouter.m_heap, &m_scheme->m_motors, sizeof(hkpConstraintMotor*));
    }
    m_scheme->m_motors.m_data[m_scheme->m_motors.m_size] = motor;
    m_scheme->m_motors.m_size += 1;
    hkpGenericConstraintDataScheme* pInfoScheme = m_scheme;
    ++pInfoScheme->m_info.m_numSolverResults;
    pInfoScheme->m_info.m_sizeOfSchemas += 80;
    pInfoScheme->m_info.m_numSolverElemTemps += 3;
    return iDataIndex;
}

// The angular friction command and its angular basis axis go into the scheme's commands, then the maximum impulse
// and three zero lanes into its data; the scheme is charged 0x30 schema bytes, one solver result and two element
// temporaries. Returns the data index the vector was stored at, read before the append.
int32_t hkpConstraintConstructionKit::setAngularFriction(int32_t axis, float maxImpulse) {
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rCommandRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rCommandRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = hkpGenericConstraintDataScheme::e_setAngularFriction;
    m_scheme->m_commands.m_size += 1;
    // EXE-INLINED none: hkArray<int32_t>::pushBack
    hkMemoryRouter& rAxisRouter = hkMemoryRouter::getInstance();
    if (m_scheme->m_commands.m_size ==
        (m_scheme->m_commands.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rAxisRouter.m_heap, &m_scheme->m_commands, sizeof(int32_t));
    }
    m_scheme->m_commands.m_data[m_scheme->m_commands.m_size] = static_cast<uint8_t>(axis);
    m_scheme->m_commands.m_size += 1;
    hkpGenericConstraintDataScheme* pScheme = m_scheme;
    const int32_t iDataIndex = pScheme->m_data.m_size;
    hkVector4 kData;
    kData.m_quad[0] = maxImpulse;
    kData.m_quad[1] = 0.0f;
    kData.m_quad[2] = 0.0f;
    kData.m_quad[3] = 0.0f;
    // EXE-INLINED none: hkArray<hkVector4>::pushBack
    hkMemoryRouter& rDataRouter = hkMemoryRouter::getInstance();
    if (pScheme->m_data.m_size == (pScheme->m_data.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rDataRouter.m_heap, &pScheme->m_data, sizeof(hkVector4));
    }
    pScheme->m_data.m_data[pScheme->m_data.m_size] = kData;
    pScheme->m_data.m_size += 1;
    hkpGenericConstraintDataScheme* pInfoScheme = m_scheme;
    pInfoScheme->m_info.m_sizeOfSchemas += 48;
    ++pInfoScheme->m_info.m_numSolverResults;
    pInfoScheme->m_info.m_numSolverElemTemps += 2;
    return iDataIndex;
}

int32_t hkpConstraintConstructionKit::setLinearFriction(int32_t axis, float maxImpulse) {
    __m128 v3 = _mm_undefined_ps();
    __m128 v4 = _mm_undefined_ps();
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &this->m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 21;
    hkArray<int32_t, hkContainerHeapAllocator>* v8 = &this->m_scheme->m_commands;
    hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
    if (v8->m_size == (v8->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v9->m_heap, v8, sizeof(int32_t));
    }
    v3[0] = 0.0f;
    v4[0] = 0.0f;
    v8->m_data[v8->m_size++] = static_cast<uint8_t>(axis);
    hkpGenericConstraintDataScheme* m_scheme = this->m_scheme;
    int32_t m_size = m_scheme->m_data.m_size;
    __m128 v16 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(maxImpulse))), v4),
        _mm_unpacklo_ps(v3, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    hkArray<hkVector4, hkContainerHeapAllocator>* p_m_data = &m_scheme->m_data;
    hkMemoryRouter* v13 = &hkMemoryRouter::getInstance();
    if (p_m_data->m_size == (p_m_data->m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v13->m_heap, p_m_data, sizeof(hkVector4));
    }
    p_m_data->m_data[p_m_data->m_size++].m_quad = v16;
    hkpGenericConstraintDataScheme* v14 = this->m_scheme;
    v14->m_info.m_sizeOfSchemas += 64;
    ++v14->m_info.m_numSolverResults;
    v14->m_info.m_numSolverElemTemps += 2;
    return m_size;
}

void hkpConstraintConstructionKit::addConstraintModifierCallback(hkpConstraintModifier* cm, int32_t userData) {
    hkArray<int32_t, hkContainerHeapAllocator>* p_m_commands = &m_scheme->m_commands;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_commands->m_size == (p_m_commands->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_commands, sizeof(int32_t));
    }
    p_m_commands->m_data[p_m_commands->m_size++] = 24;
    hkArray<hkpConstraintModifier*, hkContainerHeapAllocator>* p_m_modifiers = &m_scheme->m_modifiers;
    hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
    if (p_m_modifiers->m_size ==
        (p_m_modifiers->m_capacityAndFlags & hkArrayBase<hkpConstraintModifier*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v7->m_heap, p_m_modifiers, sizeof(hkpConstraintModifier*));
    }
    p_m_modifiers->m_data[p_m_modifiers->m_size++] = cm;
    hkArray<int32_t, hkContainerHeapAllocator>* v8 = &m_scheme->m_commands;
    hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
    if (v8->m_size == (v8->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v9->m_heap, v8, sizeof(int32_t));
    }
    v8->m_data[v8->m_size++] = userData;
}

int32_t
hkpConstraintConstructionKit::setPivotsHelper(hkpRigidBody* bodyA, hkpRigidBody* bodyB, const hkVector4& pivot) {
    hkVector4 pivotInA;
    hkVector4 pivotInB;
    pivotInA.setTransformedInversePos(bodyA->m_motion.m_motionState.m_transform, pivot);
    pivotInB.setTransformedInversePos(bodyB->m_motion.m_motionState.m_transform, pivot);
    int32_t v5 = setPivotA(pivotInA);
    setPivotB(pivotInB);
    m_pivotSpecifiedA.m_bool = 1;
    m_pivotSpecifiedB.m_bool = 1;
    return v5;
}

void hkpConstraintConstructionKit::computeConstraintInfo(
    const hkArray<int32_t, hkContainerHeapAllocator>& commands,
    hkpConstraintInfo& info
) {
    int32_t* m_data = commands.m_data;
    int32_t* a1 = &commands.m_data[commands.m_size];
    if (m_data < a1) {
        while (2) {
            switch (*m_data) {
                case 0: {
                    return;
                }
                case 3:
                case 4:
                case 5:
                case 24: {
                    ++m_data;
                    goto LABEL_13;
                }
                case 6:
                case 17:
                case 18: {
                    info.m_sizeOfSchemas += 48;
                    ++info.m_numSolverResults;
                    ++m_data;
                    ++info.m_numSolverElemTemps;
                    goto LABEL_13;
                }
                case 7:
                case 13: {
                    info.m_sizeOfSchemas += 144;
                    info.m_numSolverResults += 3;
                    info.m_numSolverElemTemps += 3;
                    goto LABEL_13;
                }
                case 12: {
                    info.m_sizeOfSchemas += 96;
                    info.m_numSolverResults += 2;
                    ++m_data;
                    info.m_numSolverElemTemps += 2;
                    goto LABEL_13;
                }
                case 14: {
                    info.m_sizeOfSchemas += 64;
                    ++info.m_numSolverResults;
                    ++m_data;
                    info.m_numSolverElemTemps += 3;
                    goto LABEL_13;
                }
                case 15: {
                    info.m_sizeOfSchemas += 80;
                    ++info.m_numSolverResults;
                    ++m_data;
                    info.m_numSolverElemTemps += 3;
                    goto LABEL_13;
                }
                case 16: {
                    info.m_sizeOfSchemas += 64;
                    ++info.m_numSolverResults;
                    ++m_data;
                    ++info.m_numSolverElemTemps;
                    goto LABEL_13;
                }
                case 19: {
                    info.m_sizeOfSchemas += 48;
                    ++info.m_numSolverResults;
                    m_data += 2;
                    ++info.m_numSolverElemTemps;
                    goto LABEL_13;
                }
                case 20: {
                    info.m_sizeOfSchemas += 48;
                    ++info.m_numSolverResults;
                    ++m_data;
                    info.m_numSolverElemTemps += 2;
                    goto LABEL_13;
                }
                case 21: {
                    info.m_sizeOfSchemas += 64;
                    ++info.m_numSolverResults;
                    ++m_data;
                    info.m_numSolverElemTemps += 2;
                    goto LABEL_13;
                }
                default: {
                LABEL_13:;
                    if (++m_data >= a1) {
                        return;
                    }
                    continue;
                }
            }
        }
    }
}
