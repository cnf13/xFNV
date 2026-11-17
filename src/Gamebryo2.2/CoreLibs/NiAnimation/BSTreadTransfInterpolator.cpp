#include "Gamebryo2.2/CoreLibs/NiAnimation/BSTreadTransfInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"

NiRTTI BSTreadTransfInterpolator::ms_RTTI("BSTreadTransfInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

// Both transforms invalid; Name is the null string.
BSTreadTransfInterpolator::LinkInfo::LinkInfo() {
	Start.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
	Start.m_kRotate = NiQuatTransform::INVALID_ROTATE;
	Start.m_fScale = -3.4028235e+38f;
	End.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
	End.m_kRotate = NiQuatTransform::INVALID_ROTATE;
	End.m_fScale = -3.4028235e+38f;
}

BSTreadTransfInterpolator::BSTreadTransfInterpolator()
	: NiKeyBasedInterpolator(), Links(nullptr), uiNumLinks(0), fCurrLerp(0.0f), fDiffLerp(0.0f), uiLastLerpIdx(0) {}

// Links is built as LoadBinary and CopyMembers build it: the element count in a cookie before the array, the block from
// MemoryManager. spLerpData is released by its destructor.
BSTreadTransfInterpolator::~BSTreadTransfInterpolator() {
	if (Links) {
		for (uint32_t i = reinterpret_cast<size_t*>(Links)[-1]; i > 0;) {
			Links[--i].~LinkInfo();
		}
		MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(Links) - 1);
	}
}

NiObject* BSTreadTransfInterpolator::CreateClone(NiCloningProcess& kCloning) {
	BSTreadTransfInterpolator* pkObject_2 =
		static_cast<BSTreadTransfInterpolator*>(NiMemObject::operator new(sizeof(BSTreadTransfInterpolator)));
	if (pkObject_2) {
		::new (pkObject_2) BSTreadTransfInterpolator;
		CopyMembers(pkObject_2, kCloning);
		return pkObject_2;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

// Links is built as new[] builds it: the element count is stored before the array.
void BSTreadTransfInterpolator::LoadBinary(NiStream& arStream) {
	BSTreadTransfInterpolator::LinkInfo* pLinks;
	uint32_t uiComponentSize;
	NiKeyBasedInterpolator::LoadBinary(arStream);
	NiBinaryStream* m_pkIstr = arStream.Istr();
	NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = m_pkIstr->m_pfnRead;
	uiComponentSize = 4;
	m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &uiNumLinks, 4, &uiComponentSize, 1);
	uint32_t uiCount = uiNumLinks;
	size_t uiBytes = uiCount <= SIZE_MAX / sizeof(BSTreadTransfInterpolator::LinkInfo)
		? sizeof(BSTreadTransfInterpolator::LinkInfo) * uiCount
		: SIZE_MAX;
	size_t* puiCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate((static_cast<uint32_t>(uiBytes +
																   static_cast<uint32_t>(sizeof(size_t))) < uiBytes)
				? -1
				: uiBytes + sizeof(size_t)));
	if (puiCookie) {
		*puiCookie = uiCount;
		// Raw new[] memory: the LinkInfo array starts right after the element-count cookie.
		pLinks = reinterpret_cast<BSTreadTransfInterpolator::LinkInfo*>(puiCookie + 1);
		rt::eh_vector_constructor_iterator(pLinks, sizeof(BSTreadTransfInterpolator::LinkInfo),
			static_cast<int32_t>(uiCount), &fnvd::constructElement<BSTreadTransfInterpolator::LinkInfo>);
	} else {
		pLinks = 0;
	}
	Links = pLinks;
	for (uint32_t i = 0; i < uiNumLinks; ++i) {
		arStream.LoadFixedString(Links[i].Name);
		Links[i].Start.LoadBinary(arStream);
		Links[i].End.LoadBinary(arStream);
	}
	NiObject* pkLerpData = arStream.ResolveLinkID();
	spLerpData = static_cast<NiFloatData*>(pkLerpData);
}

void BSTreadTransfInterpolator::LinkObject(NiStream& arStream) {
	NiKeyBasedInterpolator::LinkObject(arStream);
}

bool BSTreadTransfInterpolator::RegisterStreamables(NiStream& arStream) {
	if (!NiKeyBasedInterpolator::RegisterStreamables(arStream))
		return false;

	for (uint32_t i = 0; i < uiNumLinks; i++)
		arStream.RegisterFixedString(Links[i].Name);
	if (spLerpData)
		spLerpData->RegisterStreamables(arStream);
	return true;
}

void BSTreadTransfInterpolator::SaveBinary(NiStream& arStream) {
	NiKeyBasedInterpolator::SaveBinary(arStream);

	NiStreamSaveBinary(arStream, uiNumLinks);
	for (uint32_t i = 0; i < uiNumLinks; i++) {
		arStream.SaveFixedString(Links[i].Name);
		Links[i].Start.SaveBinary(arStream);
		Links[i].End.SaveBinary(arStream);
	}
	arStream.SaveLinkID(spLerpData);
}

// A jump to the base body; nothing of this class is compared.
bool BSTreadTransfInterpolator::IsEqual(NiObject* apObject) {
	return NiKeyBasedInterpolator::IsEqual(apObject);
}

