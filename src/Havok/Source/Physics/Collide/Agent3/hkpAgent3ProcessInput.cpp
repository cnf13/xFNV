#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"

// Copies the hkpAgent3Input part only (bytes 0x00-0x5F, the alignment bytes 0x14-0x1F left out): the five pointers,
// then m_aTb through a stack copy. m_distAtT1 and m_linearTimInfo stay as the destination had them. Returns apMemory
// in eax.
hkpAgent3ProcessInput* hkpAgent3ProcessInput::CopyConstruct(void* apMemory, const hkpAgent3ProcessInput& arOther) {
	hkpAgent3ProcessInput* pThis = static_cast<hkpAgent3ProcessInput*>(apMemory);
	pThis->m_bodyA = arOther.m_bodyA;
	pThis->m_bodyB = arOther.m_bodyB;
	pThis->m_overrideBodyA = arOther.m_overrideBodyA;
	pThis->m_input = arOther.m_input;
	pThis->m_contactMgr = arOther.m_contactMgr;
	pThis->m_aTb = arOther.m_aTb;
	return pThis;
}
