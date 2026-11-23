#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstddef>
#include <cstdint>
#include <new>

class NiStream;

// Base of one animation key: the time it sits at. Keys of one sequence sit back to back in an array whose stride
// depends on the key content and interpolation type.
class NiAnimationKey : public NiMemObject {
public:
    enum KeyContent {
        FLOATKEY,
        POSKEY,
        ROTKEY,
        COLORKEY,
        TEXTKEY,
        BOOLKEY,
        NUMKEYCONTENTS,
    };

    enum KeyType {
        NOINTERP,
        LINKEY,
        BEZKEY,
        TCBKEY,
        EULERKEY,
        STEPKEY,
        NUMKEYTYPES,
    };

    using InterpFunction =
        void (*)(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pvResult);
    using CreateFunction = NiAnimationKey* (*)(NiStream& kStream, uint32_t uiNumKeys);
    using SaveFunction = void (*)(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    using CurvatureFunction = float (*)(NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    using EqualFunction = bool (*)(const NiAnimationKey& kKey0, const NiAnimationKey& kKey1);
    using FillDerivedValsFunction = void (*)(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
    using DeleteFunction = void (*)(NiAnimationKey* pkKeyArray);
    // Insert takes the key array and its count by reference and returns false when the time is already taken.
    using InsertFunction = bool (*)(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    using ArrayFunction = NiAnimationKey* (*)(uint32_t uiNumKeys);
    using CopyFunction = void (*)(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);

    // Each table is indexed by content * NUMKEYTYPES + type and filled at startup.
    static constexpr uint32_t NUMKEYSLOTS = static_cast<uint32_t>(NUMKEYCONTENTS) * NUMKEYTYPES;
    typedef CurvatureFunction CurvatureFunctionTable[NUMKEYSLOTS];
    typedef uint8_t KeySizeTable[NUMKEYSLOTS];
    typedef FillDerivedValsFunction FillDerivedValsFunctionTable[NUMKEYSLOTS];
    typedef DeleteFunction DeleteFunctionTable[NUMKEYSLOTS];
    typedef SaveFunction SaveFunctionTable[NUMKEYSLOTS];
    typedef EqualFunction EqualFunctionTable[NUMKEYSLOTS];
    typedef InsertFunction InsertFunctionTable[NUMKEYSLOTS];
    typedef ArrayFunction ArrayFunctionTable[NUMKEYSLOTS];
    typedef CopyFunction CopyFunctionTable[NUMKEYSLOTS];
    typedef InterpFunction InterpFunctionTable[NUMKEYSLOTS];
    typedef CreateFunction CreateFunctionTable[NUMKEYSLOTS];

    void SetTime(float fTime) {
        m_fTime = fTime;
    }
    float GetTime() const {
        return m_fTime;
    }

    // The key uiIndex strides of ucKeySize bytes past this one.
    NiAnimationKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
        return reinterpret_cast<NiAnimationKey*>(reinterpret_cast<uintptr_t>(this) + uiIndex * ucKeySize);
    }

    void LoadBinary(NiStream& stream);

    static uint8_t GetKeySize(KeyContent eContent, KeyType eType) {
        return ms_keysizes[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static CreateFunction GetCreateFunction(KeyContent eContent, KeyType eType) {
        return ms_loaders[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static SaveFunction GetSaveFunction(KeyContent eContent, KeyType eType) {
        return ms_saves[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static EqualFunction GetEqualFunction(KeyContent eContent, KeyType eType) {
        return ms_equals[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static FillDerivedValsFunction GetFillDerivedFunction(KeyContent eContent, KeyType eType) {
        return ms_fillderived[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static InsertFunction GetInsertFunction(KeyContent eContent, KeyType eType) {
        return ms_inserts[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static ArrayFunction GetArrayFunction(KeyContent eContent, KeyType eType) {
        return ms_arrays[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static CopyFunction GetCopyFunction(KeyContent eContent, KeyType eType) {
        return ms_copies[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    // Adds keys at fStartTime and fEndTime when the array lacks them, reallocating pkKeys and updating uiNumKeys.
    static void GuaranteeKeyAtStartAndEnd(
        KeyContent eContent,
        KeyType eType,
        NiAnimationKey*& pkKeys,
        uint32_t& uiNumKeys,
        float fStartTime,
        float fEndTime
    );
    // Whether a key may go in at fTime, and the index it goes in at.
    static bool
    OkayToInsert(float fTime, const NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint32_t& uiInsertAt, uint8_t ucSize);
    // Copies the keys of eContent and eType that lie between fStartTime and fEndTime into a
    // new key array, which it returns through pkNewKeys with its count in uiNewNumKeys.
    static void CopySequence(
        KeyContent eContent,
        KeyType eType,
        const NiAnimationKey* pkOrigKeys,
        uint32_t uiOrigNumKeys,
        float fStartTime,
        float fEndTime,
        NiAnimationKey*& pkNewKeys,
        uint32_t& uiNewNumKeys
    );

    static DeleteFunction GetDeleteFunction(KeyContent eContent, KeyType eType) {
        return ms_deletes[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static InterpFunction GetInterpFunction(KeyContent eContent, KeyType eType);

    static CurvatureFunction GetCurvatureFunction(KeyContent eContent, KeyType eType) {
        return ms_curves[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static InterpFunction GetInterpD1Function(KeyContent eContent, KeyType eType) {
        return ms_interpd1s[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    static InterpFunction GetInterpD2Function(KeyContent eContent, KeyType eType) {
        return ms_interpd2s[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    }

    // NiNew T[uiNumKeys] for a key type: one MemoryManager block holding the count in a size_t cookie, then the keys.
    // The byte count is clamped to 32 bits before the cookie is added. Null when the allocation fails. Not a
    // new-expression: the key types are trivially destructible, so only the Itanium ABI would store the count.
    template <class T>
    static T* AllocateKeyArray(uint32_t uiNumKeys) {
        uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(T);
        uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
        uint32_t uiAllocSize = uiBytes > 0xFFFFFFFFu - sizeof(size_t) ? 0xFFFFFFFFu
                                                                      : static_cast<uint32_t>(uiBytes + sizeof(size_t));
        size_t* puiCookie = static_cast<size_t*>(MemoryManager::Instance().Allocate(uiAllocSize));
        if (!puiCookie) {
            return nullptr;
        }
        *puiCookie = uiNumKeys;
        return ::new (static_cast<void*>(puiCookie + 1)) T[uiNumKeys];
    }

    // NiDelete[] of an AllocateKeyArray block: no destructors run. Null is ignored.
    static void FreeKeyArray(NiAnimationKey* pkKeys) {
        if (pkKeys) {
            MemoryManager::Instance().Deallocate(reinterpret_cast<size_t*>(pkKeys) - 1);
        }
    }

    // Writes uiNumKeys 8-byte float keys, one NiFloatKey::SaveBinary (NiFloatKey::SaveBinary) each.
    static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    // Writes pkKey's m_fTime; each key type's SaveBinary starts with
    // it.
    static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
    static int32_t Equal(const float* key0, const float* key1);

    float m_fTime;

    static inline KeySizeTable ms_keysizes{};
    static inline FillDerivedValsFunctionTable ms_fillderived{};
    static inline DeleteFunctionTable ms_deletes{};
    static inline SaveFunctionTable ms_saves{};
    static inline EqualFunctionTable ms_equals{};
    static inline CurvatureFunctionTable ms_curves{};
    static inline CreateFunctionTable ms_loaders{};
    static inline InsertFunctionTable ms_inserts{};
    static inline ArrayFunctionTable ms_arrays{};
    static inline CopyFunctionTable ms_copies{};
    static inline InterpFunctionTable ms_interps{};
    // The derivative interpolation functions; NiPathInterpolator::PathSpeed reads the POSKEY row.
    static inline InterpFunctionTable ms_interpd1s{};
    // The second-derivative table, NUMKEYSLOTS entries (NiTCBPosKey's RegisterSupportedFunctions stores InterpolateD2
    // there).
    static inline InterpFunctionTable ms_interpd2s{};
};

inline NiAnimationKey::InterpFunction NiAnimationKey::GetInterpFunction(KeyContent eContent, KeyType eType) {
    return ms_interps[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
}
