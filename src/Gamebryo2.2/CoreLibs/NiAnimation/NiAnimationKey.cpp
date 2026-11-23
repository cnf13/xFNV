#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiEulerRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

#include <bit>
#include <cstdint>

void NiAnimationKey::LoadBinary(NiStream& stream) {
    uint32_t v4;
    NiBinaryStream* m_pkIstr = stream.Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v4 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, this, 4, &v4, 1);
}

// An Euler key guarantees the keys of each of its three float channels that has keys and resets the channel's key
// size and last index; any other key array gets keys at fStartTime and fEndTime through its insert function, unless
// it is a single key already at fStartTime.
void NiAnimationKey::GuaranteeKeyAtStartAndEnd(
    KeyContent eContent,
    KeyType eType,
    NiAnimationKey*& pkKeys,
    uint32_t& uiNumKeys,
    float fStartTime,
    float fEndTime
) {
    if (eType == EULERKEY) {
        NiEulerRotKey* pkERKey = static_cast<NiEulerRotKey*>(pkKeys);
        for (uint8_t i = 0; i < 3; i++) {
            NiAnimationKey* pkERKeys = pkERKey->m_apkKeys[i];
            uint32_t uiNumERKeys = pkERKey->m_uiNumKeys[i];
            KeyType eERType = pkERKey->m_eType[i];
            if (uiNumERKeys) {
                GuaranteeKeyAtStartAndEnd(FLOATKEY, eERType, pkERKeys, uiNumERKeys, fStartTime, fEndTime);
            }
            pkERKey->SetKeys(i, static_cast<NiFloatKey*>(pkERKeys));
            pkERKey->SetNumKeys(i, uiNumERKeys);
            pkERKey->SetType(i, eERType);
        }
        return;
    }

    InsertFunction fnInsert = ms_inserts[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
    if (uiNumKeys == 1 && pkKeys->m_fTime == fStartTime) {
        return;
    }
    fnInsert(fStartTime, pkKeys, uiNumKeys);
    fnInsert(fEndTime, pkKeys, uiNumKeys);
}

// The index of the first key not before fTime goes to uiInsertAt; a key already at fTime (or a NaN time) refuses the
// insert.
bool NiAnimationKey::OkayToInsert(
    float fTime,
    const NiAnimationKey* pkKeys,
    uint32_t uiNumKeys,
    uint32_t& uiInsertAt,
    uint8_t ucSize
) {
    bool bOkay = true;
    for (uiInsertAt = 0; uiInsertAt < uiNumKeys; uiInsertAt++) {
        const NiAnimationKey* pkKey = pkKeys->GetKeyAt(uiInsertAt, ucSize);
        if (pkKey->m_fTime < fTime) {
            continue;
        }
        if (pkKey->m_fTime > fTime) {
            break;
        }

        bOkay = false;
        break;
    }
    return bOkay;
}

// An Euler key copies each of its three float channels' sequences and builds one new Euler key from them; any other
// key array is counted between fStartTime and fEndTime, then copied into a new array with each time made relative to
// fStartTime.
void NiAnimationKey::CopySequence(
    KeyContent eContent,
    KeyType eType,
    const NiAnimationKey* pkOrigKeys,
    uint32_t uiOrigNumKeys,
    float fStartTime,
    float fEndTime,
    NiAnimationKey*& pkNewKeys,
    uint32_t& uiNewNumKeys
) {
    if (eType == EULERKEY) {
        const NiEulerRotKey* pkEulerKey = static_cast<const NiEulerRotKey*>(pkOrigKeys);
        NiAnimationKey* apkNewFloatKeys[3];
        uint32_t auiNewNumFloatKeys[3];
        KeyType aeFloatTypes[3];
        for (uint32_t i = 0; i < 3; i++) {
            NiFloatKey* pkFloatKeys = pkEulerKey->m_apkKeys[i];
            uint32_t uiNumFloatKeys = pkEulerKey->m_uiNumKeys[i];
            aeFloatTypes[i] = pkEulerKey->m_eType[i];
            CopySequence(
                FLOATKEY,
                aeFloatTypes[i],
                pkFloatKeys,
                uiNumFloatKeys,
                fStartTime,
                fEndTime,
                apkNewFloatKeys[i],
                auiNewNumFloatKeys[i]
            );
        }

        pkNewKeys = AllocateKeyArray<NiEulerRotKey>(1);
        uiNewNumKeys = 1;
        static_cast<NiEulerRotKey*>(pkNewKeys)->Initialize(
            static_cast<NiFloatKey*>(apkNewFloatKeys[0]),
            auiNewNumFloatKeys[0],
            aeFloatTypes[0],
            static_cast<NiFloatKey*>(apkNewFloatKeys[1]),
            auiNewNumFloatKeys[1],
            aeFloatTypes[1],
            static_cast<NiFloatKey*>(apkNewFloatKeys[2]),
            auiNewNumFloatKeys[2],
            aeFloatTypes[2]
        );
    }
    else {
        uint32_t uiSlot = static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType;
        uiNewNumKeys = 0;
        uint8_t ucSize = GetKeySize(eContent, eType);
        for (uint32_t uiKey = 0; uiKey < uiOrigNumKeys; uiKey++) {
            const NiAnimationKey* pkKey = pkOrigKeys->GetKeyAt(uiKey, ucSize);
            if (pkKey->GetTime() < fStartTime) {
                continue;
            }
            if (pkKey->GetTime() > fEndTime) {
                break;
            }
            uiNewNumKeys++;
        }

        if (uiNewNumKeys) {
            pkNewKeys = ms_arrays[uiSlot](uiNewNumKeys);
            CopyFunction pfnCopy = ms_copies[uiSlot];
            uiNewNumKeys = 0;
            for (uint32_t uiKey = 0; uiKey < uiOrigNumKeys; uiKey++) {
                const NiAnimationKey* pkKey = pkOrigKeys->GetKeyAt(uiKey, ucSize);
                if (pkKey->GetTime() < fStartTime) {
                    continue;
                }
                if (pkKey->GetTime() > fEndTime) {
                    break;
                }
                NiAnimationKey* pkDest = pkNewKeys->GetKeyAt(uiNewNumKeys, ucSize);
                pfnCopy(pkDest, pkKey);
                pkDest->SetTime(pkKey->GetTime() - fStartTime);
                uiNewNumKeys++;
            }
        }
        else {
            pkNewKeys = nullptr;
        }
    }
}

// The keys are 8-byte float keys, each written by the float key's SaveBinary.
void NiAnimationKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiFloatKey* pkFloatKeys = static_cast<NiFloatKey*>(pkKeys);
    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        NiFloatKey::SaveBinary(&stream, &pkFloatKeys[ui]);
    }
}

void NiAnimationKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
    uint32_t v4;
    NiBinaryStream* m_pkOstr = stream.Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v4 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, pkKey, 4, &v4, 1);
}

int32_t NiAnimationKey::Equal(const float* key0, const float* key1) {
    return (*key1) == (*key0);
}

void NiEulerRotKey::Initialize(
    NiFloatKey* pkX,
    uint32_t uiNumX,
    NiAnimationKey::KeyType eTypeX,
    NiFloatKey* pkY,
    uint32_t uiNumY,
    NiAnimationKey::KeyType eTypeY,
    NiFloatKey* pkZ,
    uint32_t uiNumZ,
    NiAnimationKey::KeyType eTypeZ
) {
    CleanUp();
    m_uiNumKeys[0] = uiNumX;
    m_eType[0] = eTypeX;
    m_apkKeys[0] = pkX;
    m_apkKeys[1] = pkY;
    m_uiNumKeys[1] = uiNumY;
    m_uiNumKeys[2] = uiNumZ;
    m_eType[1] = eTypeY;
    m_eType[2] = eTypeZ;
    m_apkKeys[2] = pkZ;
    m_uiLastIdx[0] = 0;
    m_uiLastIdx[1] = 0;
    m_uiLastIdx[2] = 0;
    // Each axis with keys gets its key type's float-key size.
    for (uint32_t i = 0; i < 3; ++i) {
        if (m_uiNumKeys[i]) {
            m_ucSizes[i] = NiAnimationKey::ms_keysizes[m_eType[i]];
        }
        else {
            m_ucSizes[i] = 0;
        }
    }
}
