#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

float NiPoint2::SqrLength() const {
	return x * x + y * y;
}

bool NiPoint2::operator!=(const NiPoint2& pt) const {
	return (bool)((uint8_t)(!operator==(pt)));
}

// The exe inlines both transfers (a BinaryRead with a one-component size local each).
void NiPoint2::LoadBinary(NiStream& arStream) {
	// EXE-INLINED none: NiStreamLoadBinary
	uint32_t uiSize = 4;
	arStream.Istr()->BinaryRead(&x, sizeof(x), &uiSize, 1);
	// EXE-INLINED none: NiStreamLoadBinary
	uiSize = 4;
	arStream.Istr()->BinaryRead(&y, sizeof(y), &uiSize, 1);
}

// The exe inlines both transfers (a BinaryWrite with a one-component size local each).
void NiPoint2::SaveBinary(NiStream& arStream) {
	// EXE-INLINED none: NiStreamSaveBinary
	uint32_t uiSize = 4;
	arStream.Ostr()->BinaryWrite(&x, sizeof(x), &uiSize, 1);
	// EXE-INLINED none: NiStreamSaveBinary
	uiSize = 4;
	arStream.Ostr()->BinaryWrite(&y, sizeof(y), &uiSize, 1);
}

float NiPoint2::Length() const {
	float v3 = (x * x) + (y * y);
	return NiSqrt(v3);
}

bool NiPoint2::operator==(const NiPoint2& pt) const {
	return (bool)((uint8_t)(((&pt)->x == x) && ((&pt)->y == y)));
}

float NiPoint2::Dot(const NiPoint2& pt) const {
	return (float)((x * (&pt)->x) + (y * (&pt)->y));
}

NiPoint2 NiPoint2::operator*(float fScalar) const {
	// The exe copies *this and scales the copy in place; it calls no constructor.
	NiPoint2 kResult = *this;
	kResult.x = kResult.x * fScalar;
	kResult.y = kResult.y * fScalar;
	return kResult;
}

NiPoint2 NiPoint2::operator+(const NiPoint2& pt) const {
	// The exe copies *this and adds to the copy in place; it calls no constructor.
	NiPoint2 kResult = *this;
	kResult.x = kResult.x + pt.x;
	kResult.y = kResult.y + pt.y;
	return kResult;
}

NiPoint2 NiPoint2::operator-(const NiPoint2& pt) const {
	// The exe subtracts on the x87 (53-bit) and then stores to a float.
	NiPoint2 kResult = *this;
	kResult.x = static_cast<float>(static_cast<double>(kResult.x) - pt.x);
	kResult.y = static_cast<float>(static_cast<double>(kResult.y) - pt.y);
	return kResult;
}

// The exe has operator* build the product straight into apResult.
NiPoint2* NiPoint2::Scale_(NiPoint2* apResult, float afScale, NiPoint2* apSource) {
	::new (static_cast<void*>(apResult)) NiPoint2(apSource->operator*(afScale));
	return apResult;
}

void NiPoint2::SaveBinary(NiStream& stream, NiPoint2* pkValues, uint32_t uiNumValues) {
	uint32_t v5;
	NiBinaryStream* m_pkOstr = (&stream)->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
	v5 = 4;
	m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, pkValues, 8 * uiNumValues, &v5, 1);
}
