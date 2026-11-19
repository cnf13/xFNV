#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrimitiveDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

hkpPrimitiveDrawer::hkpPrimitiveDrawer() : m_displayHandler(nullptr) {}

void hkpPrimitiveDrawer::setDisplayHandler(hkDebugDisplayHandler* displayHandler) {
    m_displayHandler = displayHandler;
}

void hkpPrimitiveDrawer::displayArrow(
    const hkVector4& startPos,
    const hkVector4& arrowDirection,
    const hkVector4& perpDirection,
    int32_t color,
    float scale,
    int32_t id,
    int32_t tag
) {
    uint32_t v20;
    hkVector4 v21;
    hkVector4 v22;
    hkQuaternion v23;
    hkQuaternion v24;
    hkDebugDisplayHandler* m_displayHandler = this->m_displayHandler;
    v21.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(scale), _mm_set_ss(scale), 0), arrowDirection.m_quad), startPos.m_quad
    );
    m_displayHandler->displayLine(startPos, v21, color, id, tag);
    v23.setAxisAngle(perpDirection, 0.7853982f);
    v24.setAxisAngle(perpDirection, -1.5707964f);
    __m128 v10 = _mm_mul_ps(v23.m_vec.m_quad, arrowDirection.m_quad);
    __m128 v11 = _mm_shuffle_ps(v10, v10, 170);
    XFNV_M128_F32(v11, 0) =
        XFNV_M128_F32(v11, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0));
    __m128 v12 = _mm_shuffle_ps(v23.m_vec.m_quad, v23.m_vec.m_quad, 255);
    __m128 v13 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(v23.m_vec.m_quad, v23.m_vec.m_quad, 201),
                    _mm_shuffle_ps(arrowDirection.m_quad, arrowDirection.m_quad, 210)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(v23.m_vec.m_quad, v23.m_vec.m_quad, 210),
                    _mm_shuffle_ps(arrowDirection.m_quad, arrowDirection.m_quad, 201)
                )
            ),
            v12
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v12, v12), hkQuadRealMinusHalf), arrowDirection.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), v23.m_vec.m_quad)
        )
    );
    __m128 v25 = _mm_add_ps(v13, v13);
    hkDebugDisplayHandler* v14 = this->m_displayHandler;
    *reinterpret_cast<float*>(&v20) = -scale * 0.3330000042915344;
    v22.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v20)), _mm_set_ss(std::bit_cast<float>(v20)), 0), v25
        ),
        v21.m_quad
    );
    v14->displayLine(v21, v22, color, id, tag);
    hkDebugDisplayHandler* v15 = this->m_displayHandler;
    __m128 v16 = _mm_mul_ps(v24.m_vec.m_quad, v25);
    __m128 v17 = _mm_shuffle_ps(v16, v16, 170);
    XFNV_M128_F32(v17, 0) =
        XFNV_M128_F32(v17, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 85), 0) + XFNV_M128_F32(v16, 0));
    __m128 v18 = _mm_shuffle_ps(v24.m_vec.m_quad, v24.m_vec.m_quad, 255);
    __m128 v19 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(v24.m_vec.m_quad, v24.m_vec.m_quad, 201), _mm_shuffle_ps(v25, v25, 210)),
                _mm_mul_ps(_mm_shuffle_ps(v24.m_vec.m_quad, v24.m_vec.m_quad, 210), _mm_shuffle_ps(v25, v25, 201))
            ),
            v18
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v18, v18), hkQuadRealMinusHalf), v25),
            _mm_mul_ps(_mm_shuffle_ps(v17, v17, 0), v24.m_vec.m_quad)
        )
    );
    v22.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v20)), _mm_set_ss(std::bit_cast<float>(v20)), 0),
            _mm_add_ps(v19, v19)
        ),
        v21.m_quad
    );
    v15->displayLine(v21, v22, color, id, tag);
}

