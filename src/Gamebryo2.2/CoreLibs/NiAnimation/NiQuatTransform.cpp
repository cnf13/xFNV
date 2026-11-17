#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiQuatTransform::NiQuatTransform() : m_kTranslate(INVALID_TRANSLATE), m_kRotate(INVALID_ROTATE), m_fScale(-FLT_MAX) {}

// The exe copies the four components in order, each read just before its store (apRotate may overlap this).
void NiQuatTransform::SetRotate(const NiQuaternion& kRotate) {
	m_kRotate.m_fW = kRotate.m_fW;
	m_kRotate.m_fX = kRotate.m_fX;
	m_kRotate.m_fY = kRotate.m_fY;
	m_kRotate.m_fZ = kRotate.m_fZ;
	SetRotateValid(true);
}

void NiQuatTransform::SetScaleValid(bool bValid) {
	if (!bValid) {
		m_fScale = (float)(-0x1.fffffe0000000p+127f);
	}
}

bool NiQuatTransform::operator==(const NiQuatTransform& kTransform) const {
	return m_kTranslate.x == kTransform.m_kTranslate.x && m_kTranslate.y == kTransform.m_kTranslate.y &&
		m_kTranslate.z == kTransform.m_kTranslate.z && m_kRotate == kTransform.m_kRotate &&
		m_fScale == kTransform.m_fScale;
}

NiQuatTransform NiQuatTransform::HierApply(const NiQuatTransform& kTransform) const {
	NiQuatTransform result;
	double v17;
	double v18;
	double v20;
	NiQuaternion kTempRot;
	NiMatrix3 kRotate;
	float x = NiQuatTransform::INVALID_TRANSLATE.x;
	float z = NiQuatTransform::INVALID_TRANSLATE.z;
	result.m_fScale = -3.4028235e+38f;
	float y = NiQuatTransform::INVALID_TRANSLATE.y;
	float resulta = 1.0f;
	double m_fScale = this->m_fScale;
	result.m_kTranslate.x = x;
	result.m_kRotate.m_fW = NiQuatTransform::INVALID_ROTATE.m_fW;
	float m_fZ = NiQuatTransform::INVALID_ROTATE.m_fZ;
	result.m_kTranslate.y = y;
	float m_fX = NiQuatTransform::INVALID_ROTATE.m_fX;
	result.m_kTranslate.z = z;
	float m_fY = NiQuatTransform::INVALID_ROTATE.m_fY;
	result.m_kRotate.m_fX = m_fX;
	result.m_kRotate.m_fY = m_fY;
	const NiQuatTransform* v12 = &kTransform;
	result.m_kRotate.m_fZ = m_fZ;
	if (-3.4028234663852886e+38 == m_fScale || kTransform.m_fScale == -3.4028235e+38f) {
		result.m_fScale = -3.4028235e+38f;
	} else {
		resulta = kTransform.m_fScale * this->m_fScale;
		result.m_fScale = resulta;
	}
	bool v13 = m_kRotate.m_fX == -3.4028235e+38f;
	kRotate = NiMatrix3_IDENTITY;
	if (v13 || kTransform.m_kRotate.m_fX == -3.4028235e+38f) {
		v17 = -3.4028234663852886e+38;
		v18 = -3.4028234663852886e+38;
		result.m_kRotate.m_fX = -3.4028235e+38f;
	} else {
		::new (&kTempRot) NiQuaternion(m_kRotate * kTransform.m_kRotate);
		kTempRot.Normalize();
		float v14 = kTempRot.m_fX;
		float v15 = kTempRot.m_fY;
		result.m_kRotate.m_fW = kTempRot.m_fW;
		float v16 = kTempRot.m_fZ;
		result.m_kRotate.m_fX = v14;
		result.m_kRotate.m_fY = v15;
		result.m_kRotate.m_fZ = v16;
		m_kRotate.ToRotation(kRotate);
		v12 = &kTransform;
		v17 = -3.4028234663852886e+38;
		v18 = -3.4028234663852886e+38;
	}
	if (m_kTranslate.x == v18) {
		v20 = v17;
	} else {
		bool v19 = v12->m_kTranslate.x == v18;
		v20 = v17;
		if (!v19) {
			NiPoint3 kRotated = kRotate * v12->m_kTranslate;
			float v25 = resulta * kRotated.x;
			float v26 = kRotated.y * resulta;
			float v27 = resulta * kRotated.z;
			kTempRot.m_fW = m_kTranslate.x + v25;
			double v22 = m_kTranslate.y;
			result.m_kTranslate.x = kTempRot.m_fW;
			kTempRot.m_fX = v22 + v26;
			double v23 = m_kTranslate.z;
			result.m_kTranslate.y = kTempRot.m_fX;
			kTempRot.m_fY = v23 + v27;
			result.m_kTranslate.z = kTempRot.m_fY;
			return result;
		}
	}
	result.m_kTranslate.x = v20;
	return result;
}

