#include "BSHavok/bhkCollisionFilter.h"

#include "BSCore/BSString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "SDK/MSVC/string.h"

CFilter::CFilter(uint32_t iV) : iFilter(iV) {}

CFilter::CFilter(COL_LAYER aeLayer, uint32_t aiGroup, BIPED_PART aePart) {
	Init(aeLayer, aiGroup, aePart);
}

void CFilter::Init(COL_LAYER aeLayer, uint32_t aiGroup, BIPED_PART aePart) {
	iFilter = ((int32_t)aeLayer) | ((((int32_t)aePart) << 8) | (aiGroup << 16));
}

COL_LAYER CFilter::GetLayer() {
	return static_cast<COL_LAYER>(iFilter & 0x7F);
}

uint32_t CFilter::GetGroup() {
	return (iFilter >> 0x10) & 0xFFFF;
}

uint32_t CFilter::GetPart() const {
	return (iFilter >> 8) & 0x1F;
}

bool CFilter::GetNoCollision() const {
	return (iFilter & 0x4000) != 0;
}

void CFilter::SetLayer(COL_LAYER aeLayer) {
	iFilter &= (~0x7Fu);
	iFilter |= (((int32_t)aeLayer) & 127);
}

void CFilter::SetDisabled(uint8_t a2) {
	uint32_t v2;
	if (a2) {
		v2 = iFilter | 16384;
	} else {
		v2 = iFilter & (~0x4000u);
	}
	iFilter = v2;
}

// Whether the layer is one of the biped layers.
bool CFilter::QBipLayer() {
	int32_t layer = (int32_t)GetLayer();
	return (bool)((uint8_t)((layer == 8) || (layer == 29)));
}

// Adds the filter's lines to apStrings: the info word, the group, the layer, the link flag, the biped part (for the
// biped layers by name, else by number when linked) and the no-collision and scaled flags.
void CFilter::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	char acBuffer[128];
	BSsprintf(acBuffer, 128, "0x%08X", iFilter);
	char* pcString = NiGetViewerString("COLFILTER", static_cast<const char*>(acBuffer));
	apStrings->Add(pcString);

	// The group, the high sixteen bits.
	BSsprintf(acBuffer, 128, "0x%X", static_cast<uint16_t>(iFilter >> 16));
	pcString = NiGetViewerString("-GROUP", static_cast<const char*>(acBuffer));
	apStrings->Add(pcString);

	pcString = NiGetViewerString("-LAYER", static_cast<const char*>(bhkCollisionFilter::pLayerNameA[iFilter & 0x7F]));
	apStrings->Add(pcString);

	// Bit 15: linked.
	pcString = NiGetViewerString("-LINK", static_cast<bool>((iFilter >> 15) & 1));
	apStrings->Add(pcString);

	uint32_t uiFilter = iFilter;
	uint32_t uiLayer = uiFilter & 0x7F;
	if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
		pcString = NiGetViewerString("-PART",
			static_cast<const char*>(bhkCollisionFilter::pBipPartNameA[(uiFilter >> 8) & 0x1F]));
		apStrings->Add(pcString);
	} else if ((uiFilter >> 15) & 1) {
		pcString = NiGetViewerString("-PART", static_cast<uint32_t>((uiFilter >> 8) & 0x1F));
		apStrings->Add(pcString);
	}

	// Bit 14: no collision.
	pcString = NiGetViewerString("-NOCOL", static_cast<bool>((iFilter >> 14) & 1));
	apStrings->Add(pcString);

	// Bit 13: scaled.
	pcString = NiGetViewerString("-SCALED", static_cast<bool>((iFilter >> 13) & 1));
	apStrings->Add(pcString);
}

// Two collidables collide when their filter infos' layers and groups allow it.
hkBool bhkCollisionFilter::isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const {
	hkBool result;
	result.m_bool = IsCollisionEnabled(CFilter{a.m_broadPhaseHandle.m_collisionFilterInfo},
		CFilter{b.m_broadPhaseHandle.m_collisionFilterInfo});
	return result;
}

// A shape collection's children are filtered by their root collidables' filter infos, not per child.
hkBool bhkCollisionFilter::isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
	const hkpShapeContainer& bContainer, uint32_t bKey) const {
	const hkpCdBody* rootB = &b;
	while (rootB->m_parent) {
		rootB = rootB->m_parent;
	}
	const hkpCdBody* rootA = &a;
	while (rootA->m_parent) {
		rootA = rootA->m_parent;
	}
	const hkpCollidable* collidableA = static_cast<const hkpCollidable*>(rootA);
	const hkpCollidable* collidableB = static_cast<const hkpCollidable*>(rootB);
	hkBool result;
	result.m_bool = IsCollisionEnabled(CFilter{collidableA->m_broadPhaseHandle.m_collisionFilterInfo},
		CFilter{collidableB->m_broadPhaseHandle.m_collisionFilterInfo});
	return result;
}

// Two shape collections' children are filtered by the root collidables' filter infos.
hkBool bhkCollisionFilter::isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
	const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
	const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const {
	const hkpCdBody* rootA = &collectionBodyA;
	while (rootA->m_parent) {
		rootA = rootA->m_parent;
	}
	const hkpCdBody* rootB = &collectionBodyB;
	while (rootB->m_parent) {
		rootB = rootB->m_parent;
	}
	const hkpCollidable* collidableA = static_cast<const hkpCollidable*>(rootA);
	const hkpCollidable* collidableB = static_cast<const hkpCollidable*>(rootB);
	hkBool result;
	result.m_bool = IsCollisionEnabled(CFilter{collidableA->m_broadPhaseHandle.m_collisionFilterInfo},
		CFilter{collidableB->m_broadPhaseHandle.m_collisionFilterInfo});
	return result;
}

