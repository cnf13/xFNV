#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// Havok's log sink. Only part of the class is declared: its owners delete it through its virtual destructor.
class hkLogger : public hkReferencedObject {
public:
    ~hkLogger() override;
};
