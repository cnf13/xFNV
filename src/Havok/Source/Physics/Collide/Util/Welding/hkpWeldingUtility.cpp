#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"

#include <cmath>

static void cleanupLoadedObjecthkpWeldingUtility(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpWeldingUtilityTypeInfo = {"hkpWeldingUtility", nullptr, cleanupLoadedObjecthkpWeldingUtility, nullptr,
	&typeid(hkpWeldingUtility), sizeof(hkpWeldingUtility)};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enums of
// hkpWeldingUtility, as the exe's .rdata holds them.
using hkpWeldingUtilityWeldingTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpWeldingUtilityWeldingTypeEnumItemsArray hkpWeldingUtilityWeldingTypeEnumItems{
	{hkpWeldingUtility::WELDING_TYPE_ANTICLOCKWISE, "WELDING_TYPE_ANTICLOCKWISE"},
	{hkpWeldingUtility::WELDING_TYPE_CLOCKWISE, "WELDING_TYPE_CLOCKWISE"},
	{hkpWeldingUtility::WELDING_TYPE_TWO_SIDED, "WELDING_TYPE_TWO_SIDED"},
	{hkpWeldingUtility::WELDING_TYPE_NONE, "WELDING_TYPE_NONE"}};
using hkpWeldingUtilitySectorTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpWeldingUtilitySectorTypeEnumItemsArray hkpWeldingUtilitySectorTypeEnumItems{
	{hkpWeldingUtility::ACCEPT_0, "ACCEPT_0"}, {hkpWeldingUtility::SNAP_0, "SNAP_0"},
	{hkpWeldingUtility::REJECT, "REJECT"}, {hkpWeldingUtility::SNAP_1, "SNAP_1"},
	{hkpWeldingUtility::ACCEPT_1, "ACCEPT_1"}};
using hkpWeldingUtilityNumAnglesEnumItemsArray = const hkClassEnum::Item[1];
static hkpWeldingUtilityNumAnglesEnumItemsArray hkpWeldingUtilityNumAnglesEnumItems{
	{hkpWeldingUtility::NUM_ANGLES, "NUM_ANGLES"}};
hkpWeldingUtilityEnumsArray hkpWeldingUtilityEnums{
	{"WeldingType", hkpWeldingUtilityWeldingTypeEnumItems, 4, nullptr, {0}},
	{"SectorType", hkpWeldingUtilitySectorTypeEnumItems, 5, nullptr, {0}},
	{"NumAngles", hkpWeldingUtilityNumAnglesEnumItems, 1, nullptr, {0}}};

// The PDB's names: the other two enums by pointer, after WeldingType's (release hkpCollide.lib,
// hkpWeldingUtilityClass.obj: .data +0, +4, +8, then hkpWeldingUtilityTypeInfo at +0xC). Nothing in the exe names
// them.
static const hkClassEnum* hkpWeldingUtilitySectorTypeEnum = &hkpWeldingUtilityEnums[1];
static const hkClassEnum* hkpWeldingUtilityNumAnglesEnum = &hkpWeldingUtilityEnums[2];

// A zero dword no code or fixup references, between m_sinCosTable's
// .bss (hkpWeldingUtilityTableInit.obj, exactly 0x300 bytes) and hkpSimulationClass.obj's: another
// object's 4-byte .bss contribution, its module unknown.
static uint32_t data_1269510;
hkClass hkpWeldingUtilityClass("hkpWeldingUtility", nullptr, 1, nullptr, 0, hkpWeldingUtilityEnums, 3, nullptr, 0,
	nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkpWeldingUtility(void* p) {
	;
}

int32_t add1SaturateAndSetLowBit(int32_t r_eax) {
	if (r_eax != 0xFFFF) {
		++r_eax;
	}
	return r_eax | 1;
}

// For each edge angle from -pi (float pi widened) in steps of 0.2094 rad: a negative angle accepts up to the edge
// (cos 1, sin 0) and snaps between the mirrored angle less the edge snap delta and the triangle snap delta; a
// non-negative one accepts from its own angle and snaps up to pi. The intermediates are double, rounded to float
// where the exe stores them.
void hkpWeldingUtility::initWeldingTable(float edgeNormalSnapDeltaAngle, float triangleNormalSnapDeltaAngle) {
	for (int32_t i = 0; i < NUM_ANGLES; ++i) {
		SinCosTableEntry& kEntry = m_sinCosTable[i];
		float fAngle = static_cast<float>(static_cast<double>(i) * 0.20943951606750488 - 3.1415927410125732);
		if (fAngle < 0.0f) {
			float fAbs = static_cast<float>(std::fabs(static_cast<double>(fAngle)));
			float fBound = static_cast<float>(3.1415927410125732 - static_cast<double>(fAbs));
			float fSnap =
				static_cast<float>(static_cast<double>(fBound) - static_cast<double>(edgeNormalSnapDeltaAngle));
			kEntry.m_cosAccept0 = 1.0f;
			kEntry.m_sinAccept0 = 0.0f;
			if (fSnap > triangleNormalSnapDeltaAngle) {
				kEntry.m_cosSnap0 = static_cast<float>(std::cos(triangleNormalSnapDeltaAngle));
			} else {
				float fClamped = (0.0f < fSnap) ? fSnap : 0.0f;
				kEntry.m_cosSnap0 = static_cast<float>(std::cos(fClamped));
			}
			float fClamped = (fSnap > 0.0f) ? fSnap : 0.0f;
			kEntry.m_cosSnap1 = static_cast<float>(std::cos(fClamped));
			kEntry.m_cosAccept1 = static_cast<float>(std::cos(fBound));
			kEntry.m_sinAccept1 = static_cast<float>(std::sin(fBound));
		} else {
			kEntry.m_cosAccept0 = static_cast<float>(std::cos(fAngle));
			kEntry.m_sinAccept0 = static_cast<float>(std::sin(fAngle));
			double dSum = static_cast<double>(fAngle) + static_cast<double>(edgeNormalSnapDeltaAngle);
			float fSum = static_cast<float>(dSum);
			float fSnap0 = (fSum < 3.1415927410125732) ? fSum : 3.1415927f;
			kEntry.m_cosSnap0 = static_cast<float>(std::cos(fSnap0));
			float fSumRounded = static_cast<float>(dSum);
			float fUpper = static_cast<float>(3.1415927410125732 - static_cast<double>(triangleNormalSnapDeltaAngle));
			if (fUpper > fSumRounded) {
				kEntry.m_cosSnap1 = static_cast<float>(std::cos(fUpper));
			} else {
				float fSnap1 = (fSumRounded < 3.1415927410125732) ? fSumRounded : 3.1415927f;
				kEntry.m_cosSnap1 = static_cast<float>(std::cos(fSnap1));
			}
			kEntry.m_cosAccept1 = -1.0f;
			kEntry.m_sinAccept1 = 0.0f;
		}
	}
}

const hkClass& hkpWeldingUtility::staticClass() {
	return hkpWeldingUtilityClass;
}

void hkpMeshShape::assertSubpartValidity(const Subpart& part) {}
