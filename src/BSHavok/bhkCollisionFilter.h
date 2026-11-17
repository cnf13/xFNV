#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Reconstruction.hpp"

class NiColor;

template <class T>
void ENDIAN(T& arValue, bool abUnused);

// The collision layers: the low seven bits of a collision filter info.
enum COL_LAYER {
	// The PDB's L_UNIDENTIFIED.
	L_UNIDENTIFIED = 0x0,
	L_STATIC = 0x1,
	L_ANIMSTATIC = 0x2,
	L_TRANSPARENT = 0x3,
	L_CLUTTER = 0x4,
	L_WEAPON = 0x5,
	L_PROJECTILE = 0x6,
	L_SPELL = 0x7,
	L_BIPED = 0x8,
	L_TREES = 0x9,
	L_PROPS = 0xA,
	L_WATER = 0xB,
	L_TRIGGER = 0xC,
	L_TERRAIN = 0xD,
	L_TRAP = 0xE,
	L_NONCOLLIDABLE = 0xF,
	L_CLOUDTRAP = 0x10,
	L_GROUND = 0x11,
	L_PORTAL = 0x12,
	L_DEBRIS_SMALL = 0x13,
	L_DEBRIS_LARGE = 0x14,
	L_ACOUSTIC_SPACE = 0x15,
	L_ACTORZONE = 0x16,
	L_PROJECTILEZONE = 0x17,
	L_GASTRAP = 0x18,
	L_SHELLCASING = 0x19,
	L_TRANSPARENT_SMALL = 0x1A,
	L_INVISIBLE_WALL = 0x1B,
	L_TRANSPARENT_SMALL_ANIM = 0x1C,
	L_DEADBIP = 0x1D,
	L_CHARCONTROLLER = 0x1E,
	L_AVOIDBOX = 0x1F,
	L_COLLISIONBOX = 0x20,
	L_CAMERASPHERE = 0x21,
	L_DOORDETECTION = 0x22,
	L_CAMERA = 0x23,
	L_ITEMPICKER = 0x24,
	L_LOS = 0x25,
	L_PATHINGPICK = 0x26,
	L_CUSTOMPICK1 = 0x27,
	L_CUSTOMPICK2 = 0x28,
	L_SPELLEXPLOSION = 0x29,
	L_DROPPINGPICK = 0x2A,
	NUMFILTERLAYERS = 0x2B,
};

// The body part a filter's collision belongs to. Names from the Xbox PDB, in its order.
enum BIPED_PART : int32_t {
	P_OTHER,
	P_HEAD,
	P_BODY,
	P_SPINE1,
	P_SPINE2,
	P_LUPPERARM,
	P_LFOREARM,
	P_LHAND,
	P_LTHIGH,
	P_LCALF,
	P_LFOOT,
	P_RUPPERARM,
	P_RFOREARM,
	P_RHAND,
	P_RTHIGH,
	P_RCALF,
	P_RFOOT,
	P_TAIL,
	P_SHIELD,
	P_QUIVER,
	P_WEAPON,
	P_PONYTAIL,
	P_WING,
	P_PACK,
	P_CHAIN,
	P_ADDONHEAD,
	P_ADDONCHEST,
	P_ADDONLEG,
	P_ADDONARM,
	NUMFILTERPARTS,
};

// A Havok collision filter info word: layer, biped part, flags and collision group.
class CFilter {
public:
	CFilter() = default;
	CFilter(uint32_t iV);
	CFilter(COL_LAYER aeLayer, uint32_t aiGroup, BIPED_PART aePart);

	// iFilter.
	operator uint32_t() { return iFilter; }
	// Sets the layer, group and biped part of the filter.
	void Init(COL_LAYER aeLayer, uint32_t aiGroup, BIPED_PART aePart);
	// The layer, the low seven bits.
	COL_LAYER GetLayer();
	// The collision group, the high sixteen bits.
	uint32_t GetGroup();
	// The biped part, bits 8 to 12. Out of line in the exe (0x4D9DA0); GetBipedPart is its only src caller.
	uint32_t GetPart() const;
	// The biped part as a BIPED_PART.
	BIPED_PART GetBipedPart();
	bool GetNoCollision() const;
	void SetLayer(COL_LAYER aeLayer);
	// Sets the collision group, the high sixteen bits.
	void SetGroup(uint32_t aiGroup);
	void SetDisabled(uint8_t a2);
	// Whether the layer is one of the biped layers, LAYER_BIPED or LAYER_DEADBIP.
	bool QBipLayer();
	// Adds the filter's lines (the info word, group, layer, part and flags) to apStrings.
	void GetViewerStrings(NiTPrimitiveArray<char *>* apStrings);

	uint32_t iFilter;
};

// The game's collision filter, a pairwise layer table. It adds no data to hkpCollisionFilter; its table is static.
class bhkCollisionFilter : public hkpCollisionFilter {
public:
	// One bit per layer: ulLayerFlagA[eLayer] holds the bit of every layer eLayer collides with. IDA names it
	// bhkCollisionFilter::ulLayerFlagA; not in the PDB.
	using LayerFlagArray = uint64_t[43];
	// One bit per biped part: uiPartFlagA[part] holds the bit of every part part collides with. IDA names it
	// bhkCollisionFilter::uiPartFlagA; not in the PDB.
	using PartFlagArray = uint32_t[32];
	// One collision group per layer, which InitFlags clears and gives the terrain, tree and water layers.
	using LayerGroupArray = uint32_t[43];
	// The layer names CFilter::GetViewerStrings shows, indexed by COL_LAYER; the exe's table ends in a null entry.
	using LayerNameArray = char*[NUMFILTERLAYERS + 1];
	// The biped part names, indexed by bits 8 to 12 of a filter; the exe's table has 32 entries, the last three null.
	using BipPartNameArray = char*[32];