// Normalizes normal and startPerp in place, then draws numSegments lines around center from radius * startPerp
// (turned by thetaMin about normal), each turned by (thetaMax - thetaMin) / numSegments from the last.
void hkpPrimitiveDrawer::drawSemiCircle(
    const hkVector4& center,
    hkVector4& normal,
    hkVector4& startPerp,
    float thetaMin,
    float thetaMax,
    float radius,
    int32_t numSegments,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkVector4 start;
    hkVector4 end;
    hkRotation rot;
    startPerp.normalize3();
    normal.normalize3();
    rot.setAxisAngle(normal, thetaMin);
    start.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(radius), _mm_set_ss(radius), 0), startPerp.m_quad);
    start.setRotatedDir(rot, start);
    start.m_quad = _mm_add_ps(center.m_quad, start.m_quad);
    // The step is computed in double precision and stored as a float.
    const float step = (thetaMax - thetaMin) / static_cast<double>(numSegments);
    rot.setAxisAngle(normal, step);
    for (int32_t i = numSegments; i > 0; --i) {
        end.m_quad = _mm_sub_ps(start.m_quad, center.m_quad);
        end.setRotatedDir(rot, end);
        end.m_quad = _mm_add_ps(center.m_quad, end.m_quad);
        m_displayHandler->displayLine(start, end, color, id, tag);
        start.m_quad = end.m_quad;
    }
}

void hkpPrimitiveDrawer::displayOrientedPoint(
    const hkVector4& position,
    const hkRotation& rot,
    float size,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkVector4 v8;
    uint32_t v15;
    hkVector4 v16;
    hkVector4 v17;
    v8.m_quad = rot.m_col0.m_quad;
    *reinterpret_cast<float*>(&v15) = -size;
    __m128 m_quad = rot.m_col1.m_quad;
    __m128 v19 = rot.m_col2.m_quad;
    __m128 v9 = position.m_quad;
    v17.m_quad =
        _mm_sub_ps(position.m_quad, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(size), _mm_set_ss(size), 0), v8.m_quad));
    hkDebugDisplayHandler* m_displayHandler = this->m_displayHandler;
    v16.m_quad = _mm_sub_ps(
        v9,
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v15)), _mm_set_ss(std::bit_cast<float>(v15)), 0), v8.m_quad
        )
    );
    m_displayHandler->displayLine(v17, v16, color, id, tag);
    __m128 v11 = position.m_quad;
    hkDebugDisplayHandler* v12 = this->m_displayHandler;
    v17.m_quad = _mm_sub_ps(position.m_quad, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(size), _mm_set_ss(size), 0), m_quad));
    v16.m_quad = _mm_sub_ps(
        v11,
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v15)), _mm_set_ss(std::bit_cast<float>(v15)), 0), m_quad
        )
    );
    v12->displayLine(v17, v16, color, id, tag);
    __m128 v13 = position.m_quad;
    hkDebugDisplayHandler* v14 = this->m_displayHandler;
    v17.m_quad = _mm_sub_ps(position.m_quad, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(size), _mm_set_ss(size), 0), v19));
    v16.m_quad = _mm_sub_ps(
        v13,
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v15)), _mm_set_ss(std::bit_cast<float>(v15)), 0), v19)
    );
    v14->displayLine(v17, v16, color, id, tag);
}

