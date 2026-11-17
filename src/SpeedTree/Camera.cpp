#include "SpeedTree/Camera.h"

#include "BSCore/MemoryManager.h"

CVec3 CIdvCamera::m_cCameraPos(0.0f, 0.0f, 0.0f);
CVec3 CIdvCamera::m_cCameraDir(0.577f, 0.577f, 0.577f);

CIdvCamera::CIdvCamera() : m_cPosition(0.0f, 0.0f, 0.0f) {}

CIdvCamera::~CIdvCamera() {}

const CIdvCamera& CIdvCamera::operator=(const CIdvCamera& cRight) {
	if (&cRight != this) {
		m_cPosition = cRight.m_cPosition;
	}
	return *this;
}

void CIdvCamera::operator delete(void* apMemory) {
	ourFree(apMemory);
}
