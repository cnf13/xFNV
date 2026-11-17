#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppLongRayVirtualMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

#include <bit>
#include <xmmintrin.h>

hkpMoppLongRayVirtualMachine::QueryInt*
hkpMoppLongRayVirtualMachine::QueryInt::Assign(hkpMoppLongRayVirtualMachine::QueryInt* apThis,
	const hkpMoppLongRayVirtualMachine::QueryInt& arOther) {
	*apThis = arOther;
	return apThis;
}

void hkpMoppLongRayVirtualMachine::queryLongRay(const hkpShapeContainer* collection, const hkpMoppCode* code,
	const hkpShapeRayCastInput& input, const hkpCdBody& body, hkpRayHitCollector& collector) {
	hkpMoppCode::CodeInfo v7;
	hkpMoppLongRayVirtualMachine::QueryInt aScaledQuery;
	hkpMoppLongRayVirtualMachine::QueryFloat aQueryFloat;
	m_collection = collection;
	m_collector = &collector;
	m_body = &body;
	m_rayResultPtr = 0;
	m_earlyOutHitFraction = collector.m_earlyOutHitFraction;
	m_code = code;
	uint8_t* m_data = code->m_data.m_data;
	aScaledQuery.m_FtoBoffset.m_quad = _mm_setzero_ps();
	m_ItoFScale = 1.0 / code->m_info.m_offset.m_quad[3];
	v7.m_offset.m_quad = code->m_info.m_offset.m_quad;
	__m128 m_quad = input.m_from.m_quad;
	__m128 v9 = _mm_sub_ps(input.m_to.m_quad, v7.m_offset.m_quad);
	aScaledQuery.m_FtoBScale = code->m_info.m_offset.m_quad[3] * 1.52587890625e-05;
	__m128 v10 = _mm_sub_ps(m_quad, v7.m_offset.m_quad);
	__m128 m_FtoBScale_low = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(aScaledQuery.m_FtoBScale)));
	m_ray.m_from.m_quad = m_quad;
	m_ray.m_to = input.m_to;
	m_ray.m_filterInfo = input.m_filterInfo;
	m_ray.m_rayShapeCollectionFilter = input.m_rayShapeCollectionFilter;
	m_hitFound.m_bool = 0;
	aScaledQuery.m_primitiveOffset = 0;
	aScaledQuery.m_shift = 0;
	aScaledQuery.m_properties[0] = 0;
	int32_t v12 = *m_data != 13;
	aQueryFloat.m_rayEnds[0].m_quad = _mm_mul_ps(_mm_shuffle_ps(m_FtoBScale_low, m_FtoBScale_low, 0), v10);
	aQueryFloat.m_rayEnds[1].m_quad = _mm_mul_ps(_mm_shuffle_ps(m_FtoBScale_low, m_FtoBScale_low, 0), v9);
	m_reindexingMask = v12 - 1;
	queryRayOnTree(&aScaledQuery, m_data, &aQueryFloat, 0);
}