void hkpPrimitiveDrawer::displayCone(
    float coneAngle,
    const hkVector4& startPos,
    const hkVector4& coneAxis,
    const hkVector4& perpVector,
    int32_t numSegments,
    int32_t color,
    float coneSize,
    int32_t id,
    int32_t tag
) {
    hkQuaternion v30;
    hkQuaternion v31;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkVector4* v11 =
        static_cast<hkVector4*>(hkMemoryRouter::easyAlloc(*Instance->m_heap, sizeof(hkVector4) * numSegments));
    hkVector4* v29 = v11;
    v30.setAxisAngle(perpVector, coneAngle);
    float afAngle = 6.2831854820251465 / static_cast<double>(numSegments);
    v31.setAxisAngle(coneAxis, afAngle);
    __m128 v12 = _mm_mul_ps(coneAxis.m_quad, v30.m_vec.m_quad);
    __m128 v13 = _mm_shuffle_ps(v12, v12, 170);
    XFNV_M128_F32(v13, 0) =
        XFNV_M128_F32(v13, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0));
    __m128 v14 = _mm_shuffle_ps(v30.m_vec.m_quad, v30.m_vec.m_quad, 255);
    __m128 v15 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(coneAxis.m_quad, coneAxis.m_quad, 210),
                    _mm_shuffle_ps(v30.m_vec.m_quad, v30.m_vec.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(coneAxis.m_quad, coneAxis.m_quad, 201),
                    _mm_shuffle_ps(v30.m_vec.m_quad, v30.m_vec.m_quad, 210)
                )
            ),
            v14
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v14, v14), hkQuadRealMinusHalf), coneAxis.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v30.m_vec.m_quad)
        )
    );
    __m128 v16 = _mm_add_ps(v15, v15);
    if (numSegments > 0) {
        __m128* p_m_quad = &v11->m_quad;
        int32_t v18 = numSegments;
        do {
            __m128 m_quad = startPos.m_quad;
            *p_m_quad = startPos.m_quad;
            XFNV_M128_F32(m_quad, 0) = coneSize / coneAngle;
            *p_m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), v16), *p_m_quad);
            __m128 v20 = _mm_mul_ps(v31.m_vec.m_quad, v16);
            __m128 v21 = _mm_shuffle_ps(v20, v20, 170);
            XFNV_M128_F32(v21, 0) =
                XFNV_M128_F32(v21, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v20, v20, 85), 0) + XFNV_M128_F32(v20, 0));
            __m128 v22 = _mm_shuffle_ps(v31.m_vec.m_quad, v31.m_vec.m_quad, 255);
            __m128 v23 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_shuffle_ps(v31.m_vec.m_quad, v31.m_vec.m_quad, 201), _mm_shuffle_ps(v16, v16, 210)
                        ),
                        _mm_mul_ps(
                            _mm_shuffle_ps(v31.m_vec.m_quad, v31.m_vec.m_quad, 210), _mm_shuffle_ps(v16, v16, 201)
                        )
                    ),
                    v22
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v22, v22), hkQuadRealMinusHalf), v16),
                    _mm_mul_ps(_mm_shuffle_ps(v21, v21, 0), v31.m_vec.m_quad)
                )
            );
            ++p_m_quad;
            --v18;
            v16 = _mm_add_ps(v23, v23);
        }
        while (v18);
    }
    int32_t v24 = 0;
    if (numSegments > 0) {
        hkVector4* v25 = v11;
        do {
            m_displayHandler->displayLine(startPos, *v25, color, id, tag);
            ++v24;
            v11 = v29;
            m_displayHandler->displayLine(*v25++, v29[v24 % numSegments], color, id, tag);
        }
        while (v24 < numSegments);
    }
    hkMemoryRouter* v26 = &hkMemoryRouter::getInstance();
    hkMemoryRouter::easyFree(*v26->m_heap, v11);
}

// Draws the square of side scale about startPos in the plane of planeNormal, a corner along vectorOnPlane turned by
// 45 degrees, with its two diagonals, and an arrow along planeNormal.
void hkpPrimitiveDrawer::displayPlane(
    const hkVector4& startPos,
    const hkVector4& planeNormal,
    const hkVector4& vectorOnPlane,
    int32_t color,
    float scale,
    int32_t id,
    int32_t tag
) {
    hkVector4 pVec;
    hkVector4 points[4];
    hkQuaternion q45;
    hkQuaternion q90;
    // Multiplies by the double 0.70711 and stores the product as a float.
    const float halfDiagonal = scale * 0.7071099877357483;
    pVec.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(halfDiagonal), _mm_set_ss(halfDiagonal), 0), vectorOnPlane.m_quad);
    q90.setAxisAngle(planeNormal, 1.5707964f);
    q45.setAxisAngle(planeNormal, 0.7853982f);
    pVec.setRotatedDir(q45, pVec);
    for (hkVector4& point : points) {
        point.m_quad = startPos.m_quad;
        point.m_quad = _mm_add_ps(point.m_quad, pVec.m_quad);
        pVec.setRotatedDir(q90, pVec);
    }
    m_displayHandler->displayLine(points[0], points[1], color, id, tag);
    m_displayHandler->displayLine(points[1], points[2], color, id, tag);
    m_displayHandler->displayLine(points[2], points[3], color, id, tag);
    m_displayHandler->displayLine(points[3], points[0], color, id, tag);
    m_displayHandler->displayLine(points[0], points[2], color, id, tag);
    m_displayHandler->displayLine(points[1], points[3], color, id, tag);
    pVec.normalize3();
    displayArrow(startPos, planeNormal, pVec, color, scale, id, tag);
}

