#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlrData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiPSysEmitterCtlrData::ms_RTTI("NiPSysEmitterCtlrData", &NiObject::ms_RTTI);

// The key sizes are deliberately left unset by the constructor, as the game does.
NiPSysEmitterCtlrData::NiPSysEmitterCtlrData()
	: m_uiNumBirthRateKeys(0), m_pkBirthRateKeys(nullptr), m_eBirthRateKeyType(NiAnimationKey::NOINTERP),
	  m_uiNumEmitterActiveKeys(0), m_pkEmitterActiveKeys(nullptr), m_eEmitterActiveKeyType(NiAnimationKey::NOINTERP) {}

// Each key array is freed through the delete function of its key type (the bool keys' functions follow the float
// keys' in the table).
NiPSysEmitterCtlrData::~NiPSysEmitterCtlrData() {
	if (m_pkBirthRateKeys) {
		NiAnimationKey::ms_deletes[m_eBirthRateKeyType](m_pkBirthRateKeys);
	}
	if (m_pkEmitterActiveKeys) {
		NiAnimationKey::ms_deletes[m_eEmitterActiveKeyType + 30](m_pkEmitterActiveKeys);
	}
}

// The key counts and the float key type are read into uninitialised variables and then drive the create, fill and
// replace calls; a short read leaves them unwritten, as the game does.
void NiPSysEmitterCtlrData::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);

	uint32_t uiNumKeys;
	NiStreamLoadBinary(kStream, uiNumKeys);
	if (uiNumKeys) {
		NiFloatKey::KeyType eType;
		NiStreamLoadBinary(kStream, eType);
		uint8_t ucSize = NiFloatKey::GetKeySize(eType);
		NiFloatKey* pkKeys = static_cast<NiFloatKey*>(NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY,
			eType)(kStream, uiNumKeys));
		NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(pkKeys, uiNumKeys, ucSize);
		ReplaceBirthRateKeys(pkKeys, uiNumKeys, eType);
	}

	NiStreamLoadBinary(kStream, uiNumKeys);
	if (uiNumKeys) {
		// Files before 10.1.0.104 store no type: bool keys were always step keys.
		NiBoolKey::KeyType eType = NiAnimationKey::STEPKEY;
		if (kStream.GetFileVersion() >= 0x0A010068)
			NiStreamLoadBinary(kStream, eType);
		uint8_t ucSize = NiBoolKey::GetKeySize(eType);
		NiBoolKey* pkKeys = static_cast<NiBoolKey*>(NiAnimationKey::GetCreateFunction(NiAnimationKey::BOOLKEY,
			eType)(kStream, uiNumKeys));
		NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::BOOLKEY, eType)(pkKeys, uiNumKeys, ucSize);
		ReplaceEmitterActiveKeys(pkKeys, uiNumKeys, eType);
	}
}

void NiPSysEmitterCtlrData::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
}

bool NiPSysEmitterCtlrData::RegisterStreamables(NiStream& kStream) {
	return NiObject::RegisterStreamables(kStream);
}

void NiPSysEmitterCtlrData::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_uiNumBirthRateKeys);
	if (m_uiNumBirthRateKeys) {
		NiStreamSaveBinary(kStream, m_eBirthRateKeyType);
		NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eBirthRateKeyType)(kStream, m_pkBirthRateKeys,
			m_uiNumBirthRateKeys);
	}

	NiStreamSaveBinary(kStream, m_uiNumEmitterActiveKeys);
	// The active key type and the whole key array are written once per active key.
	for (uint32_t i = 0; i < m_uiNumEmitterActiveKeys; i++) {
		if (m_uiNumEmitterActiveKeys) {
			NiStreamSaveBinary(kStream, m_eEmitterActiveKeyType);
			NiAnimationKey::GetSaveFunction(NiAnimationKey::BOOLKEY, m_eEmitterActiveKeyType)(kStream,
				m_pkEmitterActiveKeys, m_uiNumEmitterActiveKeys);
		}
	}
}

