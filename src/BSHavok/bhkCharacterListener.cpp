#include "BSHavok/bhkCharacterListener.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyListener.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Builds the listener: its vtable, the up direction, the slope's cosine, the support surface and the empty support
// heights. The exe calls the CRT's __CIcos and __CIsin with the slope on the x87 stack, rounds both results to float,
// and divides the step height (widened to double) by the sine before multiplying by the cosine. hkSupportNorm (0x40)
// is left as it was: the exe does not write it.
// std::cos/_CIsin are those call sites (0xEC9F70, 0xECA0A0): the slope goes in ST(0), each result is rounded to
// float as the exe's `fstp dword` does.
bhkCharacterListener::bhkCharacterListener(const hkVector4& arUp, float afMaxSlope, float afStepHeight)
	: iFlags(0), hkUp(arUp), fMaxSlope(static_cast<float>(std::cos(afMaxSlope))), eSupportMaterial(BHK_MATERIAL_MAX),
	  eSupportLayer(L_UNIDENTIFIED), SupportHeight{1.17549435e-38f, 1.17549435e-38f, 1.17549435e-38f, 1.17549435e-38f},
	  fStepHeight(afStepHeight), bAllSurfacesStopped(false), bStickyCollisions(false), iNumCollisions(0) {
	hkSupportVelocity.setZero4();
	float fSin = static_cast<float>(std::sin(afMaxSlope));
	fCastDepth = static_cast<float>(static_cast<double>(afStepHeight) / static_cast<double>(fSin) *
		static_cast<double>(fMaxSlope));
}

// The listener's own destructor is inlined down to the base's vtable store; with bit 0 the block is freed as an
// aligned one. The exe treats it as raw memory: the padding byte in front of the object gives the block's start.
bhkCharacterListener::~bhkCharacterListener() {}

// Counts a contact with another character controller; with sticky collisions it also stops the controller this
// listener belongs to (the exe steps back from the listener subobject at +0x410 with no null test).
void bhkCharacterListener::contactPointAddedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& arPoint) {
	if ((arPoint.m_rootCollidableB->m_broadPhaseHandle.m_collisionFilterInfo & 0x7F) == L_CHARCONTROLLER) {
		++iNumCollisions;
		if (bStickyCollisions) {
			static_cast<bhkCharacterController&>(*this).SetLinearVelocityNi(NiPoint3_ZERO);
		}
	}
}

void bhkCharacterListener::contactPointRemovedCallback(const hkpCharacterProxy* apProxy,
	const hkpRootCdPoint& arPoint) {
	uint32_t iNumCollisions;
	if (((int32_t)((&arPoint)->m_rootCollidableB->m_broadPhaseHandle.m_collisionFilterInfo & 127)) == 30) {
		iNumCollisions = this->iNumCollisions;
		if (iNumCollisions) {
			this->iNumCollisions = iNumCollisions - 1;
		}
	}
}

void bhkCharacterListener::characterInteractionCallback(hkpCharacterProxy* apProxy, hkpCharacterProxy* apOtherProxy,
	const hkContactPoint& arContact) {
	hkVector4 v13;
	v13.m_quad = (&apProxy->getLinearVelocity())->m_quad;
	if ((_mm_movemask_ps(_mm_cmple_ps(_mm_and_ps(_mm_sub_ps(v13.m_quad, hkVector4Zero.m_quad),
										  hkVector4_setAbs4_absMask),
			 _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x3C23D70Au)),
				 _mm_set_ss(std::bit_cast<float>(0x3C23D70Au)), 0))) &
			7) != 7) {
		__m128 v8 = _mm_undefined_ps();
		hkVector4 v12;
		v12.m_quad = arContact.m_separatingNormal.m_quad;
		v12.m_quad[2] = 0.0f;
		v13.m_quad[2] = 0.0f;
		__m128 v4 = _mm_mul_ps(v12.m_quad, v12.m_quad);
		float v5 = _mm_shuffle_ps(v4, v4, 85)[0] + v4[0];
		__m128 v6 = _mm_shuffle_ps(v4, v4, 170);
		v6[0] = v6[0] + v5;
		float v7 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v6[0])));
		v8[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
		__m128 v9 = _mm_cmpeq_ss(v6, v8);
		v8[0] = 3.0f - static_cast<float>(static_cast<float>(v6[0] * v7) * v7);
		__m128 v10 = _mm_setzero_ps();
		v10[0] = static_cast<float>(0.5f * v7) * v8[0];
		__m128 v11 = _mm_andnot_ps(v9, v10);
		v13.m_quad = _mm_add_ps((&apOtherProxy->getLinearVelocity())->m_quad,
			_mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), v12.m_quad), v13.m_quad));
		apOtherProxy->setVelocity(&v13);
	}
}

uint32_t bhkCharacterListener::GetFlag(uint32_t aiFlags) {
	return iFlags & aiFlags;
}

