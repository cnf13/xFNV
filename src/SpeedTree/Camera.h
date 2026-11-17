#pragma once

#include "Reconstruction.hpp"
#include "SpeedTree/SpeedTreeRT.h"

class CIdvCamera {
public:
	CIdvCamera();
	virtual ~CIdvCamera();

	virtual const CIdvCamera& operator=(const CIdvCamera& cRight);

	// The exe's deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	CVec3 m_cPosition;

	static CVec3 m_cCameraPos;
	static CVec3 m_cCameraDir;
};
inline constexpr fnvd::Vtable CIdvCamera_vftable{};