hkBool* hkpMoppLongRayVirtualMachine::queryLongRay_00DD2DF0(hkBool* apResult, const hkpShapeContainer* collection,
	const hkpMoppCode* code, const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& rayResult) {
	hkpMoppCode::CodeInfo v8;
	hkpMoppLongRayVirtualMachine::QueryInt aScaledQuery;
	hkpMoppLongRayVirtualMachine::QueryFloat aQueryFloat;
	m_earlyOutHitFraction = 1.0f;
	m_collection = collection;
	m_code = code;
	m_rayResultPtr = &rayResult;
	uint8_t* m_data = code->m_data.m_data;
	aScaledQuery.m_FtoBoffset.m_quad = _mm_setzero_ps();
	m_ItoFScale = 1.0 / code->m_info.m_offset.m_quad[3];
	v8.m_offset.m_quad = code->m_info.m_offset.m_quad;
	__m128 m_quad = input.m_from.m_quad;
	__m128 v10 = _mm_sub_ps(input.m_to.m_quad, v8.m_offset.m_quad);
	aScaledQuery.m_FtoBScale = code->m_info.m_offset.m_quad[3] * 1.52587890625e-05;
	__m128 v11 = _mm_sub_ps(m_quad, v8.m_offset.m_quad);
	__m128 m_FtoBScale_low = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(aScaledQuery.m_FtoBScale)));
	m_ray.m_from.m_quad = m_quad;
	m_ray.m_to = input.m_to;
	m_ray.m_filterInfo = input.m_filterInfo;
	m_ray.m_rayShapeCollectionFilter = input.m_rayShapeCollectionFilter;
	m_hitFound.m_bool = 0;
	aScaledQuery.m_primitiveOffset = 0;
	aScaledQuery.m_shift = 0;
	aScaledQuery.m_properties[0] = 0;
	int32_t v13 = (*m_data != 13) - 1;
	aQueryFloat.m_rayEnds[0].m_quad = _mm_mul_ps(_mm_shuffle_ps(m_FtoBScale_low, m_FtoBScale_low, 0), v11);
	aQueryFloat.m_rayEnds[1].m_quad = _mm_mul_ps(_mm_shuffle_ps(m_FtoBScale_low, m_FtoBScale_low, 0), v10);
	m_reindexingMask = v13;
	queryRayOnTree(&aScaledQuery, m_data, &aQueryFloat, 0);
	apResult->m_bool = m_hitFound.m_bool;
	return apResult;
}

