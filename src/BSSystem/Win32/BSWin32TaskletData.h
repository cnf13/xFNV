#pragma once

#include "BSSystem/BSTaskletData.h"

class BSWin32TaskletGroupData;

// The PC tasklet data, the counterpart of BSXenonTaskletData.
class BSWin32TaskletData : public BSTaskletData {
public:
    BSWin32TaskletData();
    ~BSWin32TaskletData() override;

    BSWin32TaskletGroupData* pGroupData;
    bool bRunOnStartup;
    uint8_t padding0D[3];
    // A BSNonReentrantSpinLock, a 4-byte lock class that is not declared here, so it is held as a uint32_t.
    uint32_t TaskDataLock;
    BSWin32TaskletData* pLink;
};