// Only advances the lerp: samples it at afTime and records how far it moved since the last update, wrapping past 1
// when time ran backwards (a looped cycle). The links' transforms are blended by the controller, so akValue is left
// untouched.
bool BSTreadTransfInterpolator::Update(float afTime, NiObjectNET*, NiQuatTransform&) {
	NiFloatData* pkLerpData = spLerpData;
	uint32_t uiNumKeys = pkLerpData->m_uiNumKeys;
	NiFloatKey* pkKeys = pkLerpData->m_pkKeys;
	NiFloatKey::KeyType eType = pkLerpData->m_eType;
	uint8_t ucSize = pkLerpData->m_ucKeySize;
	if (uiNumKeys) {
		uint32_t uiLastIdx = uiLastLerpIdx;
		float fLerp = NiFloatKey::GenInterp(afTime, pkKeys, eType, uiNumKeys, uiLastIdx, ucSize);
		uiLastLerpIdx = uiLastIdx;
		if (afTime < m_fLastTime && fCurrLerp > fLerp)
			fDiffLerp = (1.0f - fCurrLerp) + fLerp;
		else
			fDiffLerp = fLerp - fCurrLerp;
		fCurrLerp = fLerp;
	}

	m_fLastTime = afTime;
	return true;
}

uint32_t BSTreadTransfInterpolator::GetKeyCount(uint16_t usChannel) const {
	if (usChannel == 0 && spLerpData)
		return spLerpData->m_uiNumKeys;
	return 0;
}

NiAnimationKey::KeyType BSTreadTransfInterpolator::GetKeyType(uint16_t usChannel) const {
	if (usChannel == 0 && spLerpData)
		return spLerpData->m_eType;
	return NiAnimationKey::NOINTERP;
}

NiAnimationKey::KeyContent BSTreadTransfInterpolator::GetKeyContent(uint16_t usChannel) const {
	return usChannel == 0 ? NiAnimationKey::FLOATKEY : NiAnimationKey::NUMKEYCONTENTS;
}

NiAnimationKey* BSTreadTransfInterpolator::GetKeyArray(uint16_t usChannel) const {
	if (usChannel == 0 && spLerpData)
		return spLerpData->m_pkKeys;
	return nullptr;
}

uint8_t BSTreadTransfInterpolator::GetKeyStride(uint16_t usChannel) const {
	if (usChannel == 0 && spLerpData)
		return spLerpData->m_ucKeySize;
	return 0;
}

// Never posed: with lerp data the key count is fetched, but the answer is false either way.
bool BSTreadTransfInterpolator::GetChannelPosed(uint16_t usChannel) const {
	if (spLerpData)
		GetKeyCount(usChannel);
	return false;
}

// The destination's links are freed as delete[] does, then rebuilt by new[] and copied link by link: the name by
// NiFixedString assignment, Start and End as 32-byte copies.
void BSTreadTransfInterpolator::CopyMembers(BSTreadTransfInterpolator* pkDest, NiCloningProcess& kCloning) {
	BSTreadTransfInterpolator::LinkInfo* pNewLinks;
	BSTreadTransfInterpolator* pDest = pkDest;
	NiKeyBasedInterpolator::CopyMembers(pkDest, kCloning);
	BSTreadTransfInterpolator::LinkInfo* pOldLinks = pkDest->Links;
	if (pOldLinks) {
		BSTreadTransfInterpolator::LinkInfo* pLink =
			&pOldLinks[static_cast<int32_t>(reinterpret_cast<size_t*>(pOldLinks)[-1])];
		for (int32_t i = static_cast<int32_t>(reinterpret_cast<size_t*>(pOldLinks)[-1]) - 1; i >= 0; --i) {
			--pLink;
			// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
			NiGlobalStringTable::ReleaseHandle(pLink->Name.GetHandle());
		}
		MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pOldLinks) - 1);
	}
	pkDest->uiNumLinks = this->uiNumLinks;
	uint32_t uiNumLinks = this->uiNumLinks;
	size_t uiBytes = uiNumLinks <= SIZE_MAX / sizeof(BSTreadTransfInterpolator::LinkInfo)
		? sizeof(BSTreadTransfInterpolator::LinkInfo) * uiNumLinks
		: SIZE_MAX;
	size_t* puiNewCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate((static_cast<uint32_t>(uiBytes +
																   static_cast<uint32_t>(sizeof(size_t))) < uiBytes)
				? -1
				: uiBytes + sizeof(size_t)));
	if (puiNewCookie) {
		// Raw new[] memory: the LinkInfo array starts right after the element-count cookie.
		pNewLinks = reinterpret_cast<BSTreadTransfInterpolator::LinkInfo*>(puiNewCookie + 1);
		*puiNewCookie = uiNumLinks;
		rt::eh_vector_constructor_iterator(pNewLinks, sizeof(BSTreadTransfInterpolator::LinkInfo),
			static_cast<int32_t>(uiNumLinks), &fnvd::constructElement<BSTreadTransfInterpolator::LinkInfo>);
	} else {
		pNewLinks = 0;
	}
	pkDest->Links = pNewLinks;
	for (uint32_t i = 0; i < this->uiNumLinks; ++i) {
		NiFixedString& kDestName = pDest->Links[i].Name;
		kDestName = this->Links[i].Name;
		pkDest->Links[i].Start = this->Links[i].Start;
		pkDest->Links[i].End = this->Links[i].End;
		pDest = pkDest;
	}
	pDest->spLerpData = spLerpData;
	pDest->fCurrLerp = fCurrLerp;
	pDest->fDiffLerp = fDiffLerp;
}

NiObject* BSTreadTransfInterpolator::CreateObject() {
	BSTreadTransfInterpolator* pkObject =
		static_cast<BSTreadTransfInterpolator*>(NiMemObject::operator new(sizeof(BSTreadTransfInterpolator)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) BSTreadTransfInterpolator;
}
