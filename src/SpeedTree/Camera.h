#pragma once

#include "SpeedTree/SpeedTreeRT.h"

class CIdvCamera {
public:
    CIdvCamera();
    virtual ~CIdvCamera();

    virtual const CIdvCamera& operator=(const CIdvCamera& cRight);

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    CVec3 m_cPosition;

    static CVec3 m_cCameraPos;
    static CVec3 m_cCameraDir;
};
