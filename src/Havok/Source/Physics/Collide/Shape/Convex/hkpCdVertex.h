#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <cstring>

class hkpCdVertex : public hkVector4 {
public:
    // The vertex id the shape keeps in the low 16 bits of w.
    int32_t getId() const {
        uint16_t usId;
        std::memcpy(&usId, &reinterpret_cast<const float*>(&m_quad)[3], sizeof(usId));
        return usId;
    }
};
