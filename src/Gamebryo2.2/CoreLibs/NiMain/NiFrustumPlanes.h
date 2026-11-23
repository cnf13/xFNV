#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"

class NiCamera;
class NiFrustum;
class NiTransform;

// The six culling planes of a frustum in world space, with a bit per plane saying whether it still culls.
class NiFrustumPlanes {
public:
    enum {
        NEAR_PLANE = 0,
        FAR_PLANE = 1,
        LEFT_PLANE = 2,
        RIGHT_PLANE = 3,
        TOP_PLANE = 4,
        BOTTOM_PLANE = 5,
        MAX_PLANES = 6,
    };

    // Constructs the planes in apMemory.
    NiFrustumPlanes();

    void Set(const NiFrustum& kFrust, const NiTransform& kXform);
    void Set(const NiCamera& kCam);

    const NiPlane& GetPlane(uint32_t ePlane) const {
        return m_akCullingPlanes[ePlane];
    }
    void SetPlane(uint32_t ePlane, const NiPlane& kPlane) {
        m_akCullingPlanes[ePlane] = kPlane;
    }
    bool IsPlaneActive(uint32_t ePlane) const {
        return (m_uiActivePlanes & (1u << ePlane)) != 0;
    }
    bool IsAnyPlaneActive() const {
        return m_uiActivePlanes != 0;
    }
    void EnablePlane(uint32_t uiPlane) {
        m_uiActivePlanes |= 1u << uiPlane;
    }
    void DisablePlane(uint32_t ePlane) {
        m_uiActivePlanes &= ~(1u << ePlane);
    }
    uint32_t GetActivePlaneState() const {
        return m_uiActivePlanes;
    }
    void SetActivePlaneState(uint32_t uiState) {
        m_uiActivePlanes = uiState;
    }

    NiPlane m_akCullingPlanes[MAX_PLANES];
    uint32_t m_uiActivePlanes;
};