// Appends a name and its colour to the table's pairs.
void hkMonitorStreamColorTable::addColorPair(const char* apColorName, uint32_t aColor) {
    // The pair is built in an untyped 8-byte buffer: the C-string constructor fills the name, the colour goes beside
    // it, and the element is copy-constructed from the buffer. The casts below read that buffer.
    uint32_t aPair[2];
    ::new (aPair) hkStringPtr(apColorName);
    aPair[1] = aColor;
    // The router is fetched before the capacity is tested, so the call is not inside the branch.
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t iCapacity = static_cast<int32_t>(m_colorPairs.m_capacityAndFlags & hkArrayBase<void>::CAPACITY_MASK);
    if (m_colorPairs.m_size == static_cast<uint32_t>(iCapacity)) {
        hkArrayUtil::_reserveMore(*pHeap, &m_colorPairs, sizeof(ColorPair));
    }
    ColorPair* pPair = m_colorPairs.m_data + m_colorPairs.m_size;
    if (pPair) {
        ::new (pPair) hkStringPtr(*reinterpret_cast<const hkStringPtr*>(aPair));
        pPair->m_color = aPair[1];
    }
    ++m_colorPairs.m_size;
    reinterpret_cast<hkStringPtr*>(aPair)->~hkStringPtr();
}

// Adds the totals of every child of apNode into the node of apSum with the same name, allocating the ones apSum does
// not have yet, then does the same for the grandchildren.
void hkMakeSumRecursive(hkMonitorStreamAnalyzer::Node* sum, hkMonitorStreamAnalyzer::Node* tree) {
    // The child count compare is signed.
    for (hkMonitorStreamAnalyzer::Node* pChild : tree->m_children) {
        hkMonitorStreamAnalyzer::Node* pSumChild = hkFindChildByName(sum, pChild->m_name, false);
        if (!pSumChild) {
            // Takes the node from the router's heap through the allocator's own blockAlloc.
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            void* pMemory = pHeap->blockAlloc(sizeof(hkMonitorStreamAnalyzer::Node));
            if (pMemory) {
                pSumChild = ::new (pMemory) hkMonitorStreamAnalyzer::Node(
                    sum, pChild->m_name, static_cast<hkMonitorStreamAnalyzer::Node::NodeType>(pChild->m_type.m_storage)
                );
            }
            else {
                pSumChild = nullptr;
            }
        }
        pSumChild->m_value[0] += pChild->m_value[0];
        pSumChild->m_count[0] += pChild->m_count[0];
        pSumChild->m_value[1] += pChild->m_value[1];
        pSumChild->m_count[1] += pChild->m_count[1];
        pSumChild->m_value[2] += pChild->m_value[2];
        pSumChild->m_count[2] += pChild->m_count[2];
        pSumChild->m_value[3] += pChild->m_value[3];
        pSumChild->m_count[3] += pChild->m_count[3];
        pSumChild->m_value[4] += pChild->m_value[4];
        pSumChild->m_count[4] += pChild->m_count[4];
        pSumChild->m_value[5] += pChild->m_value[5];
        pSumChild->m_count[5] += pChild->m_count[5];
        pSumChild->m_value[6] += pChild->m_value[6];
        pSumChild->m_count[6] += pChild->m_count[6];
        pSumChild->m_value[7] += pChild->m_value[7];
        pSumChild->m_count[7] += pChild->m_count[7];
        hkMakeSumRecursive(pSumChild, pChild);
    }
}
