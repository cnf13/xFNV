#pragma once

#include "BSCore/BSCoreMath.h"
#include "BSCore/BSCoreUtils.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSString.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/MessageHandler.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <stdint.h>

class bhkCollisionObject;
class bhkNiCollisionObject;
class BSShaderPPLightingProperty;
class hkpCollidable;
class hkpPhantom;
class hkpRigidBody;
class hkpWorldObject;
// How many collision objects the scene graph under apRoot holds, skipping the weapon bone, fade nodes or arrows
// as asked.
class NiAVObject;
// The marker remover RemoveEditorMarkers keeps in the global kMarkerRemover:
// the palette it looks markers up in and the marker node it is removing (pPalette +0x0, pRoot +0x4).
class NiAVObjectPalette;
class NiBound;
class NiCamera;
class NiColor;
class NiColorA;
class NiFixedString;
class NiGeometry;
class NiLines;
class NiNode;
// Whether apObj is a particle system or has one anywhere below it.
class NiObjectNET;
class NiPick;
class NiTriShape;
class TESForm;
class TESObjectCELL;
class TESObjectREFR;
class TESWorldSpace;

// The category of each OBJECT_TYPE.
enum OBJECT_CATEGORY_TYPE : int32_t {
    CATEGORY_ACTOR = 0,
    CATEGORY_ITEM = 1,
    CATEGORY_MAGIC = 2,
    CATEGORY_WORLD = 3,
    CATEGORY_MISC = 4,
    CATEGORY_ACTOR_DATA = 5,
    CATEGORY_EFFECT = 6,
    CATEGORY_AUDIO = 7,
    CATEGORY_COUNT = 8,
};

// The object kinds the editor and FormIDToObjectType sort base forms into.
enum OBJECT_TYPE : int32_t {
    // FormIDToObjectType's result for a form type with no object kind; FormIDToString tests for it with a signed
    // less-than compare.
    OBJECT_NONE = -1,
    OBJECT_ACTIVATOR = 0x0,
    OBJECT_ARMOR = 0x1,
    OBJECT_BOOK = 0x2,
    OBJECT_CONTAINER = 0x3,
    OBJECT_DOOR = 0x4,
    OBJECT_INGREDIENT = 0x5,
    OBJECT_LIGHT = 0x6,
    OBJECT_MISCITEM = 0x7,
    OBJECT_STATIC = 0x8,
    OBJECT_GRASS = 0x9,
    OBJECT_TREE = 0xA,
    OBJECT_WEAPON = 0xB,
    OBJECT_NPC = 0xC,
    OBJECT_CREATURE = 0xD,
    OBJECT_LEVCREATURE = 0xE,
    OBJECT_LEVCHARACTER = 0xF,
    OBJECT_SPELLMAKING = 0x10,
    OBJECT_ENCHANTING = 0x11,
    OBJECT_ALCHEMY = 0x12,
    OBJECT_LEVITEM = 0x13,
    OBJECT_KEY = 0x14,
    OBJECT_AMMO = 0x15,
    OBJECT_FLORA = 0x16,
    OBJECT_FURNITURE = 0x17,
    OBJECT_SOUND = 0x18,
    OBJECT_LANDTEXTURE = 0x19,
    OBJECT_COMBATSTYLE = 0x1A,
    OBJECT_LOADSCREEN = 0x1B,
    OBJECT_LEVSPELL = 0x1C,
    OBJECT_ANIMOBJECT = 0x1D,
    OBJECT_WATER = 0x1E,
    OBJECT_IDLEMARKER = 0x1F,
    OBJECT_EFFECTSHADER = 0x20,
    OBJECT_PROJECTILE = 0x21,
    OBJECT_TALKINGACTIVATOR = 0x22,
    OBJECT_EXPLOSION = 0x23,
    OBJECT_TEXTURESET = 0x24,
    OBJECT_DEBRIS = 0x25,
    OBJECT_MENUICON = 0x26,
    OBJECT_FORMLIST = 0x27,
    OBJECT_PERK = 0x28,
    OBJECT_BODYPARTDATA = 0x29,
    OBJECT_ADDON = 0x2A,
    OBJECT_MOVABLESTATIC = 0x2B,
    OBJECT_TERMINAL = 0x2C,
    OBJECT_PLACEABLEWATER = 0x2D,
    OBJECT_CAMERASHOT = 0x2E,
    OBJECT_IMPACTDATA = 0x2F,
    OBJECT_IMPACTDATASET = 0x30,
    OBJECT_QUEST = 0x31,
    OBJECT_PACKAGE = 0x32,
    OBJECT_VOICETYPE = 0x33,
    OBJECT_CLASS = 0x34,
    OBJECT_RACE = 0x35,
    OBJECT_EYES = 0x36,
    OBJECT_HEADPART = 0x37,
    OBJECT_FACTION = 0x38,
    OBJECT_NOTE = 0x39,
    OBJECT_WEATHER = 0x3A,
    OBJECT_CLIMATE = 0x3B,
    OBJECT_ARMOR_ADDON = 0x3C,
    OBJECT_GLOBAL = 0x3D,
    OBJECT_IMAGESPACE = 0x3E,
    OBJECT_IMAGESPACEMODIFIER = 0x3F,
    OBJECT_ENCOUNTERZONE = 0x40,
    OBJECT_MESSAGE = 0x41,
    OBJECT_CONSTRUCTIBLE = 0x42,
    OBJECT_ACOUSTICSPACE = 0x43,
    OBJECT_RAGDOLL = 0x44,
    OBJECT_SCRIPT = 0x45,
    OBJECT_MAGICEFFECT = 0x46,
    OBJECT_MUSICTYPE = 0x47,
    OBJECT_STATICCOLLECTION = 0x48,
    OBJECT_MOD = 0x49,
    OBJECT_REPUTATION = 0x4A,
    OBJECT_RECIPE = 0x4B,
    OBJECT_RECIPE_CATEGORY = 0x4C,
    OBJECT_CHIP = 0x4D,
    OBJECT_CASINO = 0x4E,
    OBJECT_LOADSCREENTYPE = 0x4F,
    OBJECT_MEDIASET = 0x50,
    OBJECT_MEDIALOCATIONCONTROLLER = 0x51,
    OBJECT_CHALLENGE = 0x52,
    OBJECT_AMMO_EFFECT = 0x53,
    OBJECT_CARAVANCARD = 0x54,
    OBJECT_CARAVANMONEY = 0x55,
    OBJECT_CARAVANDECK = 0x56,
    OBJECT_COUNT = 0x57,
};