void bhkCharacterListener::SetFlag(uint32_t aiFlags, bool abTF) {
	if (abTF)
		iFlags |= aiFlags;
	else
		ClearFlag(aiFlags);
}

void bhkCharacterListener::ClearFlag(uint32_t aiFlags) {
	iFlags = ~aiFlags & iFlags;
}

bool bhkCharacterListener::QQuadruped() {
	return GetFlag(0x1) != 0;
}

bool bhkCharacterListener::QStuckQuadruped() {
	return GetFlag(0x20) != 0;
}

void bhkCharacterListener::SetStickyCollisions(const bool& bSticky) {
	bStickyCollisions = bSticky;
	if (!bSticky)
		iNumCollisions = 0;
}

void bhkCharacterListener::SetStuckQuadruped(bool abTF) {
	SetFlag(32, abTF);
}

bool bhkCharacterListener::StuckToSomeone() {
	return (bool)((uint8_t)(bStickyCollisions && iNumCollisions));
}

// The exe keeps the sum in the afHeight argument slot (fstp dword) before the multiply by the double 0.5. The empty
// marker is FLT_MIN (0x00800000); its `test ah,0x44; jp` takes the average branch on unordered and on unequal, so an
// unordered compare averages.
void bhkCharacterListener::AddSupportHeight(uint32_t aiIndex, float afHeight) {
	if (SupportHeight[aiIndex] == 1.17549435e-38f) {
		SupportHeight[aiIndex] = afHeight;
		return;
	}
	afHeight = SupportHeight[aiIndex] + afHeight;
	SupportHeight[aiIndex] = static_cast<float>(afHeight * 0.5);
}

// The exe subtracts all four points with SSE, zeroes z, and sums the squares as floats. A tie or NaN picks the
// second point of each pair. The sum of the old height and the new one is rounded to float before the multiply by
// the double 0.5; FLT_MIN marks an empty slot and an unordered compare averages.
void bhkCharacterListener::AddSupportHeight2(hkVector4* apFrontBack, const hkVector4& arPos) {
	alignas(16) hkVector4 kDiff[4];
	for (int32_t i = 0; i < 4; ++i) {
		kDiff[i].m_quad[0] = arPos.m_quad[0] - apFrontBack[i].m_quad[0];
		kDiff[i].m_quad[1] = arPos.m_quad[1] - apFrontBack[i].m_quad[1];
		kDiff[i].m_quad[2] = 0.0f;
	}
	float fLen0 = (kDiff[0].m_quad[1] * kDiff[0].m_quad[1] + kDiff[0].m_quad[0] * kDiff[0].m_quad[0]) +
		kDiff[0].m_quad[2] * kDiff[0].m_quad[2];
	float fLen1 = (kDiff[1].m_quad[1] * kDiff[1].m_quad[1] + kDiff[1].m_quad[0] * kDiff[1].m_quad[0]) +
		kDiff[1].m_quad[2] * kDiff[1].m_quad[2];
	int32_t iIndex = (fLen1 > fLen0) ? 0 : 1;
	float fHeight = arPos.m_quad[2];
	// EXE-INLINED 0xC6C720
	if (SupportHeight[iIndex] == 1.17549435e-38f) {
		SupportHeight[iIndex] = fHeight;
	} else {
		float fSum = fHeight + SupportHeight[iIndex];
		SupportHeight[iIndex] = static_cast<float>(fSum * 0.5);
	}
	float fLen2 = (kDiff[2].m_quad[1] * kDiff[2].m_quad[1] + kDiff[2].m_quad[0] * kDiff[2].m_quad[0]) +
		kDiff[2].m_quad[2] * kDiff[2].m_quad[2];
	float fLen3 = (kDiff[3].m_quad[1] * kDiff[3].m_quad[1] + kDiff[3].m_quad[0] * kDiff[3].m_quad[0]) +
		kDiff[3].m_quad[2] * kDiff[3].m_quad[2];
	iIndex = (fLen3 > fLen2) ? 2 : 3;
	fHeight = arPos.m_quad[2];
	// EXE-INLINED 0xC6C720
	if (SupportHeight[iIndex] == 1.17549435e-38f) {
		SupportHeight[iIndex] = fHeight;
	} else {
		float fSum = fHeight + SupportHeight[iIndex];
		SupportHeight[iIndex] = static_cast<float>(0.5 * fSum);
	}
}

void bhkCharacterListener::SetMoveLimitMass(float afValue) {
	fMoveLimitMass = afValue;
}

void bhkCharacterListener::operator delete(void* apMemory) {
	uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
	MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
}

hkpCharacterProxyListener::~hkpCharacterProxyListener() {}

void hkpCharacterProxyListener::operator delete(void* apMemory) {
	(&MemoryManager_Singleton)->Deallocate(apMemory);
}