// A ray hits a collection's child when the ray's filter info and the child's allow it. The exe calls the
// container's getCollisionFilterInfo through its vtable slot 4 (this in ecx, bKey pushed), then the static
// IsCollisionEnabled.
hkBool bhkCollisionFilter::isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
	const hkpShapeContainer& bContainer, uint32_t bKey) const {
	uint32_t childInfo = bContainer.getCollisionFilterInfo(bKey);
	hkBool result;
	result.m_bool = IsCollisionEnabled(CFilter{aInput.m_filterInfo}, CFilter{childInfo});
	return result;
}

// A world ray hits a collidable when the ray's filter info and the collidable's allow it.
hkBool bhkCollisionFilter::isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const {
	hkBool result;
	result.m_bool =
		IsCollisionEnabled(CFilter{a.m_filterInfo}, CFilter{collidableB.m_broadPhaseHandle.m_collisionFilterInfo});
	return result;
}

// A filter info without a collision group (the high sixteen bits) collides with everything. The custom pick layer and
// the 0x4000 flag keep layers apart; two bipeds of one group filter by biped part, other pairs by the layer table.
bool bhkCollisionFilter::IsCollisionEnabled(CFilter aInfoA, CFilter aInfoB) {
	// The exe treats its two parameter slots as CFilter objects (lea ecx,[esp+0x10]).
	CFilter* pFilterA = &aInfoA;
	CFilter* pFilterB = &aInfoB;
	const uint32_t uiInfoA = aInfoA.iFilter;
	const uint32_t uiInfoB = aInfoB.iFilter;
	uint32_t eLayerA = uiInfoA & 0x7F;
	if (eLayerA != L_CUSTOMPICK2 && ((uiInfoA & 0x4000) != 0 || (uiInfoB & 0x4000) != 0))
		return false;
	if ((uiInfoA & 0xFFFF0000) == 0 || (uiInfoB & 0xFFFF0000) == 0)
		return true;
	uint32_t uiUniqueGroups = (uiInfoB ^ uiInfoA) & 0xFFFF0000;
	uint32_t eLayerB = uiInfoB & 0x7F;
	// The exe shifts with __allshl, which gives 0 for a count of 64 or more.
	const uint64_t ulLayerBitB = eLayerB < 64 ? 1ULL << eLayerB : 0;
	if (eLayerA == L_BIPED && eLayerB == L_BIPED) {
		if (uiUniqueGroups != 0)
			return false;
		return (uiPartFlagA[(uiInfoA >> 8) & 0x1F] & (1u << ((uiInfoB >> 8) & 0x1F))) != 0;
	}
	if (uiUniqueGroups != 0)
		return (ulLayerFlagA[eLayerA] & ulLayerBitB) != 0;
	if ((uiInfoB & uiInfoA & 0x8000) == 0) {
		if (!pFilterA->QBipLayer() || !pFilterB->QBipLayer())
			return false;
		return (uiPartFlagA[(uiInfoA >> 8) & 0x1F] & (1u << ((uiInfoB >> 8) & 0x1F))) != 0;
	}
	if ((ulLayerFlagA[eLayerA] & ulLayerBitB) == 0)
		return false;
	int32_t iPartDelta = static_cast<int32_t>((uiInfoA >> 8) & 0x1F) - static_cast<int32_t>((uiInfoB >> 8) & 0x1F);
	return (iPartDelta < 0 ? -iPartDelta : iPartDelta) != 1;
}

// Sets or clears the bit of each layer in the other's flags, so that the pair collides or not. The exe shifts with
// __allshl (the count from cl alone, 0 for a count of 64 or more) and updates each flag word's two halves
// separately; the result is the same.
void bhkCollisionFilter::SetLayerCollision(COL_LAYER aeA, COL_LAYER aeB, bool abSet) {
	const uint8_t ucShiftA = static_cast<uint8_t>(aeA);
	const uint8_t ucShiftB = static_cast<uint8_t>(aeB);
	const uint64_t ulBitA = ucShiftA < 64 ? 1ull << ucShiftA : 0;
	const uint64_t ulBitB = ucShiftB < 64 ? 1ull << ucShiftB : 0;
	if (abSet) {
		ulLayerFlagA[aeA] |= ulBitB;
		ulLayerFlagA[aeB] |= ulBitA;
	} else {
		ulLayerFlagA[aeA] &= ~ulBitB;
		ulLayerFlagA[aeB] &= ~ulBitA;
	}
}

// Gives layer aeA the flag word uiLayerBits, then sets or clears aeA's bit in each of the 43 layers' words as
// uiLayerBits has that layer's bit. The exe shifts with __allshl (the count from cl alone, 0 for a count of 64 or
// more) and updates each word's two halves separately; the result is the same.
void bhkCollisionFilter::SetMultiLayerCollision(COL_LAYER aeA, uint64_t uiLayerBits) {
	const uint8_t ucShiftA = static_cast<uint8_t>(aeA);
	const uint64_t ulBitA = ucShiftA < 64 ? 1ull << ucShiftA : 0;
	ulLayerFlagA[aeA] = uiLayerBits;
	uint64_t ulLayerBit = 1;
	for (uint32_t i = 0; i < 43; ++i) {
		if (uiLayerBits & ulLayerBit)
			ulLayerFlagA[i] |= ulBitA;
		else
			ulLayerFlagA[i] &= ~ulBitA;
		ulLayerBit <<= 1;
	}
}