// The MOPP walk for one long ray. The split commands carry two bounds on one axis (or a diagonal): the left child holds
// the cells up to fLeftMax, the right child those from fRightMin. A ray entirely below fRightMin goes left only, one
// entirely above fLeftMax right only; otherwise the child the ray enters first is walked with a copy of the ray clipped
// at its bound, then the ray is advanced to the other bound and walked on there. Intermediate values stored as floats
// are float locals; the products and quotients in between stay in one expression.
// KEEP: a property command for properties 1-3 that takes the copy reads localQuery.m_properties[0] before anything
// wrote it (0xDD2B1B, before the Assign call 0xDD1F00) and stores it back after the copy; the recursive calls are
// then given localQuery, and their copies (Assign, the rescale) read the uninitialised value.
void hkpMoppLongRayVirtualMachine::queryRayOnTree(const hkpMoppLongRayVirtualMachine::QueryInt* query,
	const uint8_t* PC, hkpMoppLongRayVirtualMachine::QueryFloat* fQuery, int32_t chunkOffset) {
	// The query once a command rescales or re-offsets it, and the ray copy the first child of a split gets; both are
	// passed by address and aligned to 16.
	alignas(16) QueryInt localQuery;
	alignas(16) QueryFloat childFloat;
	// The child shape getChildShape builds; the error stream of the unknown command writes into the same buffer.
	alignas(16) char shapeBuffer[512];
	if (!hkpMoppLongRayVirtualMachine_HK_flyingcolors_mopp) {
		hkpProcessFlyingColors(&hkpMoppLongRayVirtualMachine_HK_flyingcolors_mopp);
		if (!hkpMoppLongRayVirtualMachine_HK_flyingcolors_mopp) {
			return;
		}
	}
	while (true) {
		const int32_t command = *PC;
		// The split's axis; 999 for the diagonal splits, which skip the early-out test against the ray's end.
		int32_t iSplitDirection = 999;
		float fLeftMax;
		float fRightMin;
		float fStart;
		float fEnd;
		int32_t iLeftOffset;
		int32_t iRightOffset;
		switch (command) {
		case 0:
			return;
		case 1:
		case 2:
		case 3:
		case 4: {
			// Rescale: move the ray to the sub-cell (x, y, z) and scale it by 2^command.
			const float fX = static_cast<float>(PC[1]);
			const float fY = static_cast<float>(PC[2]);
			const float fZ = static_cast<float>(PC[3]);
			const float fScale = static_cast<float>(1 << command);
			const __m128 vOffset = _mm_set_ps(0.0f, fZ, fY, fX);
			fQuery->m_rayEnds[0].m_quad = _mm_sub_ps(fQuery->m_rayEnds[0].m_quad, vOffset);
			fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(fQuery->m_rayEnds[1].m_quad, vOffset);
			const __m128 vScale = _mm_set1_ps(fScale);
			fQuery->m_rayEnds[0].m_quad = _mm_mul_ps(vScale, fQuery->m_rayEnds[0].m_quad);
			fQuery->m_rayEnds[1].m_quad = _mm_mul_ps(vScale, fQuery->m_rayEnds[1].m_quad);
			localQuery.m_FtoBoffset.m_quad = _mm_mul_ps(vScale, _mm_add_ps(vOffset, query->m_FtoBoffset.m_quad));
			localQuery.m_shift = query->m_shift + command;
			localQuery.m_FtoBScale = query->m_FtoBScale * fScale;
			localQuery.m_properties[0] = query->m_properties[0];
			localQuery.m_primitiveOffset = query->m_primitiveOffset;
			PC += 4;
			query = &localQuery;
			continue;
		}
		case 5:
			PC += PC[1] + 2;
			continue;
		case 6:
			PC += (PC[1] << 8) + PC[2] + 3;
			continue;
		case 7:
			PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4;
			continue;
		case 9: {
			const int32_t iAdd = PC[1];
			if (query != &localQuery) {
				QueryInt::Assign(&localQuery, *query);
				query = &localQuery;
			}
			localQuery.m_primitiveOffset += iAdd;
			PC += 2;
			continue;
		}
		case 10: {
			const int32_t iAdd = (PC[1] << 8) + PC[2];
			if (query != &localQuery) {
				QueryInt::Assign(&localQuery, *query);
				query = &localQuery;
			}
			localQuery.m_primitiveOffset += iAdd;
			PC += 3;
			continue;
		}
		case 11: {
			const uint32_t uiOffset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
			if (query != &localQuery) {
				QueryInt::Assign(&localQuery, *query);
				query = &localQuery;
			}
			localQuery.m_primitiveOffset = uiOffset;
			PC += 5;
			continue;
		}
		case 12:
			chunkOffset = ((PC[1] << 8) | PC[2]) << 9;
			PC = m_code->m_data.m_data + chunkOffset;
			continue;
		case 13:
			PC += 5;
			continue;
		case 16:
		case 17:
		case 18:
			fLeftMax = static_cast<float>(PC[1]);
			fRightMin = static_cast<float>(PC[2]);
			iSplitDirection = command - 16;
			fStart = fQuery->m_rayEnds[0].m_quad[iSplitDirection];
			fEnd = fQuery->m_rayEnds[1].m_quad[iSplitDirection];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 19:
			fLeftMax = static_cast<float>(2 * PC[1]);
			fRightMin = static_cast<float>(2 * PC[2]);
			fStart = fQuery->m_rayEnds[0].m_quad[2] + fQuery->m_rayEnds[0].m_quad[1];
			fEnd = fQuery->m_rayEnds[1].m_quad[2] + fQuery->m_rayEnds[1].m_quad[1];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 20:
			fLeftMax = static_cast<float>(2 * PC[1] - 255);
			fRightMin = static_cast<float>(2 * PC[2] - 255);
			fStart = fQuery->m_rayEnds[0].m_quad[1] - fQuery->m_rayEnds[0].m_quad[2];
			fEnd = fQuery->m_rayEnds[1].m_quad[1] - fQuery->m_rayEnds[1].m_quad[2];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 21:
			fLeftMax = static_cast<float>(2 * PC[1]);
			fRightMin = static_cast<float>(2 * PC[2]);
			fStart = fQuery->m_rayEnds[0].m_quad[2] + fQuery->m_rayEnds[0].m_quad[0];
			fEnd = fQuery->m_rayEnds[1].m_quad[2] + fQuery->m_rayEnds[1].m_quad[0];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 22:
			fLeftMax = static_cast<float>(2 * PC[1] - 255);
			fRightMin = static_cast<float>(2 * PC[2] - 255);
			fStart = fQuery->m_rayEnds[0].m_quad[0] - fQuery->m_rayEnds[0].m_quad[2];
			fEnd = fQuery->m_rayEnds[1].m_quad[0] - fQuery->m_rayEnds[1].m_quad[2];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 23:
			fLeftMax = static_cast<float>(2 * PC[1]);
			fRightMin = static_cast<float>(2 * PC[2]);
			fStart = fQuery->m_rayEnds[0].m_quad[1] + fQuery->m_rayEnds[0].m_quad[0];
			fEnd = fQuery->m_rayEnds[1].m_quad[1] + fQuery->m_rayEnds[1].m_quad[0];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 24:
			fLeftMax = static_cast<float>(2 * PC[1] - 255);
			fRightMin = static_cast<float>(2 * PC[2] - 255);
			fStart = fQuery->m_rayEnds[0].m_quad[0] - fQuery->m_rayEnds[0].m_quad[1];
			fEnd = fQuery->m_rayEnds[1].m_quad[0] - fQuery->m_rayEnds[1].m_quad[1];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 25:
			fLeftMax = static_cast<float>(3 * PC[1]);
			fRightMin = static_cast<float>(3 * PC[2]);
			fStart = fQuery->m_rayEnds[0].m_quad[1] + fQuery->m_rayEnds[0].m_quad[0] + fQuery->m_rayEnds[0].m_quad[2];
			fEnd = fQuery->m_rayEnds[1].m_quad[1] + fQuery->m_rayEnds[1].m_quad[0] + fQuery->m_rayEnds[1].m_quad[2];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 26:
			fLeftMax = static_cast<float>(3 * (PC[1] - 85));
			fRightMin = static_cast<float>(3 * (PC[2] - 85));
			fStart = fQuery->m_rayEnds[0].m_quad[1] + fQuery->m_rayEnds[0].m_quad[0] - fQuery->m_rayEnds[0].m_quad[2];
			fEnd = fQuery->m_rayEnds[1].m_quad[1] + fQuery->m_rayEnds[1].m_quad[0] - fQuery->m_rayEnds[1].m_quad[2];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 27:
			fLeftMax = static_cast<float>(3 * (PC[1] - 85));
			fRightMin = static_cast<float>(3 * (PC[2] - 85));
			fStart = fQuery->m_rayEnds[0].m_quad[0] - fQuery->m_rayEnds[0].m_quad[1] + fQuery->m_rayEnds[0].m_quad[2];
			fEnd = fQuery->m_rayEnds[1].m_quad[0] - fQuery->m_rayEnds[1].m_quad[1] + fQuery->m_rayEnds[1].m_quad[2];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 28:
			fLeftMax = static_cast<float>(3 * (PC[1] - 170));
			fRightMin = static_cast<float>(3 * (PC[2] - 170));
			fStart = fQuery->m_rayEnds[0].m_quad[0] - fQuery->m_rayEnds[0].m_quad[1] - fQuery->m_rayEnds[0].m_quad[2];
			fEnd = fQuery->m_rayEnds[1].m_quad[0] - fQuery->m_rayEnds[1].m_quad[1] - fQuery->m_rayEnds[1].m_quad[2];
			iRightOffset = PC[3];
			iLeftOffset = 0;
			PC += 4;
			break;
		case 32:
		case 33:
		case 34:
			iSplitDirection = command - 32;
			fRightMin = static_cast<float>(PC[1]);
			fLeftMax = fRightMin + 1.0;
			fStart = fQuery->m_rayEnds[0].m_quad[iSplitDirection];
			fEnd = fQuery->m_rayEnds[1].m_quad[iSplitDirection];
			iRightOffset = PC[2];
			iLeftOffset = 0;
			PC += 3;
			break;
		case 35:
		case 36:
		case 37:
			fLeftMax = static_cast<float>(PC[1]);
			fRightMin = static_cast<float>(PC[2]);
			iSplitDirection = command - 35;
			fStart = fQuery->m_rayEnds[0].m_quad[iSplitDirection];
			fEnd = fQuery->m_rayEnds[1].m_quad[iSplitDirection];
			iLeftOffset = (PC[3] << 8) + PC[4];
			iRightOffset = (PC[5] << 8) + PC[6];
			PC += 7;
			break;
		case 38:
		case 39:
		case 40:
		case 41:
		case 42:
		case 43: {
			// Bounds on one axis: clip the ray to [fLower, fUpper], or stop when it misses them.
			int32_t iDirection;
			float fLower;
			float fUpper;
			if (command < 41) {
				iDirection = command - 38;
				fLower = static_cast<float>(PC[1]);
				fUpper = static_cast<float>(PC[2]);
				PC += 3;
			} else {
				// 24-bit bounds in the code's integer space, converted to this query's.
				iDirection = command - 41;
				const int32_t iLower = (((PC[1] << 8) + PC[2]) << 8) + PC[3];
				float fScaled = static_cast<double>(iLower) * m_ItoFScale;
				fScaled = fScaled * query->m_FtoBScale;
				fLower = fScaled - query->m_FtoBoffset.m_quad[iDirection];
				const int32_t iUpper = (((PC[4] << 8) + PC[5]) << 8) + PC[6];
				fScaled = static_cast<double>(iUpper) * m_ItoFScale;
				fScaled = fScaled * query->m_FtoBScale;
				fUpper = fScaled - query->m_FtoBoffset.m_quad[iDirection];
				PC += 7;
			}
			fStart = fQuery->m_rayEnds[0].m_quad[iDirection];
			fEnd = fQuery->m_rayEnds[1].m_quad[iDirection];
			// The end the ray leaves the bounds through: 0 when it runs up the axis, 1 when it runs down.
			int32_t iFirst;
			if (fEnd > fStart) {
				if (fLower > fEnd) {
					return;
				}
				if (fUpper < fStart) {
					return;
				}
				iFirst = 0;
			} else {
				if (fLower > fStart) {
					return;
				}
				if (fUpper < fEnd) {
					return;
				}
				iFirst = 1;
			}
			const __m128 vStart = fQuery->m_rayEnds[0].m_quad;
			const __m128 vEnd = fQuery->m_rayEnds[1].m_quad;
			const float fStartToUpper = fStart - fUpper;
			const float fEndToUpper = fEnd - fUpper;
			if (fEndToUpper * fStartToUpper < 0.0) {
				const float t = fStartToUpper / (fStartToUpper - fEndToUpper);
				const __m128 vT = _mm_set1_ps(t);
				fQuery->m_rayEnds[1 - iFirst].m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vT), vStart), _mm_mul_ps(vEnd, vT));
			}
			const float fStartToLower = fStart - fLower;
			const float fEndToLower = fEnd - fLower;
			if (fEndToLower * fStartToLower < 0.0) {
				const float t = fStartToLower / (fStartToLower - fEndToLower);
				const __m128 vT = _mm_set1_ps(t);
				fQuery->m_rayEnds[iFirst].m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vT), vStart), _mm_mul_ps(vT, vEnd));
			}
			continue;
		}
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
		case 79:
		case 80:
		case 81:
		case 82:
		case 83: {
			// A terminal: cast the ray against the child shape it names.
			uint32_t uiTerminal;
			if (command < 80) {
				uiTerminal = command - 48;
			} else if (command == 80) {
				uiTerminal = PC[1];
			} else if (command == 81) {
				uiTerminal = (PC[1] << 8) + PC[2];
			} else if (command == 82) {
				uiTerminal = (((PC[1] << 8) + PC[2]) << 8) + PC[3];
			} else {
				uiTerminal = (((PC[1] << 8) + PC[2]) << 16) + (PC[3] << 8) + PC[4];
			}
			const uint32_t uiKey =
				(((chunkOffset >> 9) << 8) & m_reindexingMask) | (query->m_primitiveOffset + uiTerminal);
			const hkpShape* pChild = m_collection->getChildShape(uiKey, shapeBuffer);
			const hkpRayShapeCollectionFilter* pFilter = m_ray.m_rayShapeCollectionFilter;
			if (pFilter && !pFilter->isCollisionEnabled(m_ray, *pChild, *m_collection, uiKey).m_bool) {
				return;
			}
			if (m_rayResultPtr) {
				++m_rayResultPtr->m_shapeKeyIndex;
				const hkBool hit = pChild->castRayImpl(m_ray, *m_rayResultPtr);
				--m_rayResultPtr->m_shapeKeyIndex;
				if (hit.m_bool) {
					m_hitFound = hit;
					hkpShapeRayCastOutput* pResult = m_rayResultPtr;
					m_earlyOutHitFraction = pResult->m_hitFraction;
					pResult->m_shapeKeys[pResult->m_shapeKeyIndex] = uiKey;
				}
				return;
			}
			// The child's body under the body the ray was cast against.
			alignas(16) hkpCdBody childBody;
			childBody.m_parent = m_body;
			childBody.m_motion = m_body->m_motion;
			childBody.m_shape = pChild;
			childBody.m_shapeKey = uiKey;
			pChild->castRayWithCollectorImpl(m_ray, childBody, *m_collector);
			m_earlyOutHitFraction = m_collector->m_earlyOutHitFraction;
			return;
		}
		case 96:
		case 97:
		case 98:
		case 99:
		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107: {
			// A property value for the subtree. The query holds one property, so property 0 ends as the value. For
			// properties 1-3, the copied m_properties[0] is kept and only property 0 is stored.
			int32_t iProperty;
			uint32_t uiValue;
			if (command < 100) {
				iProperty = command - 96;
				uiValue = PC[1];
				PC += 2;
			} else if (command < 104) {
				iProperty = command - 100;
				uiValue = (PC[1] << 8) + PC[2];
				PC += 3;
			} else {
				iProperty = command - 104;
				uiValue = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
				PC += 5;
			}
			if (query != &localQuery) {
				QueryInt::Assign(&localQuery, *query);
				query = &localQuery;
			}
			if (iProperty == 0) {
				localQuery.m_properties[0] = uiValue;
			}
			continue;
		}
		case 112:
			chunkOffset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
			PC = m_code->m_data.m_data + chunkOffset;
			continue;
		default: {
			hkErrStream errStream(shapeBuffer, 512);
			errStream.operator<<("Unknown command - This mopp data has been corrupted (check for memory trashing), or "
								 "an hkpMoppBvTreeShape has been pointed at invalid mopp data.\n");
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x1298fedd, shapeBuffer,
					".\\Collide\\Mopp\\Machine\\hkpMoppLongRayVirtualMachine.cpp", 554)) {
				__builtin_debugtrap();
			}
			continue;
		}
		}

		// A split: one child, or both in the order the ray runs.
		if (fEnd < fRightMin && fStart < fRightMin) {
			PC += iLeftOffset;
			continue;
		}
		PC += iRightOffset;
		if (fLeftMax < fStart && fLeftMax < fEnd) {
			continue;
		}
		const uint8_t* pLeftCommands = PC + (iLeftOffset - iRightOffset);
		childFloat.m_rayEnds[0] = fQuery->m_rayEnds[0];
		childFloat.m_rayEnds[1] = fQuery->m_rayEnds[1];
		const float fStartToLeftMax = fStart - fLeftMax;
		const float fStartToRightMin = fStart - fRightMin;
		const float fEndToLeftMax = fEnd - fLeftMax;
		const float fEndToRightMin = fEnd - fRightMin;
		if (fEndToLeftMax > fStartToLeftMax) {
			// Up the axis: the left child first, its ray ending at fLeftMax.
			if (fEndToLeftMax * fStartToLeftMax < 0.0) {
				const float t = fStartToLeftMax / (fStartToLeftMax - fEndToLeftMax);
				const __m128 vT = _mm_set1_ps(t);
				childFloat.m_rayEnds[1].m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vT), childFloat.m_rayEnds[0].m_quad),
						_mm_mul_ps(vT, childFloat.m_rayEnds[1].m_quad));
			}
			queryRayOnTree(query, pLeftCommands, &childFloat, chunkOffset);
			if (fEndToRightMin * fStartToRightMin < 0.0) {
				const float t = fStartToRightMin / (fStartToRightMin - fEndToRightMin);
				const __m128 vT = _mm_set1_ps(t);
				fQuery->m_rayEnds[0].m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vT), fQuery->m_rayEnds[0].m_quad),
						_mm_mul_ps(vT, fQuery->m_rayEnds[1].m_quad));
			}
			if (m_earlyOutHitFraction < 1.0f) {
				// A hit shortened the ray: its end moves to the hit, in this query's space.
				const __m128 vFraction = _mm_set1_ps(m_earlyOutHitFraction);
				fQuery->m_rayEnds[1].m_quad =
					_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vFraction), m_ray.m_from.m_quad),
								   _mm_mul_ps(m_ray.m_to.m_quad, vFraction)),
						m_code->m_info.m_offset.m_quad);
				fQuery->m_rayEnds[1].m_quad = _mm_mul_ps(_mm_set1_ps(query->m_FtoBScale), fQuery->m_rayEnds[1].m_quad);
				fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(fQuery->m_rayEnds[1].m_quad, query->m_FtoBoffset.m_quad);
				if (iSplitDirection < 3 && fRightMin > fQuery->m_rayEnds[1].m_quad[iSplitDirection]) {
					return;
				}
			}
			// The right child.
			continue;
		}
		// Down the axis: the right child first, its ray ending at fRightMin.
		if (fEndToRightMin * fStartToRightMin < 0.0) {
			const float t = fStartToRightMin / (fStartToRightMin - fEndToRightMin);
			const __m128 vT = _mm_set1_ps(t);
			childFloat.m_rayEnds[1].m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vT), childFloat.m_rayEnds[0].m_quad),
					_mm_mul_ps(vT, childFloat.m_rayEnds[1].m_quad));
		}
		queryRayOnTree(query, PC, &childFloat, chunkOffset);
		if (fEndToLeftMax * fStartToLeftMax < 0.0) {
			const float t = fStartToLeftMax / (fStartToLeftMax - fEndToLeftMax);
			const __m128 vT = _mm_set1_ps(t);
			fQuery->m_rayEnds[0].m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vT), fQuery->m_rayEnds[0].m_quad),
					_mm_mul_ps(vT, fQuery->m_rayEnds[1].m_quad));
		}
		if (m_earlyOutHitFraction < 1.0f) {
			const __m128 vFraction = _mm_set1_ps(m_earlyOutHitFraction);
			fQuery->m_rayEnds[1].m_quad =
				_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, vFraction), m_ray.m_from.m_quad),
							   _mm_mul_ps(m_ray.m_to.m_quad, vFraction)),
					m_code->m_info.m_offset.m_quad);
			fQuery->m_rayEnds[1].m_quad = _mm_mul_ps(_mm_set1_ps(query->m_FtoBScale), fQuery->m_rayEnds[1].m_quad);
			fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(fQuery->m_rayEnds[1].m_quad, query->m_FtoBoffset.m_quad);
			if (iSplitDirection < 3 && fLeftMax < fQuery->m_rayEnds[1].m_quad[iSplitDirection]) {
				return;
			}
		}
		// The left child.
		PC = pLeftCommands;
	}
}