	~bhkCollisionFilter() override;

	hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override;
	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override;

	// Whether the collision filter infos aInfoA and aInfoB may collide.
	static bool IsCollisionEnabled(CFilter aInfoA, CFilter aInfoB);
	// Sets whether layers aeA and aeB collide (cdecl, 0xC827F0). Not inlined: InitFlags, in the same file, calls it
	// out of line about 270 times, as the exe does.
	static void SetLayerCollision(COL_LAYER aeA, COL_LAYER aeB, bool abSet);
	// Sets the layers aeA collides with to the bits of uiLayerBits, one per COL_LAYER.
	static void SetMultiLayerCollision(COL_LAYER aeA, uint64_t uiLayerBits);
	static uint32_t GetNewSystemGroup();
	// The debug display color of the layer in aiFilter's low seven bits; white for layers without one.
	static void GetDebugColor(CFilter aiFilter, NiColor& arColor);
	// Fills the static layer, part and group tables.
	static void InitFlags();
	// Returns the block to the thread's Havok heap, with the size in its m_memSizeAndFlags.
	static void operator delete(void* apMemory);
	static void SetCollisionFlags(COL_LAYER aeLayer, uint64_t aiFlags);

	static inline LayerFlagArray ulLayerFlagA{};
	static inline PartFlagArray uiPartFlagA{};
	static inline LayerGroupArray iLayerGroupA{};
	static inline LayerNameArray pLayerNameA{
		const_cast<char*>("UNIDENTIFIED"), const_cast<char*>("STATIC"), const_cast<char*>("ANIMSTATIC"),
		const_cast<char*>("TRANSPARENT"), const_cast<char*>("CLUTTER"), const_cast<char*>("WEAPON"),
		const_cast<char*>("PROJECTILE"), const_cast<char*>("SPELL"), const_cast<char*>("BIPED"), const_cast<char*>("TREES"),
		const_cast<char*>("PROPS"), const_cast<char*>("WATER"), const_cast<char*>("TRIGGER"), const_cast<char*>("TERRAIN"),
		const_cast<char*>("TRAP"), const_cast<char*>("NONCOLLIDABLE"), const_cast<char*>("CLOUDTRAP"),
		const_cast<char*>("GROUND"), const_cast<char*>("PORTAL"), const_cast<char*>("DEBRIS SMALL"),
		const_cast<char*>("DEBRIS LARGE"), const_cast<char*>("ACOUSTIC SPACE"), const_cast<char*>("ACTORZONE"),
		const_cast<char*>("PROJECTILEZONE"), const_cast<char*>("GASTRAP"), const_cast<char*>("SHELLCASING"),
		const_cast<char*>("TRANSPARENT SMALL"), const_cast<char*>("INVISIBLE WALL"),
		const_cast<char*>("TRANSPARENT SMALL ANIM"), const_cast<char*>("DEADBIP"), const_cast<char*>("CHARCONTROLLER"),
		const_cast<char*>("AVOIDBOX"), const_cast<char*>("COLLISIONBOX"), const_cast<char*>("CAMERASPHERE"),
		const_cast<char*>("DOORDETECTION"), const_cast<char*>("CAMERAPICK"), const_cast<char*>("ITEMPICK"),
		const_cast<char*>("LINEOFSIGHT"), const_cast<char*>("PATHPICK"), const_cast<char*>("CUSTOMPICK1"),
		const_cast<char*>("CUSTOMPICK2"), const_cast<char*>("SPELLEXPLOSION"), const_cast<char*>("DROPPINGPICK")};
	static inline BipPartNameArray pBipPartNameA{
		const_cast<char*>("OTHER"), const_cast<char*>("HEAD"), const_cast<char*>("BODY"), const_cast<char*>("SPINE1"),
		const_cast<char*>("SPINE2"), const_cast<char*>("LUPPERARM"), const_cast<char*>("LFOREARM"),
		const_cast<char*>("LHAND"), const_cast<char*>("LTHIGH"), const_cast<char*>("LCALF"), const_cast<char*>("LFOOT"),
		const_cast<char*>("RUPPERARM"), const_cast<char*>("RFOREARM"), const_cast<char*>("RHAND"),
		const_cast<char*>("RTHIGH"), const_cast<char*>("RCALF"), const_cast<char*>("RFOOT"), const_cast<char*>("TAIL"),
		const_cast<char*>("SHIELD"), const_cast<char*>("QUIVER"), const_cast<char*>("WEAPON"),
		const_cast<char*>("PONYTAIL"), const_cast<char*>("WING"), const_cast<char*>("PACK"), const_cast<char*>("CHAIN"),
		const_cast<char*>("ADDONHEAD"), const_cast<char*>("ADDONCHEST"), const_cast<char*>("ADDONARM"),
		const_cast<char*>("ADDONLEG"), nullptr, nullptr, nullptr};
	// The last system group handed out.
	static inline uint32_t uiNextGroup = 10;
};
inline constexpr fnvd::Vtable bhkCollisionFilter_vftable{};
inline constexpr fnvd::Vtable bhkCollisionFilter_vftable_hkpCollidableCollidableFilter{};
inline constexpr fnvd::Vtable bhkCollisionFilter_vftable_hkpShapeCollectionFilter{};
inline constexpr fnvd::Vtable bhkCollisionFilter_vftable_hkpRayShapeCollectionFilter{};
inline constexpr fnvd::Vtable bhkCollisionFilter_vftable_hkpRayCollidableFilter{};