void bhkCollisionFilter::GetDebugColor(CFilter aiFilter, NiColor& arColor) {
	switch (aiFilter.iFilter & 0x7F) {
	case 1:
		arColor.r = 1.0f;
		arColor.g = 0.0f;
		arColor.b = 0.0f;
		break;
	case 2:
	case 10:
		arColor.r = 0.9f;
		arColor.g = 0.24f;
		arColor.b = 0.92f;
		break;
	case 3:
		arColor.r = 0.96f;
		arColor.g = 0.8f;
		arColor.b = 0.8f;
		break;
	case 4:
		arColor.r = 0.27f;
		arColor.g = 0.41f;
		arColor.b = 0.94f;
		break;
	case 5:
		arColor.r = 0.85f;
		arColor.g = 0.61f;
		arColor.b = 0.16f;
		break;
	case 6:
		arColor.r = 0.9f;
		arColor.g = 0.73f;
		arColor.b = 0.42f;
		break;
	case 7:
		arColor.r = 0.23f;
		arColor.g = 0.97f;
		arColor.b = 0.9f;
		break;
	case 8:
		arColor.r = 0.0f;
		arColor.g = 1.0f;
		arColor.b = 0.0f;
		break;
	case 9:
		arColor.r = 0.8f;
		arColor.g = 0.62f;
		arColor.b = 0.45f;
		break;
	case 11:
		arColor.r = 0.51f;
		arColor.g = 0.98f;
		arColor.b = 0.98f;
		break;
	case 12:
	case 14:
		arColor.r = 0.73f;
		arColor.g = 0.76f;
		arColor.b = 0.75f;
		break;
	case 13:
	case 17:
		arColor.r = 0.96f;
		arColor.g = 0.97f;
		arColor.b = 0.52f;
		break;
	case 16:
		arColor.r = 0.73f;
		arColor.g = 0.78f;
		arColor.b = 0.7f;
		break;
	case 18:
		arColor.r = 0.0f;
		arColor.g = 0.7f;
		arColor.b = 0.0f;
		break;
	case 24:
		arColor.r = 0.45f;
		arColor.g = 0.6f;
		arColor.b = 0.1f;
		break;
	case 30:
		arColor.r = 1.0f;
		arColor.g = 1.0f;
		arColor.b = 0.0f;
		break;
	case 31:
		arColor.r = 1.0f;
		arColor.g = 0.8f;
		arColor.b = 0.0f;
		break;
	default:
		arColor = NiColor_WHITE;
		break;
	}
}

