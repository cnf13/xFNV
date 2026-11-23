#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

class NiTransformController;

// A sequence of the old stream format: named objects and their controllers; loading converts it.
class NiSequence : public NiObject {
public:
    NiSequence(const char* pcName, uint32_t uiInitSize, uint32_t uiGrowBy);
    ~NiSequence() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    // RegisterStreamables, SaveBinary and IsEqual just call the NiObject bodies.
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;

    // Replaces the name with a copy of pcName.
    void SetName(const char* pcName);
    // Appends a copy of pcName and pkController at the name array's size, growing both arrays; returns the index
    // used. Protected in the engine; public here so its callers can name it.
    uint32_t AddNameControllerPair(const char* pcName, NiTransformController* pkController);

    // Loads a sequence from a KF stream, converting the old format; aspSequence holds the result.
    static void CreateFromKFFile(
        const NiStream& kStream,
        uint32_t uiIndex,
        NiPointer<NiSequence>& aspSequence,
        const char* pcSequenceName
    );
    // The pre-4.1.0.3 sequence converter.
    static NiSequence* ConvertSequence(const NiStream& kStream, uint32_t uiIndex, const char* pcSequenceName);
    static NiObject* CreateObject();
    // Loads a KF file into a local stream and hands it to the stream form; aspSequence is released when the file cannot
    // be loaded.
    static void CreateFromKFFile(
        const char* pcSequenceFile,
        uint32_t uiIndex,
        NiPointer<NiSequence>& aspSequence,
        const char* pcSequenceName
    );

    char* m_pcName;
    NiTPrimitiveArray<char*> m_kObjectName;
    NiTObjectArray<NiPointer<NiTimeController>> m_kCtrl;
    NiPointer<NiRefObject> m_spTextKeys;
    uint32_t m_uiTextKeyReference;

    static NiRTTI ms_RTTI;
};

void SetTextKeys(NiSequence* apThis, [[maybe_unused]] void* apEdx, NiRefObject* apData);