// The language of the game's text (its values are what GetCurrentLanguage reads out of the sLanguage setting, and what
// GetLanguageString indexes PipBoyLanguages with). Not the platform's BSSysInfoSystemUtility::LANGUAGE, whose numbering
// is the Xbox's (Japanese first).
enum LANGUAGE : int32_t {
    LANGUAGE_ENGLISH = 0x0,
    LANGUAGE_GERMAN = 0x1,
    LANGUAGE_FRENCH = 0x2,
    LANGUAGE_SPANISH = 0x3,
    LANGUAGE_ITALIAN = 0x4,
    LANGUAGE_COUNT = 0x5,
};

// The smaller of a double and a float, as a double; the float wins ties and NaN compares.
double dMin(double adA, float afB);
// The heading (z rotation) aDirection points along, in radians.
float GetZAngleFromVector(const NiPoint3& aDirection);
// Wraps an angle in radians into [0, 2*pi).
float ClampAngle(float afAngle);
// The generic clamp: aMax when aVal > aMax, else aMin when aVal < aMin.
template <class T>
void BSClamp(T& aVal, T aMin, T aMax);
// The signed shortest turn from afStartAngle to afTargetAngle; afOutDir gets its direction.
float GetAngleDelta(float afStartAngle, float afTargetAngle, float& afOutDir);
// A debug line from aStart to aEnd, shaded from aStartColor to aEndColor.
NiLines* MakeLine(
    const NiPoint3& aStart,
    const NiColorA& aStartColor,
    const NiPoint3& aEnd,
    const NiColorA& aEndColor,
    bool abWithNoLightingProp
);
// A debug marker of size afDrawSize in aColor.
NiTriShape* MakeTriPoint(float afDrawSize, const NiColorA& aColor, bool abWithNoLightingProp);
// apGeom's vertices in world space, in a new array the caller frees with delete[]; null when apGeom has none.
NiPoint3* CreateWorldVertices(NiGeometry* apGeom);
// Gives apDest's vertices that meet apSource's in world space apSource's normals; returns whether any changed.
bool FixNormals(
    NiGeometry* apSource,
    NiGeometry* apDest,
    float afTolerance,
    bool abDisplayMatches,
    bool abExtrudeMatches
);
int32_t GetCollisionObjectCountInSceneGraph(
    NiAVObject* apRoot,
    bool abIgnoreWeaponBone,
    bool abIgnoreFadeNodes,
    bool abIgnoreArrows
);
// Tags apObject's scene graph with the reference id auiID.
void SetReferenceIDOnScenegraph(NiAVObject* apObject, uint32_t auiID);
// Clears the tint of the scene graph under apRoot.
void UnTintScenegraph(NiAVObject* apRoot);
// The per-pixel lighting property found in the scene graph under apNode, or null.
BSShaderPPLightingProperty* FindPerPixelLighting(NiNode* apNode);
bhkCollisionObject* FindFirstCollisionObject(NiAVObject* ap3D);
bool HasParticleSystem(NiObjectNET* apObj);
// Whether apObject's world bound lies in apCamera's frustum; false for a null object.
bool NodeInFrustum(NiAVObject* apObject, NiCamera* apCamera);
// Whether aTestLocation lies in apCamera's frustum within afDistance.
bool PointInFrustum(NiPoint3 aTestLocation, NiCamera* apCamera, float afDistance);
// Whether the segment aStartPt-aEndPt meets aTestRect; the hit nearest aStartPt goes to aOutputIntersectPt.
bool LineVsRect(
    const tagRECT& aTestRect,
    const NiPoint3& aStartPt,
    const NiPoint3& aEndPt,
    NiPoint3& aOutputIntersectPt
);
// Whether apObj has a morpher controller.
bool HasMorpherController(NiObjectNET* apObj);
// Whether a pick from aOrigin along aDir reaches apTarget within afDistance; abClearResults
// clears apPick's results first.
bool PickDistance(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    NiAVObject* apTarget,
    bool abClearResults
);
// PickDistance's recursion over apTarget's children.
bool PickDistanceRecursive(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    NiAVObject* apTarget
);
// Whether aBound lies in apCamera's frustum.
bool BoundInFrustum(const NiBound& aBound, NiCamera* apCamera);
// Always true.
bool ReturnTrue();
// Sets (abSet) or clears bit 8 of *apFlags and returns apFlags. Defined in ModelLoader.cpp.
uint8_t* MLSetFlag_8(char abSet, uint8_t* apFlags);
// Splits the location form into its cell or worldspace.
bool GetLocationCellOrWorld(TESForm* apLocation, TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld);
// The scene graph object of the collidable's owner.
NiAVObject* GetAVObjectForCollidable(const hkpCollidable* aphkCol);
// Writes the platform and language variant of texture path apInPath into apOutPath; whether
// there is one.
bool SwapPlatformLanguageTexturePath(const char* apInPath, char* apOutPath, uint32_t aiOutPathSize);
// Replaces the textures of the lighting shader properties under ap3D with their platform and language variants;
// whether any was replaced.
bool SwapPlatformLanguageTextures(NiAVObject* ap3D);
// Detaches the scabbard node under apNode; whether it found one.
bool RemoveScabard(NiNode* apNode);
// Sets the phase of every controller of apNode and of the nodes below it.
void SetPhase(NiNode* apNode, float afPhase);
// Removes the decals from every shader property under apRoot, recursing through the children.
void RecursivelyRemoveAllDecals(NiAVObject* apRoot);
// Gives every geometry under apObject without a shade property a no-lighting shader property,
// recursing through the children.
void AddNoLightingPropertyRecurse(NiAVObject* apObject);
// Prepares apObject and its children for the no-lighting shader, recursing through the
// children.
void PrepareObjectWithNoLightingRecurse(NiAVObject* apObject);
float GetUnclampedZAngleFromVector(const NiPoint3& aDirection);
// The language of the game's textures (a BSSysInfoSystemUtility::LANGUAGE).
LANGUAGE GetCurrentTexLanguage();
OBJECT_TYPE FormIDToObjectType(uint8_t acFormID);
char* FormIDToString(uint8_t acFormID);
NiTriShape* MakeQuadBox(
    const NiPoint3& aPoint1,
    const NiPoint3& aPoint2,
    const NiPoint3& aPoint3,
    const NiPoint3& aPoint4,
    const NiPoint3& aPoint5,
    const NiPoint3& aPoint6,
    const NiPoint3& aPoint7,
    const NiPoint3& aPoint8,
    const NiColorA& aColor,
    bool abWithNoLightingProp
);
NiTriShape*
MakeTriangle(NiPoint3 aPt1, NiPoint3 aPt2, NiPoint3 aPt3, const NiColorA& aColor, bool abWithNoLightingProp);
BSShaderPPLightingProperty* FindPerPixelLighting_Recurse(NiNode* apNode);
bool RemoveEditorMarkers(NiNode* apNode);
float FindTimeForNoteKey(NiAVObject* pObject, const char* pcSequenceName, const char* pcKeyName);
void TintScenegraph(NiAVObject* apRoot, NiColor& aColor, void* apTextureEffectData);
void GetCollisionObjectCount(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
int32_t GetCollisionObjectIndexInSceneGraph(
    NiAVObject* apRoot,
    bhkNiCollisionObject* apCollision,
    bool abIgnoreWeaponBone,
    bool abIgnoreFadeNodes,
    bool abIgnoreArrows
);
void GetCollisionObjectIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
bhkNiCollisionObject* GetCollisionObjectByIndexInSceneGraph(
    NiAVObject* apRoot,
    int32_t aiIndex,
    bool abIgnoreWeaponBone,
    bool abIgnoreFadeNodes,
    bool abIgnoreArrows
);
int32_t GetHavokWorldObjectCountInSceneGraph(NiAVObject* apRoot);
void GetHavokWorldObjectCount(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
int32_t GetHavokWorldObjectIndexInSceneGraph(NiAVObject* apRoot, hkpWorldObject* apWorldObject);
void GetHavokWorldObjectIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
hkpWorldObject* GetHavokWorldObjectByIndexInSceneGraph(NiAVObject* apRoot, int32_t aiIndex);
void GetHavokWorldObjectByIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
const char* GetLanguageString(LANGUAGE aeLanguage);
int32_t IsStringInteger(char* apString);
int32_t IsStringFloat(char* apString);
// Stores the Windows user name in userNameG and reports it; apAppName is WinMain's application class name.
void FillUserNameGlobal(const char* apAppName);
NiFixedString* get_g_EditorMarker();
NiFixedString* get_g_EditorMarker0();
NiFixedString* get_g_EditorMarker1();
NiFixedString* get_g_EditorMarker2();
// A bhkWorld::RecFunction (the record data by reference).
void GetCollisionObjectByIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);

// FixNormals' match for a source vertex, kept in a NiTMap<uint32_t, VertexDist> by source vertex: the
// nearest destination vertex, a second one as near (else the destination's vertex count) and their distance.
struct VertexDist {
    VertexDist() = default;
    // The value NiTMap::DeleteItem clears an item with writes nothing to the item's own value, so assigning 0 writes
    // nothing either.
    VertexDist(int32_t) {}

    VertexDist& operator=(int32_t) {
        return *this;
    }

    int32_t iVertexDest;
    int32_t iVertexDest2;
    float fDistance;
};

struct EditorMarkerRemover {
    NiAVObjectPalette* pPalette;
    NiAVObject* pRoot;
};

// The texture path's language folders, indexed by LANGUAGE.
using PipBoyLanguages_t = char* [5];
inline PipBoyLanguages_t PipBoyLanguages{
    const_cast<char*>("ENGLISH"),
    const_cast<char*>("GERMAN"),
    const_cast<char*>("FRENCH"),
    const_cast<char*>("SPANISH"),
    const_cast<char*>("ITALIAN")
};
using formString_t = char* [40];
inline formString_t formString{
    const_cast<char*>("Race"),
    const_cast<char*>("Class"),
    const_cast<char*>("Script"),
    const_cast<char*>("Dialogue Topic"),
    const_cast<char*>("Cell"),
    const_cast<char*>("Faction"),
    const_cast<char*>("Sound"),
    const_cast<char*>("Global"),
    const_cast<char*>("Region"),
    const_cast<char*>("Game Setting"),
    const_cast<char*>("Effect Setting"),
    const_cast<char*>("World Space"),
    const_cast<char*>("Climate"),
    const_cast<char*>("Weather"),
    const_cast<char*>("Skill"),
    const_cast<char*>("Quest"),
    const_cast<char*>("Head Part"),
    const_cast<char*>("Hair"),
    const_cast<char*>("Eyes"),
    const_cast<char*>("Reference"),
    const_cast<char*>("Idle Animation"),
    const_cast<char*>("AI Package"),
    const_cast<char*>("Notes"),
    const_cast<char*>("ImageSpace"),
    const_cast<char*>("ImageSpaceModifier"),
    const_cast<char*>("AcousticSpace"),
    const_cast<char*>("Music Type"),
    const_cast<char*>("Reputation"),
    const_cast<char*>("Recipe"),
    const_cast<char*>("Recipe Category"),
    const_cast<char*>("Casino Chips"),
    const_cast<char*>("Casino"),
    const_cast<char*>("Load Screen Type"),
    const_cast<char*>("Media Set"),
    const_cast<char*>("Media Location Controller"),
    const_cast<char*>("Challenge"),
    const_cast<char*>("Ammo Effect"),
    const_cast<char*>("Caravan Card"),
    const_cast<char*>("Caravan Money"),
    const_cast<char*>("Caravan Deck")
};

// The texture path's platform folders: PC, XBOX, PS3, PS3O.
using PipBoyPlatforms_t = char* [4];
inline PipBoyPlatforms_t PipBoyPlatforms{
    const_cast<char*>("PC"),
    const_cast<char*>("XBOX"),
    const_cast<char*>("PS3"),
    const_cast<char*>("PS3O")
};
inline int32_t uiLastTime = -1;
using objectLabel_t = char* [87];
inline objectLabel_t objectLabel{
    const_cast<char*>("Activator"),
    const_cast<char*>("Armor"),
    const_cast<char*>("Book"),
    const_cast<char*>("Container"),
    const_cast<char*>("Door"),
    const_cast<char*>("Ingredient"),
    const_cast<char*>("Light"),
    const_cast<char*>("Misc Item"),
    const_cast<char*>("Static"),
    const_cast<char*>("Grass"),
    const_cast<char*>("Tree"),
    const_cast<char*>("Weapon"),
    const_cast<char*>("NPC"),
    const_cast<char*>("Creature"),
    const_cast<char*>("Leveled Creature"),
    const_cast<char*>("LeveledCharacter"),
    const_cast<char*>("Actor Effect"),
    const_cast<char*>("Object Effect"),
    const_cast<char*>("Ingestible"),
    const_cast<char*>("Leveled Item"),
    const_cast<char*>("Key"),
    const_cast<char*>("Ammo"),
    const_cast<char*>("Flora"),
    const_cast<char*>("Furniture"),
    const_cast<char*>("Sound"),
    const_cast<char*>("Land Texture"),
    const_cast<char*>("Combat Style"),
    const_cast<char*>("Load Screen"),
    const_cast<char*>("Leveled Spell"),
    const_cast<char*>("Anim Object"),
    const_cast<char*>("Water Type"),
    const_cast<char*>("Idle Marker"),
    const_cast<char*>("Effect Shader"),
    const_cast<char*>("Projectile"),
    const_cast<char*>("Talking Activator"),
    const_cast<char*>("Explosion"),
    const_cast<char*>("Texture Set"),
    const_cast<char*>("Debris"),
    const_cast<char*>("Menu Icon"),
    const_cast<char*>("Form List"),
    const_cast<char*>("Perk"),
    const_cast<char*>("Body Part Data"),
    const_cast<char*>("AddOn Node"),
    const_cast<char*>("Movable Static"),
    const_cast<char*>("Terminal"),
    const_cast<char*>("Placeable Water"),
    const_cast<char*>("Camera Shot"),
    const_cast<char*>("Impact Data"),
    const_cast<char*>("Impact Data Set"),
    const_cast<char*>("Quest"),
    const_cast<char*>("Package"),
    const_cast<char*>("Voice Type"),
    const_cast<char*>("Class"),
    const_cast<char*>("Race"),
    const_cast<char*>("Eyes"),
    const_cast<char*>("Head Part"),
    const_cast<char*>("Faction"),
    const_cast<char*>("Note"),
    const_cast<char*>("Weather"),
    const_cast<char*>("Climate"),
    const_cast<char*>("Armor Addon"),
    const_cast<char*>("Global"),
    const_cast<char*>("Imagespace"),
    const_cast<char*>("Imagespace Modifier"),
    const_cast<char*>("Encounter Zone"),
    const_cast<char*>("Message"),
    const_cast<char*>("Constructible Object"),
    const_cast<char*>("Acoustic Space"),
    const_cast<char*>("Ragdoll"),
    const_cast<char*>("Script"),
    const_cast<char*>("Base Effect"),
    const_cast<char*>("Music Type"),
    const_cast<char*>("Static Collection"),
    const_cast<char*>("Item Mod"),
    const_cast<char*>("Reputation"),
    const_cast<char*>("Recipe"),
    const_cast<char*>("Recipe Category"),
    const_cast<char*>("Casino Chip"),
    const_cast<char*>("Casino"),
    const_cast<char*>("Load Screen Type"),
    const_cast<char*>("Media Set"),
    const_cast<char*>("Media Location Controller"),
    const_cast<char*>("Challenge"),
    const_cast<char*>("Ammo Effect"),
    const_cast<char*>("Caravan Card"),
    const_cast<char*>("Caravan Money"),
    const_cast<char*>("Caravan Deck")
};
using objectCategory_t = OBJECT_CATEGORY_TYPE[87];
inline objectCategory_t objectCategory{
    CATEGORY_WORLD,      CATEGORY_ITEM,       CATEGORY_ITEM,       CATEGORY_WORLD,      CATEGORY_WORLD,
    CATEGORY_ITEM,       CATEGORY_WORLD,      CATEGORY_ITEM,       CATEGORY_WORLD,      CATEGORY_WORLD,
    CATEGORY_WORLD,      CATEGORY_ITEM,       CATEGORY_ACTOR,      CATEGORY_ACTOR,      CATEGORY_ACTOR,
    CATEGORY_ACTOR,      CATEGORY_MAGIC,      CATEGORY_MAGIC,      CATEGORY_MAGIC,      CATEGORY_ITEM,
    CATEGORY_ITEM,       CATEGORY_ITEM,       CATEGORY_WORLD,      CATEGORY_WORLD,      CATEGORY_AUDIO,
    CATEGORY_MISC,       CATEGORY_MISC,       CATEGORY_MISC,       CATEGORY_MAGIC,      CATEGORY_MISC,
    CATEGORY_MISC,       CATEGORY_MISC,       CATEGORY_EFFECT,     CATEGORY_EFFECT,     CATEGORY_ACTOR,
    CATEGORY_EFFECT,     CATEGORY_MISC,       CATEGORY_EFFECT,     CATEGORY_MISC,       CATEGORY_MISC,
    CATEGORY_ACTOR_DATA, CATEGORY_ACTOR_DATA, CATEGORY_EFFECT,     CATEGORY_WORLD,      CATEGORY_WORLD,
    CATEGORY_WORLD,      CATEGORY_EFFECT,     CATEGORY_EFFECT,     CATEGORY_EFFECT,     CATEGORY_ACTOR_DATA,
    CATEGORY_ACTOR_DATA, CATEGORY_ACTOR_DATA, CATEGORY_ACTOR_DATA, CATEGORY_ACTOR_DATA, CATEGORY_ACTOR_DATA,
    CATEGORY_ACTOR_DATA, CATEGORY_ACTOR_DATA, CATEGORY_ITEM,       CATEGORY_WORLD,      CATEGORY_WORLD,
    CATEGORY_ITEM,       CATEGORY_MISC,       CATEGORY_EFFECT,     CATEGORY_EFFECT,     CATEGORY_WORLD,
    CATEGORY_MISC,       CATEGORY_ITEM,       CATEGORY_AUDIO,      CATEGORY_ACTOR_DATA, CATEGORY_MISC,
    CATEGORY_MAGIC,      CATEGORY_AUDIO,      CATEGORY_WORLD,      CATEGORY_ITEM,       CATEGORY_ACTOR_DATA,
    CATEGORY_MISC,       CATEGORY_MISC,       CATEGORY_MISC,       CATEGORY_MISC,       CATEGORY_MISC,
    CATEGORY_AUDIO,      CATEGORY_AUDIO,      CATEGORY_ACTOR_DATA, CATEGORY_MAGIC,      CATEGORY_ITEM,
    CATEGORY_ITEM,       CATEGORY_MISC
};
inline NiCamera* pLastCamera;
inline EditorMarkerRemover kMarkerRemover{};