// Clears the part and group tables and sets every layer pair to collide, then sets the biped parts that collide
// and clears the layer pairs that don't.
void bhkCollisionFilter::InitFlags() {
	std::memset(uiPartFlagA, 0, sizeof(uiPartFlagA));
	std::memset(ulLayerFlagA, 255, sizeof(ulLayerFlagA));
	std::memset(iLayerGroupA, 0, sizeof(iLayerGroupA));
	// The exe inlines the first part and layer pair settings and folds their constant bit operations into one pass
	// over the tables (0xC8292A-0xC837E1); src writes out the folded operation on each table entry, in the
	// decomp's order.
	// EXE-INLINED 0xC827F0
	uiPartFlagA[P_HEAD] |= 0x230C0u;
	uiPartFlagA[P_BODY] |= 0x30C0u;
	uiPartFlagA[P_SPINE1] |= 0x30C0u;
	uiPartFlagA[P_SPINE2] |= 0x230C0u;
	uiPartFlagA[P_TAIL] |= 0x52u;
	uiPartFlagA[P_LUPPERARM] |= 0x3000u;
	uiPartFlagA[P_LFOOT] |= 0x80u;
	uiPartFlagA[P_LFOOT] |= 0x40000u;
	uiPartFlagA[P_RFOOT] |= 0x80u;
	uiPartFlagA[P_RCALF] |= 0x80u;
	uiPartFlagA[P_RFOOT] |= 0x40000u;
	uiPartFlagA[P_LCALF] |= 0x80u;
	uiPartFlagA[P_RCALF] |= 0x40000u;
	uiPartFlagA[P_RFOOT] |= 0x2000u;
	uiPartFlagA[P_LCALF] |= 0x40000u;
	uiPartFlagA[P_RCALF] |= 0x200u;
	uiPartFlagA[P_LCALF] |= 0x8000u;
	uiPartFlagA[P_RCALF] |= 0x400u;
	uiPartFlagA[P_LCALF] |= 0x4000u;
	uiPartFlagA[P_RCALF] |= 0x2000u;
	uiPartFlagA[P_TAIL] |= 0x1000u;
	uiPartFlagA[P_LCALF] |= 0x2000u;
	uiPartFlagA[P_RCALF] |= 0x100u;
	uiPartFlagA[P_SHIELD] |= 0x6300u;
	uiPartFlagA[P_SHIELD] |= 0x8000u;
	uiPartFlagA[P_SHIELD] |= 0x400u;
	uiPartFlagA[P_SHIELD] |= 0x10000u;
	uiPartFlagA[P_LFOOT] |= 0xC000u;
	uiPartFlagA[P_LFOOT] |= 0x2000u;
	uiPartFlagA[P_RUPPERARM] |= 0xC0u;
	uiPartFlagA[P_LCALF] |= 0x10000u;
	uiPartFlagA[P_RFOOT] |= 0x300u;
	uiPartFlagA[P_LCALF] |= 0x400000u;
	uiPartFlagA[P_LFOOT] |= 0x400000u;
	uiPartFlagA[P_RCALF] |= 0x400000u;
	uiPartFlagA[P_RFOOT] |= 0x400000u;
	uiPartFlagA[P_LUPPERARM] |= 0x400000u;
	uiPartFlagA[P_SHIELD] |= 0x400000u;
	uiPartFlagA[P_WEAPON] |= 0x400000u;
	uiPartFlagA[P_PONYTAIL] |= 0x400000u;
	uiPartFlagA[P_TAIL] |= 0x400000u;
	uiPartFlagA[P_WING] |= 0x37FFFEu;
	uiPartFlagA[P_LHAND] |= 0x41FF1Eu;
	uiPartFlagA[P_RUPPERARM] |= 0x400000u;
	uiPartFlagA[P_HEAD] |= 0x400000u;
	uiPartFlagA[P_LFOREARM] |= 0x42791Eu;
	uiPartFlagA[P_RHAND] |= 0x45C7FEu;
	uiPartFlagA[P_RFOREARM] |= 0x4241FEu;
	uiPartFlagA[P_LCALF] |= 0x1000000u;
	uiPartFlagA[P_LFOOT] |= 0x1000000u;
	uiPartFlagA[P_RCALF] |= 0x1000000u;
	uiPartFlagA[P_RFOOT] |= 0x1000000u;
	uiPartFlagA[P_SPINE1] |= 0x1400000u;
	uiPartFlagA[P_RTHIGH] |= 0x14437C0u;
	uiPartFlagA[P_CHAIN] |= 0x1C71Cu;
	ulLayerFlagA[L_TRANSPARENT] &= ~(1ull << L_DEBRIS_SMALL);
	uiPartFlagA[P_SPINE2] |= 0x1400000u;
	uiPartFlagA[P_LTHIGH] |= 0x145F0C0u;
	uiPartFlagA[P_BODY] |= 0x1400000u;
	ulLayerFlagA[L_CLOUDTRAP] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_TRANSPARENT_SMALL] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_TRANSPARENT_SMALL_ANIM] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_PROJECTILE] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_SPELL] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_TRIGGER] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_AVOIDBOX] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_CAMERA] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_CAMERASPHERE] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_ITEMPICKER] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_LOS] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_PATHINGPICK] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_DROPPINGPICK] &= ~(1ull << L_DEBRIS_SMALL);
	ulLayerFlagA[L_CLOUDTRAP] &= ~(1ull << L_DEBRIS_LARGE);
	ulLayerFlagA[L_LOS] &= ~(1ull << L_DEBRIS_LARGE);
	ulLayerFlagA[L_STATIC] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_DEBRIS_SMALL] &= ~0xD40599C8ull;
	ulLayerFlagA[L_DEBRIS_LARGE] &= ~(1ull << L_LOS);
	ulLayerFlagA[L_DEBRIS_LARGE] &= ~0x58900ull;
	ulLayerFlagA[L_DEBRIS_SMALL] &= ~0x47A00000000ull;
	ulLayerFlagA[L_ANIMSTATIC] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TRANSPARENT] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TRANSPARENT_SMALL] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TRANSPARENT_SMALL_ANIM] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_CLUTTER] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_WEAPON] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_PROJECTILE] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_SPELL] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_DEADBIP] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TREES] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_PROPS] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TRIGGER] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TERRAIN] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_TRAP] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_GROUND] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_CLOUDTRAP] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_WATER] &= ~((1ull << L_DEBRIS_SMALL) | (1ull << L_DEBRIS_LARGE) | (1ull << L_ACOUSTIC_SPACE));
	ulLayerFlagA[L_ACOUSTIC_SPACE] = (ulLayerFlagA[L_ACOUSTIC_SPACE] & ~0xFC23FFFFull) | (1ull << L_CHARCONTROLLER);
	ulLayerFlagA[L_AVOIDBOX] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_CAMERA] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_CAMERASPHERE] |= (1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_LOS] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_PATHINGPICK] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_CUSTOMPICK1] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_CUSTOMPICK2] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_SPELLEXPLOSION] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_UNIDENTIFIED] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_INVISIBLE_WALL] &= ~(1ull << L_ACOUSTIC_SPACE);
	ulLayerFlagA[L_STATIC] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_ANIMSTATIC] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_TRANSPARENT] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_TRANSPARENT_SMALL] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_TRANSPARENT_SMALL_ANIM] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_CLUTTER] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_WEAPON] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_PROJECTILE] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_SPELL] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_ACOUSTIC_SPACE] = (ulLayerFlagA[L_ACOUSTIC_SPACE] & ~0x3EA00000000ull) | (1ull << L_CAMERASPHERE);
	ulLayerFlagA[L_TREES] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_CHARCONTROLLER] =
		(ulLayerFlagA[L_CHARCONTROLLER] & ~0x202C0900ull) | ((1ull << L_WATER) | (1ull << L_ACOUSTIC_SPACE));
	ulLayerFlagA[L_PROPS] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_TRIGGER] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_TERRAIN] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_TRAP] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_GROUND] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_CLOUDTRAP] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_AVOIDBOX] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_CAMERA] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_LOS] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_PATHINGPICK] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_CUSTOMPICK1] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_CUSTOMPICK2] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_SPELLEXPLOSION] &= ~(1ull << L_PORTAL);
	ulLayerFlagA[L_PROPS] &= ~(1ull << L_BIPED);
	ulLayerFlagA[L_PORTAL] &= ~0xF41FFFFEull;
	ulLayerFlagA[L_PORTAL] &= ~0x3E800000000ull;
	ulLayerFlagA[L_STATIC] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_CLUTTER] &= ~((1ull << L_BIPED) | (1ull << L_NONCOLLIDABLE));
	ulLayerFlagA[L_ANIMSTATIC] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TRANSPARENT] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TRANSPARENT_SMALL] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TRANSPARENT_SMALL_ANIM] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_WEAPON] &= ~((1ull << L_BIPED) | (1ull << L_NONCOLLIDABLE));
	ulLayerFlagA[L_PROJECTILE] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_SPELL] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TREES] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_PROPS] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TRIGGER] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TERRAIN] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_TRAP] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_GROUND] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_BIPED] &= ~0x603C8430ull;
	ulLayerFlagA[L_DEADBIP] &=
		~((1ull << L_BIPED) | (1ull << L_NONCOLLIDABLE) | (1ull << L_PORTAL) | (1ull << L_CHARCONTROLLER));
	// As the decomp folds it: the char controller bit is cleared with the others, then set again.
	ulLayerFlagA[L_WATER] =
		(ulLayerFlagA[L_WATER] & ~((1ull << L_NONCOLLIDABLE) | (1ull << L_PORTAL) | (1ull << L_CHARCONTROLLER))) |
		(1ull << L_CHARCONTROLLER);
	ulLayerFlagA[L_CLOUDTRAP] &= ~(1ull << L_NONCOLLIDABLE);
	ulLayerFlagA[L_NONCOLLIDABLE] &= ~0x343FFFFEull;
	SetLayerCollision(L_NONCOLLIDABLE, L_CHARCONTROLLER, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_AVOIDBOX, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_CAMERA, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_LOS, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_PATHINGPICK, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_CUSTOMPICK1, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_CUSTOMPICK2, false);
	SetLayerCollision(L_NONCOLLIDABLE, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_CAMERASPHERE, L_STATIC, false);
	SetLayerCollision(L_CAMERASPHERE, L_ANIMSTATIC, false);
	SetLayerCollision(L_CAMERASPHERE, L_TRANSPARENT, false);
	SetLayerCollision(L_CAMERASPHERE, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_CAMERASPHERE, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_CAMERASPHERE, L_CLUTTER, false);
	SetLayerCollision(L_CAMERASPHERE, L_WEAPON, false);
	SetLayerCollision(L_CAMERASPHERE, L_PROJECTILE, false);
	SetLayerCollision(L_CAMERASPHERE, L_SPELL, false);
	SetLayerCollision(L_CAMERASPHERE, L_BIPED, false);
	SetLayerCollision(L_CAMERASPHERE, L_DEADBIP, false);
	SetLayerCollision(L_CAMERASPHERE, L_TREES, false);
	SetLayerCollision(L_CAMERASPHERE, L_PROPS, false);
	// The camera sphere collides with water: the exe inlines this pair's SetLayerCollision(..., true).
	// EXE-INLINED 0xC827F0
	ulLayerFlagA[L_CAMERASPHERE] |= (1ull << L_WATER);
	ulLayerFlagA[L_WATER] |= (1ull << L_CAMERASPHERE);
	SetLayerCollision(L_CAMERASPHERE, L_TRIGGER, false);
	SetLayerCollision(L_CAMERASPHERE, L_TERRAIN, false);
	SetLayerCollision(L_CAMERASPHERE, L_TRAP, false);
	SetLayerCollision(L_CAMERASPHERE, L_CAMERASPHERE, false);
	SetLayerCollision(L_CAMERASPHERE, L_GROUND, false);
	SetLayerCollision(L_CAMERASPHERE, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_CAMERASPHERE, L_CLOUDTRAP, false);
	SetLayerCollision(L_CAMERASPHERE, L_CHARCONTROLLER, false);
	SetLayerCollision(L_CAMERASPHERE, L_AVOIDBOX, false);
	SetLayerCollision(L_CAMERASPHERE, L_CAMERA, false);
	SetLayerCollision(L_CAMERASPHERE, L_LOS, false);
	SetLayerCollision(L_CAMERASPHERE, L_PATHINGPICK, false);
	SetLayerCollision(L_CAMERASPHERE, L_CUSTOMPICK1, false);
	SetLayerCollision(L_CAMERASPHERE, L_CUSTOMPICK2, false);
	SetLayerCollision(L_CAMERASPHERE, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_CAMERA, L_WEAPON, false);
	SetLayerCollision(L_CAMERA, L_SPELL, false);
	SetLayerCollision(L_CAMERA, L_TRIGGER, false);
	SetLayerCollision(L_CAMERA, L_CAMERA, false);
	SetLayerCollision(L_CAMERA, L_WATER, false);
	SetLayerCollision(L_CAMERA, L_PROJECTILE, false);
	SetLayerCollision(L_ITEMPICKER, L_SPELL, false);
	SetLayerCollision(L_ITEMPICKER, L_TRIGGER, false);
	SetLayerCollision(L_ITEMPICKER, L_WATER, false);
	SetLayerCollision(L_LOS, L_TRANSPARENT, false);
	SetLayerCollision(L_LOS, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_LOS, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_LOS, L_SPELL, false);
	SetLayerCollision(L_LOS, L_WATER, false);
	SetLayerCollision(L_LOS, L_TRIGGER, false);
	SetLayerCollision(L_LOS, L_CHARCONTROLLER, false);
	SetLayerCollision(L_PATHINGPICK, L_AVOIDBOX, false);
	SetLayerCollision(L_PATHINGPICK, L_CLUTTER, false);
	SetLayerCollision(L_PATHINGPICK, L_WEAPON, false);
	SetLayerCollision(L_PATHINGPICK, L_PROJECTILE, false);
	SetLayerCollision(L_PATHINGPICK, L_SPELL, false);
	SetLayerCollision(L_PATHINGPICK, L_BIPED, false);
	SetLayerCollision(L_PATHINGPICK, L_DEADBIP, false);
	SetLayerCollision(L_PATHINGPICK, L_CHARCONTROLLER, false);
	SetLayerCollision(L_PATHINGPICK, L_PROPS, false);
	SetLayerCollision(L_PATHINGPICK, L_CAMERA, false);
	SetLayerCollision(L_PATHINGPICK, L_TRIGGER, false);
	SetLayerCollision(L_PATHINGPICK, L_TRAP, false);
	SetLayerCollision(L_PATHINGPICK, L_CLOUDTRAP, false);
	SetLayerCollision(L_PATHINGPICK, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_PATHINGPICK, L_ITEMPICKER, false);
	SetLayerCollision(L_PATHINGPICK, L_LOS, false);
	SetLayerCollision(L_AVOIDBOX, L_AVOIDBOX, false);
	SetLayerCollision(L_AVOIDBOX, L_STATIC, false);
	SetLayerCollision(L_AVOIDBOX, L_CLUTTER, false);
	SetLayerCollision(L_AVOIDBOX, L_WEAPON, false);
	SetLayerCollision(L_AVOIDBOX, L_PROJECTILE, false);
	SetLayerCollision(L_AVOIDBOX, L_SPELL, false);
	SetLayerCollision(L_AVOIDBOX, L_BIPED, false);
	SetLayerCollision(L_AVOIDBOX, L_DEADBIP, false);
	SetLayerCollision(L_AVOIDBOX, L_TREES, false);
	SetLayerCollision(L_AVOIDBOX, L_PROPS, false);
	SetLayerCollision(L_AVOIDBOX, L_WATER, false);
	SetLayerCollision(L_AVOIDBOX, L_TERRAIN, false);
	SetLayerCollision(L_AVOIDBOX, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_AVOIDBOX, L_GROUND, false);
	SetLayerCollision(L_AVOIDBOX, L_CAMERA, false);
	SetLayerCollision(L_AVOIDBOX, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_AVOIDBOX, L_ITEMPICKER, false);
	SetLayerCollision(L_AVOIDBOX, L_LOS, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_TRANSPARENT, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_SPELL, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_WATER, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_CAMERA, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_ITEMPICKER, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_LOS, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_CUSTOMPICK1, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_CUSTOMPICK2, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_PATHINGPICK, false);
	SetLayerCollision(L_SPELLEXPLOSION, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_PROJECTILE, L_SPELL, false);
	SetLayerCollision(L_PROJECTILE, L_CLOUDTRAP, false);
	SetLayerCollision(L_PROJECTILE, L_TRANSPARENT, false);
	SetLayerCollision(L_TRIGGER, L_STATIC, false);
	SetLayerCollision(L_TRIGGER, L_ANIMSTATIC, false);
	SetLayerCollision(L_TRIGGER, L_TRANSPARENT, false);
	SetLayerCollision(L_TRIGGER, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_TRIGGER, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_TRIGGER, L_TREES, false);
	SetLayerCollision(L_TRIGGER, L_PROPS, false);
	SetLayerCollision(L_TRIGGER, L_WATER, false);
	SetLayerCollision(L_TRIGGER, L_TRIGGER, false);
	SetLayerCollision(L_TRIGGER, L_TERRAIN, false);
	SetLayerCollision(L_TRIGGER, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_TRIGGER, L_CLOUDTRAP, false);
	SetLayerCollision(L_TRIGGER, L_GROUND, false);
	SetLayerCollision(L_TRIGGER, L_ACTORZONE, false);
	SetLayerCollision(L_TRIGGER, L_COLLISIONBOX, false);
	SetLayerCollision(L_TRIGGER, L_PATHINGPICK, false);
	SetLayerCollision(L_TRIGGER, L_DROPPINGPICK, false);
	SetLayerCollision(L_TRIGGER, L_LOS, false);
	SetLayerCollision(L_TRIGGER, L_ITEMPICKER, false);
	SetLayerCollision(L_TRIGGER, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_ACTORZONE, L_STATIC, false);
	SetLayerCollision(L_ACTORZONE, L_ANIMSTATIC, false);
	SetLayerCollision(L_ACTORZONE, L_TRANSPARENT, false);
	SetLayerCollision(L_ACTORZONE, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_ACTORZONE, L_CLUTTER, false);
	SetLayerCollision(L_ACTORZONE, L_WEAPON, false);
	SetLayerCollision(L_ACTORZONE, L_PROJECTILE, false);
	SetLayerCollision(L_ACTORZONE, L_SPELL, false);
	SetLayerCollision(L_ACTORZONE, L_BIPED, false);
	SetLayerCollision(L_ACTORZONE, L_DEADBIP, false);
	SetLayerCollision(L_ACTORZONE, L_TREES, false);
	SetLayerCollision(L_ACTORZONE, L_PROPS, false);
	SetLayerCollision(L_ACTORZONE, L_WATER, false);
	SetLayerCollision(L_ACTORZONE, L_TRIGGER, false);
	SetLayerCollision(L_ACTORZONE, L_TERRAIN, false);
	SetLayerCollision(L_ACTORZONE, L_TRAP, false);
	SetLayerCollision(L_ACTORZONE, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_ACTORZONE, L_CLOUDTRAP, false);
	SetLayerCollision(L_ACTORZONE, L_GROUND, false);
	SetLayerCollision(L_ACTORZONE, L_PORTAL, false);
	SetLayerCollision(L_ACTORZONE, L_DEBRIS_SMALL, false);
	SetLayerCollision(L_ACTORZONE, L_DEBRIS_LARGE, false);
	SetLayerCollision(L_ACTORZONE, L_ACTORZONE, false);
	SetLayerCollision(L_ACTORZONE, L_PROJECTILEZONE, false);
	SetLayerCollision(L_ACTORZONE, L_SHELLCASING, false);
	SetLayerCollision(L_ACTORZONE, L_AVOIDBOX, false);
	SetLayerCollision(L_ACTORZONE, L_COLLISIONBOX, false);
	SetLayerCollision(L_ACTORZONE, L_CAMERASPHERE, false);
	SetLayerCollision(L_ACTORZONE, L_CAMERA, false);
	SetLayerCollision(L_ACTORZONE, L_ITEMPICKER, false);
	SetLayerCollision(L_ACTORZONE, L_LOS, false);
	SetLayerCollision(L_ACTORZONE, L_PATHINGPICK, false);
	SetLayerCollision(L_ACTORZONE, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_ACTORZONE, L_DROPPINGPICK, false);
	SetLayerCollision(L_ACTORZONE, L_INVISIBLE_WALL, false);
	SetLayerCollision(L_ACTORZONE, L_DOORDETECTION, false);
	SetLayerCollision(L_ACTORZONE, L_UNIDENTIFIED, false);
	SetLayerCollision(L_PROJECTILEZONE, L_STATIC, false);
	SetLayerCollision(L_PROJECTILEZONE, L_ANIMSTATIC, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TRANSPARENT, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_PROJECTILEZONE, L_CLUTTER, false);
	SetLayerCollision(L_PROJECTILEZONE, L_WEAPON, false);
	SetLayerCollision(L_PROJECTILEZONE, L_SPELL, false);
	SetLayerCollision(L_PROJECTILEZONE, L_BIPED, false);
	SetLayerCollision(L_PROJECTILEZONE, L_DEADBIP, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TREES, false);
	SetLayerCollision(L_PROJECTILEZONE, L_PROPS, false);
	SetLayerCollision(L_PROJECTILEZONE, L_WATER, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TRIGGER, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TERRAIN, false);
	SetLayerCollision(L_PROJECTILEZONE, L_TRAP, false);
	SetLayerCollision(L_PROJECTILEZONE, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_PROJECTILEZONE, L_CLOUDTRAP, false);
	SetLayerCollision(L_PROJECTILEZONE, L_GROUND, false);
	SetLayerCollision(L_PROJECTILEZONE, L_PORTAL, false);
	SetLayerCollision(L_PROJECTILEZONE, L_DEBRIS_SMALL, false);
	SetLayerCollision(L_PROJECTILEZONE, L_DEBRIS_LARGE, false);
	SetLayerCollision(L_PROJECTILEZONE, L_ACTORZONE, false);
	SetLayerCollision(L_PROJECTILEZONE, L_PROJECTILEZONE, false);
	SetLayerCollision(L_PROJECTILEZONE, L_SHELLCASING, false);
	SetLayerCollision(L_PROJECTILEZONE, L_CHARCONTROLLER, false);
	SetLayerCollision(L_PROJECTILEZONE, L_AVOIDBOX, false);
	SetLayerCollision(L_PROJECTILEZONE, L_COLLISIONBOX, false);
	SetLayerCollision(L_PROJECTILEZONE, L_CAMERASPHERE, false);
	SetLayerCollision(L_PROJECTILEZONE, L_CAMERA, false);
	SetLayerCollision(L_PROJECTILEZONE, L_ITEMPICKER, false);
	SetLayerCollision(L_PROJECTILEZONE, L_LOS, false);
	SetLayerCollision(L_PROJECTILEZONE, L_PATHINGPICK, false);
	SetLayerCollision(L_PROJECTILEZONE, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_PROJECTILEZONE, L_DROPPINGPICK, false);
	SetLayerCollision(L_GASTRAP, L_STATIC, false);
	SetLayerCollision(L_GASTRAP, L_ANIMSTATIC, false);
	SetLayerCollision(L_GASTRAP, L_TRANSPARENT, false);
	SetLayerCollision(L_GASTRAP, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_GASTRAP, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_GASTRAP, L_CLUTTER, false);
	SetLayerCollision(L_GASTRAP, L_WEAPON, false);
	SetLayerCollision(L_GASTRAP, L_SPELL, false);
	SetLayerCollision(L_GASTRAP, L_BIPED, false);
	SetLayerCollision(L_GASTRAP, L_DEADBIP, false);
	SetLayerCollision(L_GASTRAP, L_TREES, false);
	SetLayerCollision(L_GASTRAP, L_PROPS, false);
	SetLayerCollision(L_GASTRAP, L_WATER, false);
	SetLayerCollision(L_GASTRAP, L_TRIGGER, false);
	SetLayerCollision(L_GASTRAP, L_TERRAIN, false);
	SetLayerCollision(L_GASTRAP, L_TRAP, false);
	SetLayerCollision(L_GASTRAP, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_GASTRAP, L_CLOUDTRAP, false);
	SetLayerCollision(L_GASTRAP, L_GROUND, false);
	SetLayerCollision(L_GASTRAP, L_PORTAL, false);
	SetLayerCollision(L_GASTRAP, L_DEBRIS_SMALL, false);
	SetLayerCollision(L_GASTRAP, L_DEBRIS_LARGE, false);
	SetLayerCollision(L_GASTRAP, L_ACOUSTIC_SPACE, false);
	SetLayerCollision(L_GASTRAP, L_ACTORZONE, false);
	SetLayerCollision(L_GASTRAP, L_PROJECTILEZONE, false);
	SetLayerCollision(L_GASTRAP, L_GASTRAP, false);
	SetLayerCollision(L_GASTRAP, L_SHELLCASING, false);
	SetLayerCollision(L_GASTRAP, L_AVOIDBOX, false);
	SetLayerCollision(L_GASTRAP, L_COLLISIONBOX, false);
	SetLayerCollision(L_GASTRAP, L_CAMERASPHERE, false);
	SetLayerCollision(L_GASTRAP, L_CAMERA, false);
	SetLayerCollision(L_GASTRAP, L_ITEMPICKER, false);
	SetLayerCollision(L_GASTRAP, L_LOS, false);
	SetLayerCollision(L_GASTRAP, L_PATHINGPICK, false);
	SetLayerCollision(L_GASTRAP, L_DROPPINGPICK, false);
	SetLayerCollision(L_SHELLCASING, L_TRANSPARENT, false);
	SetLayerCollision(L_SHELLCASING, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_SHELLCASING, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_SHELLCASING, L_PROJECTILE, false);
	SetLayerCollision(L_SHELLCASING, L_SPELL, false);
	SetLayerCollision(L_SHELLCASING, L_BIPED, false);
	SetLayerCollision(L_SHELLCASING, L_TRIGGER, false);
	SetLayerCollision(L_SHELLCASING, L_NONCOLLIDABLE, false);
	SetLayerCollision(L_SHELLCASING, L_CLOUDTRAP, false);
	SetLayerCollision(L_SHELLCASING, L_PORTAL, false);
	SetLayerCollision(L_SHELLCASING, L_CHARCONTROLLER, false);
	SetLayerCollision(L_SHELLCASING, L_AVOIDBOX, false);
	SetLayerCollision(L_SHELLCASING, L_CAMERA, false);
	SetLayerCollision(L_SHELLCASING, L_ITEMPICKER, false);
	SetLayerCollision(L_SHELLCASING, L_LOS, false);
	SetLayerCollision(L_SHELLCASING, L_PATHINGPICK, false);
	SetLayerCollision(L_SHELLCASING, L_DROPPINGPICK, false);
	SetLayerCollision(L_SHELLCASING, L_WATER, false);
	SetLayerCollision(L_SPELL, L_SPELL, false);
	SetLayerCollision(L_SPELL, L_CLOUDTRAP, false);
	SetLayerCollision(L_SPELL, L_TRANSPARENT, false);
	SetLayerCollision(L_SPELL, L_TRANSPARENT_SMALL, false);
	SetLayerCollision(L_SPELL, L_TRANSPARENT_SMALL_ANIM, false);
	SetLayerCollision(L_DROPPINGPICK, L_CLUTTER, false);
	SetLayerCollision(L_DROPPINGPICK, L_PROJECTILE, false);
	SetLayerCollision(L_DROPPINGPICK, L_WEAPON, false);
	SetLayerCollision(L_DROPPINGPICK, L_WATER, false);
	SetLayerCollision(L_DROPPINGPICK, L_DEADBIP, false);
	SetLayerCollision(L_DROPPINGPICK, L_AVOIDBOX, false);
	SetLayerCollision(L_DROPPINGPICK, L_PATHINGPICK, false);
	SetLayerCollision(L_DROPPINGPICK, L_CAMERA, false);
	SetLayerCollision(L_DROPPINGPICK, L_SPELLEXPLOSION, false);
	SetLayerCollision(L_DROPPINGPICK, L_LOS, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_DEBRIS_SMALL, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_SHELLCASING, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_AVOIDBOX, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_COLLISIONBOX, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_CAMERASPHERE, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_CAMERA, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_LOS, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_DROPPINGPICK, false);
	SetLayerCollision(L_INVISIBLE_WALL, L_WEAPON, false);
	SetLayerCollision(L_TRANSPARENT, L_UNIDENTIFIED, false);
	SetLayerCollision(L_TRANSPARENT, L_TERRAIN, false);
	SetMultiLayerCollision(L_DOORDETECTION, (1ull << L_ANIMSTATIC) | (1ull << L_TRANSPARENT_SMALL_ANIM));
	iLayerGroupA[L_TERRAIN] = 2;
	iLayerGroupA[L_TREES] = 3;
	iLayerGroupA[L_WATER] = 4;
}
