#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiInterpolator;
class NiPoint3;

// The vertex targets a morph blends between.
class NiMorphData : public NiObject {
public:
    // One target: its vertices, name and the weight interpolator of the old stream format.
    class MorphTarget : public NiMemObject {
    public:
        MorphTarget();
        ~MorphTarget();

        // Loads the name, the old weight (10.1.0.104 up to 20.1.0.2) and uiNumVerts vertices.
        void LoadBinary(NiStream& kStream, uint32_t uiNumVerts);
        // Same vertices and name.
        bool IsEqual(MorphTarget* pkOther, uint32_t uiNumVerts);

        const NiFixedString& GetName() const;
        // the weight interpolator of files before 10.1.0.104
        NiInterpolator* GetLegacyInterpolator() const {
            return m_spLegacyInterpolator;
        }

        // Targets and target arrays are freed through MemoryManager, not NiMemObject's operators.
        static void operator delete(void* apMemory);
        static void operator delete[](void* apMemory);

        NiPoint3* m_aTargetVerts;
        NiFixedString m_kName;
        float m_fLegacyWeight;
        NiPointer<NiInterpolator> m_spLegacyInterpolator;
    };

    NiMorphData();
    ~NiMorphData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

    // target auiIndex, or null past the last one
    MorphTarget* GetTarget(uint32_t uiIndex) const {
        return uiIndex < m_uiNumTargets ? &m_aMorphTargets[uiIndex] : nullptr;
    }

    static NiObject* CreateObject();

    uint32_t m_uiNumTargets;
    uint32_t m_uiNumVertsPerTarget;
    MorphTarget* m_aMorphTargets;
    bool m_bRelativeTargets;

    static NiRTTI ms_RTTI;
};
