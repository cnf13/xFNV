#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiCamera;

// a node that turns its children to face the camera
class NiBillboardNode : public NiNode {
public:
    enum FaceMode : int32_t {
        ALWAYS_FACE_CAMERA = 0,
        ROTATE_ABOUT_UP = 1,
        RIGID_FACE_CAMERA = 2,
        ALWAYS_FACE_CENTER = 3,
        RIGID_FACE_CENTER = 4,
        BSROTATE_ABOUT_UP = 5,
    };

    enum : int32_t {
        MAX_POS = 11,
    };

    enum : int32_t {
        FACE_MODE_MASK = 7,
        FACE_MODE_POS = 0,
        UPDATECONTROLLERS_MASK = 8,
    };

    NiBillboardNode();
    ~NiBillboardNode() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateWorldBound() override;
    void OnVisible(NiCullingProcess& kCuller) override;

    // turns the node to face apCamera as its face mode says
    void RotateToCamera(const NiCamera* pkCamera);
    // Stores the face mode in m_uFlags.
    void SetMode(FaceMode eMode);
    // The face mode stored in m_uFlags.
    FaceMode GetMode() const;
    bool RotateToCenter(
        const NiCamera* pkCamera,
        const NiPoint3& kWorldTranslate,
        NiPoint3& kCamD,
        NiPoint3& kCamU,
        NiPoint3& kCamR
    );

protected:
    void SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos);
    uint16_t GetField(uint16_t uMask, uint16_t uPos) const;
    void SetBit(bool bVal, uint16_t uMask);
    void SetUpdateControllers(bool bUpdateControllers);

public:
    // "prefix = MODE" for a face mode; the string is NiAlloc'ed
    static char* GetViewerString(const char* pcPrefix, FaceMode eMode);
    // The stream loader: a 0xB4-byte node with m_uFlags 9 and m_fSavedTime 0.
    static NiObject* CreateObject();

    uint16_t m_uFlags;
    float m_fSavedTime;

    static NiRTTI ms_RTTI;
};