NiQuatTransform NiQuatTransform::operator*(const NiQuatTransform& kTransform) const {
	NiQuatTransform result;
	double v12;
	double v13;
	double v17;
	NiQuaternion kTempRot;
	float x = NiQuatTransform::INVALID_TRANSLATE.x;
	float z = NiQuatTransform::INVALID_TRANSLATE.z;
	result.m_fScale = -3.4028235e+38f;
	float y = NiQuatTransform::INVALID_TRANSLATE.y;
	double m_fScale = this->m_fScale;
	result.m_kTranslate.x = x;
	result.m_kRotate.m_fW = NiQuatTransform::INVALID_ROTATE.m_fW;
	float m_fZ = NiQuatTransform::INVALID_ROTATE.m_fZ;
	result.m_kTranslate.y = y;
	float m_fX = NiQuatTransform::INVALID_ROTATE.m_fX;
	result.m_kTranslate.z = z;
	float m_fY = NiQuatTransform::INVALID_ROTATE.m_fY;
	result.m_kRotate.m_fX = m_fX;
	result.m_kRotate.m_fY = m_fY;
	result.m_kRotate.m_fZ = m_fZ;
	if (-3.4028234663852886e+38 == m_fScale || kTransform.m_fScale == -3.4028235e+38f) {
		result.m_fScale = -3.4028235e+38f;
	} else {
		result.m_fScale = kTransform.m_fScale * this->m_fScale;
	}
	if (m_kRotate.m_fX == -3.4028235e+38f || kTransform.m_kRotate.m_fX == -3.4028235e+38f) {
		v12 = -3.4028234663852886e+38;
		v13 = -3.4028234663852886e+38;
		result.m_kRotate.m_fX = -3.4028235e+38f;
	} else {
		kTempRot = m_kRotate * kTransform.m_kRotate;
		kTempRot.Normalize();
		float v11 = kTempRot.m_fX;
		v12 = -3.4028234663852886e+38;
		v13 = -3.4028234663852886e+38;
		float v14 = kTempRot.m_fY;
		result.m_kRotate.m_fW = kTempRot.m_fW;
		float v15 = kTempRot.m_fZ;
		result.m_kRotate.m_fX = v11;
		result.m_kRotate.m_fY = v14;
		result.m_kRotate.m_fZ = v15;
	}
	if (m_kTranslate.x == v13) {
		v17 = v12;
	} else {
		bool v16 = kTransform.m_kTranslate.x == v13;
		v17 = v12;
		if (!v16) {
			kTempRot.m_fW = kTransform.m_kTranslate.x + m_kTranslate.x;
			double v18 = kTransform.m_kTranslate.y;
			result.m_kTranslate.x = kTempRot.m_fW;
			kTempRot.m_fX = v18 + m_kTranslate.y;
			double v19 = kTransform.m_kTranslate.z;
			result.m_kTranslate.y = kTempRot.m_fX;
			kTempRot.m_fY = v19 + m_kTranslate.z;
			result.m_kTranslate.z = kTempRot.m_fY;
			return result;
		}
	}
	result.m_kTranslate.x = v17;
	return result;
}

// An old file's false bool marks its part invalid, whatever value was read for it. The exe reads each flag into the
// stream argument's own stack slot, which the scale read used as its size (4): a read that fills nothing leaves 4
// there (or the previous flag), a valid flag; ucValid starts as that, as NiBoolKey::LoadBinary 0xA29580 seeds its
// flag.
void NiQuatTransform::LoadBinary(NiStream& kStream) {
	m_kTranslate.LoadBinary(kStream);
	m_kRotate.LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_fScale);

	if (kStream.GetFileVersion() < 0x0A01006E) {
		uint8_t ucValid = 4;
		NiStreamLoadBinary(kStream, ucValid);
		if (!ucValid)
			m_kTranslate.x = -FLT_MAX;
		NiStreamLoadBinary(kStream, ucValid);
		if (!ucValid)
			m_kRotate.m_fX = -FLT_MAX;
		NiStreamLoadBinary(kStream, ucValid);
		if (!ucValid)
			m_fScale = -FLT_MAX;
	}
}

