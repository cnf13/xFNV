#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

class NiExtraData;

// A named scene graph object that can carry extra data and a list of time controllers.
class NiObjectNET : public NiObject {
public:
    enum CopyType : uint32_t {
        COPY_NONE = 0,
        COPY_EXACT = 1,
        COPY_UNIQUE = 2,
    };

    NiObjectNET();
    ~NiObjectNET() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void PostLinkObject(NiStream& kStream) override;

    // The index is taken as int32_t so the caller's whole 32-bit value is passed on; uint16_t would be zero-extended.
    // Only the low 16 bits are the index.
    void DeleteExtraData(int32_t usExtraDataIndex);
    // The index is taken as int32_t so the caller's whole 32-bit value is passed on; uint16_t would be zero-extended.
    // Only the low 16 bits are the index.
    bool RemoveExtraDataAt(int32_t usExtraDataIndex);
    // Callers may pass a wider count (LinkObject passes the link count); only its low 16 bits are the size.
    bool SetExtraDataSize(uint16_t usSize);
    void RemoveAllExtraData();
    bool InsertExtraData(NiExtraData* pkExtra);

    const NiFixedString& GetName() const;
    NiTimeController* GetControllers() const;
    void SetControllers(NiTimeController* pkControl);
    void PrependController(NiTimeController* pkControl);
    NiTimeController* GetController(const NiRTTI* pkRTTI) const;
    void RemoveController(NiTimeController* pkControl);
    void RemoveAllControllers();
    // Copies the name, the cloneable extra data and the controller chain into pkDest for kCloning.
    void CopyMembers(NiObjectNET* pkDest, NiCloningProcess& kCloning);
    NiExtraData* GetExtraData(const NiFixedString& kKey) const;
    bool RemoveExtraData(const NiFixedString& kKey);
    bool AddExtraData(NiExtraData* pkExtra);
    // Adds pkExtra under the key kKey.
    bool AddExtraData(const NiFixedString& kKey, NiExtraData* pkExtra);
    // Takes a reference on kName's handle and drops the old name's.
    void SetName(const NiFixedString& strName);

    static void SetDefaultCopyType(CopyType eCopyType);
    static char GetDefaultAppendCharacter();
    // Assigns apController to a raw controller slot treated as a smart pointer (m_spControllers is kept as a raw
    // pointer): holds a reference on apController and releases the old one; returns appSlot.
    static NiTimeController** AssignControllers(NiTimeController** appSlot, NiTimeController* apController);
    static NiObjectNET::CopyType GetDefaultCopyType() {
        return NiObjectNET::ms_eCopyType;
    }

    NiFixedString m_kName;
    NiTimeController* m_spControllers;

    // Between LinkObject and PostLinkObject of a file before 5.0.0.11, the field holds the single extra datum that
    // heads the file's legacy chain instead of the array.
    union {
        NiExtraData** m_ppkExtra;
        NiExtraData* m_pkLegacyExtra;
    };

    uint16_t m_usExtraDataSize;
    uint16_t m_usMaxSize;

    static inline CopyType ms_eCopyType{};
    static NiRTTI ms_RTTI;
    static inline char ms_cAppendCharacter = 36;
    static NiCriticalSection ExtraDataCrit;
};