bool NiPSysEmitterCtlrData::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject))
		return false;

	NiPSysEmitterCtlrData* pkOther = static_cast<NiPSysEmitterCtlrData*>(pkObject);
	if (pkOther->m_uiNumBirthRateKeys != m_uiNumBirthRateKeys || pkOther->m_eBirthRateKeyType != m_eBirthRateKeyType ||
		pkOther->m_uiNumEmitterActiveKeys != m_uiNumEmitterActiveKeys ||
		pkOther->m_ucBirthRateSize != m_ucBirthRateSize)
		return false;

	// The float key equal function serves both loops; the other object's key is passed first.
	NiAnimationKey::EqualFunction pfnEqual =
		NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eBirthRateKeyType);
	for (uint32_t i = 0; i < m_uiNumBirthRateKeys; i++) {
		if (!pfnEqual(*pkOther->m_pkBirthRateKeys->GetKeyAt(i, m_ucBirthRateSize),
				*m_pkBirthRateKeys->GetKeyAt(i, m_ucBirthRateSize)))
			return false;
	}
	for (uint32_t i = 0; i < m_uiNumEmitterActiveKeys; i++) {
		if (!pfnEqual(*pkOther->m_pkEmitterActiveKeys->GetKeyAt(i, m_ucEmitterActiveSize),
				*m_pkEmitterActiveKeys->GetKeyAt(i, m_ucEmitterActiveSize)))
			return false;
	}

	return true;
}

void NiPSysEmitterCtlrData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Num Birth Rate Keys", m_uiNumBirthRateKeys));

	const char* pcType;
	switch (m_eBirthRateKeyType) {
	case NiAnimationKey::LINKEY:
		pcType = "LINKEY";
		break;
	case NiAnimationKey::BEZKEY:
		pcType = "BEZKEY";
		break;
	case NiAnimationKey::TCBKEY:
		pcType = "TCBKEY";
		break;
	default:
		pcType = "Unknown";
		break;
	}
	pkStrings->Add(NiGetViewerString("Birth Rate Key Type", pcType));
	pkStrings->Add(NiGetViewerString("Num Emitter Active Keys", m_uiNumEmitterActiveKeys));
}

// Frees the current birth rate keys and takes over pkKeys; with no keys the birth rate keys are left empty.
void NiPSysEmitterCtlrData::ReplaceBirthRateKeys(NiFloatKey* pkKeys, uint32_t uiNumKeys,
	NiAnimationKey::KeyType eType) {
	if (m_pkBirthRateKeys)
		NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eBirthRateKeyType)(m_pkBirthRateKeys);

	if (pkKeys && uiNumKeys) {
		m_uiNumBirthRateKeys = uiNumKeys;
		m_pkBirthRateKeys = pkKeys;
		m_eBirthRateKeyType = eType;
		m_ucBirthRateSize = NiFloatKey::GetKeySize(eType);
	} else {
		m_uiNumBirthRateKeys = 0;
		m_pkBirthRateKeys = nullptr;
		m_eBirthRateKeyType = NiAnimationKey::NOINTERP;
		m_ucBirthRateSize = 0;
	}
}

// The old active keys are freed through the float key delete table; the empty case leaves the key type as it was.
void NiPSysEmitterCtlrData::ReplaceEmitterActiveKeys(NiBoolKey* pkKeys, uint32_t uiNumKeys,
	NiAnimationKey::KeyType eType) {
	if (m_pkEmitterActiveKeys)
		NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eEmitterActiveKeyType)(m_pkEmitterActiveKeys);

	if (pkKeys && uiNumKeys) {
		m_uiNumEmitterActiveKeys = uiNumKeys;
		m_pkEmitterActiveKeys = pkKeys;
		m_eEmitterActiveKeyType = eType;
		m_ucEmitterActiveSize = NiBoolKey::GetKeySize(eType);
	} else {
		m_uiNumEmitterActiveKeys = 0;
		m_pkEmitterActiveKeys = nullptr;
		m_ucEmitterActiveSize = 0;
	}
}

NiObject* NiPSysEmitterCtlrData::CreateObject() {
	NiPSysEmitterCtlrData* pkObject =
		static_cast<NiPSysEmitterCtlrData*>(NiMemObject::operator new(sizeof(NiPSysEmitterCtlrData)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPSysEmitterCtlrData;
}