void NiQuatTransform::SaveBinary(NiStream& kStream) {
	m_kTranslate.SaveBinary(kStream);
	m_kRotate.SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_fScale);
}

void NiQuatTransform::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) const {
	pkStrings->Add(NiGetViewerString("NiQuatTransform"));
	if (IsTranslateValid())
		pkStrings->Add(m_kTranslate.GetViewerString("m_kTranslate"));
	if (IsRotateValid())
		pkStrings->Add(m_kRotate.GetViewerString("m_kRotate"));
	if (IsScaleValid())
		pkStrings->Add(NiGetViewerString("m_fScale", m_fScale));
}

void NiQuatTransform::SetScale(float fScale) {
	m_fScale = fScale;
	SetScaleValid(1);
}

void NiQuatTransform::SetTranslateValid(bool bValid) {
	if (!bValid) {
		m_kTranslate.x = (float)(-0x1.fffffe0000000p+127f);
	}
}

void NiQuatTransform::SetTranslate(const NiPoint3& kTranslate) {
	m_kTranslate = kTranslate;
	SetTranslateValid(1);
}

void NiQuatTransform::Invert(NiQuatTransform& kDest) const {
	NiQuaternion result;
	double v3 = -3.4028234663852886e+38;
	double v4 = -3.4028235e+38f;
	if (-3.4028235e+38f == m_fScale) {
		kDest.m_fScale = -3.4028235e+38f;
	} else {
		kDest.m_fScale = 1.0 / m_fScale;
	}
	if (m_kRotate.m_fX == -3.4028235e+38f) {
		kDest.m_kRotate.m_fX = -3.4028235e+38f;
	} else {
		v3 = -3.4028234663852886e+38;
		v4 = -3.4028235e+38f;
		kDest.m_kRotate = (result = NiQuaternion::UnitInverse(m_kRotate));
	}
	if (m_kTranslate.x == v3) {
		kDest.m_kTranslate.x = v4;
	} else {
		result.m_fW = -m_kTranslate.x;
		result.m_fX = -m_kTranslate.y;
		float m_fX = result.m_fX;
		double z = m_kTranslate.z;
		kDest.m_kTranslate.x = result.m_fW;
		kDest.m_kTranslate.y = m_fX;
		result.m_fY = -z;
		kDest.m_kTranslate.z = result.m_fY;
	}
}

void NiQuatTransform::HierInvert(NiQuatTransform& kDest) {
	NiQuaternion result;
	NiMatrix3 rot;
	float v8 = 1.0;
	double v3 = -3.4028234663852886e+38;
	double v4 = -3.4028235e+38f;
	if (-3.4028234663852886e+38 == m_fScale) {
		kDest.m_fScale = -3.4028235e+38f;
	} else {
		v8 = 1.0 / m_fScale;
		kDest.m_fScale = v8;
	}
	double m_fX = m_kRotate.m_fX;
	rot = NiMatrix3_IDENTITY;
	if (m_fX == -3.4028234663852886e+38) {
		kDest.m_kRotate.m_fX = -3.4028235e+38f;
	} else {
		kDest.m_kRotate = (result = NiQuaternion::UnitInverse(m_kRotate));
		kDest.m_kRotate.ToRotation(rot);
		v3 = -3.4028234663852886e+38;
		v4 = -3.4028235e+38f;
	}
	if (m_kTranslate.x == v3) {
		kDest.m_kTranslate.x = v4;
	} else {
		float v9 = -m_kTranslate.x;
		float v11 = -m_kTranslate.y;
		float v13 = -m_kTranslate.z;
		double v6 = v9;
		double v7 = v11;
		float v10 = rot.m_pEntry[0][2] * v13 + (rot.m_pEntry[0][0] * v9 + v11 * rot.m_pEntry[0][1]);
		float v12 = rot.m_pEntry[1][0] * v6 + rot.m_pEntry[1][1] * v11 + rot.m_pEntry[1][2] * v13;
		float v14 = v13 * rot.m_pEntry[2][2] + (v6 * rot.m_pEntry[2][0] + v7 * rot.m_pEntry[2][1]);
		result.m_fW = v10 * v8;
		kDest.m_kTranslate.x = result.m_fW;
		result.m_fX = v12 * v8;
		kDest.m_kTranslate.y = result.m_fX;
		result.m_fY = v8 * v14;
		kDest.m_kTranslate.z = result.m_fY;
	}
}
