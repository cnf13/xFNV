#!/usr/bin/env python3
"""Check reconstructed vtables against the exe's (hybrid build's clang-cl, MSVC ABI).

Usage: env/venv/bin/python harness/tools/vtable-check.py [-v] [--family A,B,...] [--all [--jobs N] [--tsv FILE]
       [--skipped FILE]] [BUILD]
  BUILD: a configured hybrid build folder whose compile_commands.json gives the flags (default build/hybrid)
  --family: check only these classes (any subtree; bases outside the list contribute only their exe entries); with
    --all, the whole --all families holding them
  --all: every class with an exe vtable and a src definition instead of FAMILIES (see all_families), the families
    checked by N processes at once (default 8). A class or family the tool cannot place or lay out is reported as
    skipped (not a mismatch) and the run goes on.
  --tsv FILE: one row per finding (class, family, folder, vtable, slot, kind, exe entry, src overrider, fold, detail);
    --skipped FILE: one row per skipped class (class, family, reason)

For every class family in FAMILIES (any order; each class's primary base comes from its RTTI) the classes' headers are
compiled with the hybrid build's flags and -fdump-vtable-layouts, and each class's primary vtable as the compiler lays
it out is compared with the exe's (found through the class's RTTI; for the classes of Bethesda's shader library, built
without RTTI, through the IDA database's `X::`vftable'` names, with the base their src header declares):
  - the number of slots;
  - the owner of each slot's final overrider: the deepest class of the chain whose exe entry differs from its
    parent's (a class that overrides in the exe must override in src, and only those);
  - each slot's parameter stack bytes (plus 4 for a hidden result pointer) against `ret N` of the class's own entry;
  - FNVD_REPLACES_VIRTUAL(address, Class, member): the address is Class's exe entry in a slot of Class::member (for
    an override only a secondary vtable holds, the body that slot's adjustor thunk jumps to);
  - FNVD_REPLACES_VIRTUAL_VIA(address, Class, Base, member): the address is Class's own entry, without an adjustor
    thunk, in its secondary vtable for the Base subobject (the exe passes that subobject as this); such an entry
    registered with plain FNVD_REPLACES_VIRTUAL is an error;
  - FNVD_REPLACES(address, Class::member) on an address that is one of Class's own vtable entries: a virtual body
    must be a virtual member (the shim skips vcall thunks), registered through FNVD_REPLACES_VIRTUAL.

Secondary vtables (a class with polymorphic bases besides its primary chain, e.g. hkpCharacterProxy's
hkpEntityListener at 8): clang prints one per base subobject (`VFTable for 'B' in 'D' in 'C'`, the path from C down to
the base whose vfptr it is); clang's record layout (-fdump-record-layouts) gives each path class's offset in C, and the
exe table is the one whose RTTI complete object locator names C with the same subobject offset. Reported as `C+0xO`:
  - each class of the path placed where the exe's RTTI base class array places it, each a direct base there of the
    one before (a class the exe has in between is a mismatch), and an exe table at that offset;
  - the number of slots; the owner of each slot along the path: C's entry against the next path class's table for
    the same subobject (its secondary table, or its primary one once the subobject is at its offset 0); purecall;
  - each entry's this adjustment (clang's `[this adjustment: -N non-virtual]`) against the exe entry's adjustor thunk
    (`sub ecx, N` / `jmp`, a stdcall's `sub dword ptr [esp + 4], N` / `jmp`; none: 0), and the stack bytes of C's
    own overrides against `ret N` of the thunk's target.
An exe secondary vtable whose base src does not declare polymorphic yet is a warning (unchecked); so are the
secondary vtables of a class without RTTI. Every class of a secondary table's path except C itself must be listed in
FAMILIES (any family: a family stays one primary-base tree, and a secondary base heads or joins its own): C's family
reads only their exe tables, their own layouts are checked in their families. Virtual inheritance is not supported.
Exit status 1 on any mismatch.

--all's classes and families (all_families): each FNVD_VFTABLE constant in src names its class by its address, not
its spelling (`BSSimpleArray_char_1024_vftable_106CF0C`): the exe's RTTI locator before the table (its type
descriptor, demangled as clang spells the class: template instances, nested classes, msvc9 for std), else the IDA
database's `X::`vftable'` name, else the constant's own name less `_vftable` where src defines a class of that name
(the table's address then stands in for the missing RTTI). A constant for a base subobject's table (`X_vftable_Base`)
is skipped: its class's check covers it. The classes' primary bases with an exe vtable and a src definition join
them, and so do FAMILIES' classes. A family is every class with the same base-most primary base (RTTI, else src's
declared base). Each finding has a kind:
  - `exe overrides, src does not`: the exe's owner of the slot is deeper than src's final overrider's (natively the
    base's function runs where the exe runs the class's own);
  - `src overrides, exe does not`: src's final overrider is deeper than the exe's owner (natively src's override runs
    where the exe runs the base's);
  - `overrider differs`: an owner mismatch the chain cannot order (a name outside it);
  - `base body unshared`: src gives classes a body of a base the exe has no vtable for, which the exe does not show
    as one shared entry;
  - `slot count`, `stack bytes`, `this adjustment`, `purecall`, `registration` (the FNVD_REPLACES checks),
    `secondary layout` (a secondary table's base path against the exe's RTTI), `base chain` (src's vfptr at offset 0
    comes through a base with an exe vtable outside the exe's primary chain: the class's slots are not compared).
A table at offset 0 that src gets through bases the exe has no vtable for (BSSimpleArray's BSSimpleArrayRoot) is
compared as the primary one.
A finding's fold column says whether the exe entry is a known fold or the very function src's overrider is: src's
overrider registered at the entry, the entry's IDA name being src's overrider, or the entry only tail-jumping to it (no
fault: natively the same function runs); src's `Same function as ... registered there` comments naming it; its
registrations; the other exe tables (outside the class's chain) holding it; and, for an owner mismatch, whether its
bytes equal those of the body src's overrider stands for (a fold or equal trivial body). Such rows are kept, the column
marks them.
"""

import bisect
import dataclasses
import glob
import json
import multiprocessing
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fn  # noqa: E402

FAMILIES = [
    ["BGSLoadGameBuffer", "BGSLoadFormBuffer"],
    ["BaseProcess", "LowProcess", "MiddleLowProcess", "MiddleHighProcess", "HighProcess"],
    ["BSExtraData", "ExtraAction", "ExtraActivateRef", "ExtraActivateRefChildren", "ExtraAmmo", "ExtraAnim",
     "ExtraAudioMarker", "ExtraCannotWear", "ExtraCellAcousticSpace", "ExtraCellClimate",
     "ExtraCellImageSpace", "ExtraCellImpactSwap", "ExtraCellMusicType", "ExtraCellWaterType", "ExtraCollisionData",
     "ExtraCombatStyle", "ExtraContainerChanges", "ExtraCount", "ExtraCreatureAwakeSound", "ExtraDecalRefs",
     "ExtraDetachTime", "ExtraDismemberedLimbs", "ExtraDroppedItemList", "ExtraEditorID", "ExtraEmittanceSource",
     "ExtraEnableStateChildren", "ExtraEnableStateParent", "ExtraEncounterZone", "ExtraFactionChanges",
     "ExtraFollower", "ExtraGlobal", "ExtraHavok", "ExtraHealth", "ExtraHealthPerc", "ExtraIgnoredBySandbox",
     "ExtraLastFinishedSequence", "ExtraLevCreaModifier", "ExtraLeveledCreature", "ExtraLeveledItem", "ExtraLight",
     "ExtraLinkedRef", "ExtraLinkedRefChildren", "ExtraLitWaterRefs", "ExtraLock", "ExtraMapMarker",
     "ExtraMerchantContainer", "ExtraModelSwap", "ExtraMultiBoundData", "ExtraMultiBoundRef", "ExtraNorthRotation",
     "ExtraOcclusionPlane", "ExtraOcclusionPlaneRefData", "ExtraOwnership", "ExtraPackage",
     "ExtraPackageStartLocation", "ExtraPatrolRefData", "ExtraPersistentCell", "ExtraPlayerCrimeList",
     "ExtraPoison", "ExtraPortalRefData", "ExtraPrimitive", "ExtraRadiation", "ExtraRadioData", "ExtraRadius",
     "ExtraRagDollData", "ExtraRandomTeleportMarker", "ExtraRank", "ExtraReferencePointer", "ExtraReflectedRefs",
     "ExtraReflectorRefs", "ExtraRefractionProperty", "ExtraRoomRefData", "ExtraRunOncePacks",
     "ExtraSavedAnimation", "ExtraSavedHavokData", "ExtraSayToTopicInfo", "ExtraScript", "ExtraSound",
     "ExtraSpecialRenderFlags", "ExtraStartingPosition", "ExtraTeleport", "ExtraUsedMarkers", "ExtraWaterZoneMap",
     "ExtraWeaponModFlags", "ExtraWorn", "ExtraWornLeft", "ExtraXTarget", "NonActorMagicCaster",
     "NonActorMagicTarget"],
    ["BaseExtraList", "ExtraDataList"],
    ["BaseFormComponent", "BGSAmmoForm", "BGSBipedModelList", "BGSClipRoundsForm", "BGSDestructibleObjectForm",
     "BGSEquipType", "BGSIdleCollection", "BGSMessageIcon", "BGSPickupPutdownSounds", "BGSPreloadable",
     "BGSRepairItemList", "BGSTextureModel", "BGSTouchSpellForm", "TESAIForm", "TESActorBaseData", "TESAnimation",
     "TESAttackDamageForm", "TESAttributes", "TESBipedModelForm", "TESContainer", "TESDescription",
     "TESEnchantableForm", "TESFullName", "TESHealthForm", "TESIcon", "TESIconTree", "TESImageSpaceModifiableForm",
     "TESLeveledList", "TESModel", "TESModelAnim", "TESModelList", "TESModelPSA", "TESModelRDT",
     "TESModelTextureSwap", "TESModelTree", "TESProduceForm", "TESRaceForm", "TESReactionForm", "TESScriptableForm",
     "TESSoundFile", "TESSpellList", "TESTexture", "TESTexture1024", "TESValueForm", "TESWeightForm",
     "TESForm", "TESObject", "TESBoundObject", "TESBoundAnimObject", "TESBoundTreeObject",
     # V3 Wave B: Items
     "TESObjectWEAP", "TESObjectARMO", "TESObjectARMA", "TESAmmo", "TESObjectMISC", "TESKey",
     "BGSConstructibleObject", "TESLevItem", "BGSNote", "TESCaravanCard", "TESCaravanMoney", "TESCaravanDeck",
     "TESCasinoChips", "TESCasino", "TESChallenge", "TESObjectBOOK", "TESObjectCLOT", "TESObjectIMOD", "TESRecipe",
     "TESRecipeCategory",
     # V3 Wave B: Dialogue
     "TESPackage", "AlarmPackage", "BackUpPackage", "CombatController", "DialoguePackage", "FleePackage",
     "SearchPackage", "SpectatorPackage", "TrespassPackage", "BGSIdleMarker", "BGSListForm", "BGSMessage", "Script",
     "TESGlobal", "TESIdleForm", "TESQuest", "TESTopic", "TESTopicInfo",
     # V3 Wave B: Magic
     "MagicItem", "MagicItemForm", "MagicItemObject", "AlchemyItem", "IngredientItem", "EnchantmentItem",
     "SpellItem", "EffectSetting", "BGSDebris", "BGSExplosion", "BGSImpactData", "BGSImpactDataSet",
     "BGSProjectile", "TESAmmoEffect", "TESEffectShader",
     # V3 Wave B: Objects
     "BGSCameraPath", "BGSCameraShot", "BGSDefaultObjectManager", "BGSEncounterZone", "BGSMenuIcon",
     "BGSMusicType", "BGSTalkingActivator", "BGSTerminal", "MediaLocationController", "MediaSet", "TESFurniture",
     "TESLoadScreen", "TESLoadScreenType", "TESObjectACTI", "TESObjectANIO", "TESObjectCONT", "TESObjectDOOR",
     "TESObjectLIGH", "TESSkill", "TESSound",
     # V3 Wave B: World
     "BGSAcousticSpace", "BGSAddonNode", "BGSLightingTemplate", "BGSMovableStatic", "BGSPlaceableWater",
     "BGSStaticCollection", "BGSTextureSet", "NavMesh", "NavMeshInfoMap", "TESClimate", "TESFlora", "TESGrass",
     "TESImageSpace", "TESImageSpaceModifier", "TESLandTexture", "TESObjectCELL", "TESObjectLAND",
     "TESObjectSTAT", "TESObjectTREE", "TESRegion", "TESWaterForm", "TESWeather", "TESWorldSpace",
     # V3 Wave B: Actors
     "ActorValueInfo", "BGSBodyPartData", "BGSDehydrationStage", "BGSHeadPart", "BGSHungerStage", "BGSPerk",
     "BGSRadiationStage", "BGSRagdoll", "BGSSleepDeprevationStage", "BGSVoiceType", "TESActorBase", "TESClass",
     "TESCombatStyle", "TESCreature", "TESEyes", "TESFaction", "TESHair", "TESLevCharacter", "TESLevCreature",
     "TESLevSpell", "TESNPC", "TESRace", "TESReputation",
     # V3 Wave B: RefSpine
     "TESObjectREFR", "MobileObject", "Actor", "Character", "Creature", "PlayerCharacter",
     # Header wave: projectiles
     "MagicProjectile", "MagicBoltProjectile", "MagicFogProjectile", "MagicBallProjectile",
     "Projectile", "BeamProjectile", "GrenadeProjectile", "MissileProjectile"],
    ["TESImageSpaceModifiableCountForm<6>"],
    ["BGSOpenCloseForm"],
    ["EffectItemList"],
    ["TESPackageData", "TESAmbushPackageData", "TESDialoguePackageData", "TESEatPackageData", "TESEscortPackageData",
     "TESFollowPackageData", "TESPatrolPackageData", "TESUseItemPackageData", "TESUseWeaponPackageData"],
    ["TESChildCell"],
    ["MagicCaster"],
    ["MagicTarget"],
    ["ActorValueOwner"],
    ["CachedValuesOwner"],
    ["Menu", "InventoryMenu", "HUDMainMenu", "MapMenu", "StatsMenu", "StartMenu", "LoadingMenu",
     # Header wave: menus
     "CaravanMenu", "VATSMenu", "BlackJackMenu", "RouletteMenu", "SlotMachineMenu", "HackingMenu", "SurgeryMenu",
     "SPECIALBookMenu", "LoveTesterMenu", "LockPickMenu", "BarterMenu", "RecipeMenu", "RepairMenu", "LevelUpMenu",
     "ComputersMenu", "RepairServicesMenu", "ItemModMenu", "CharGenMenu", "TraitSelectMenu", "SleepWaitMenu",
     "TraitMenu", "BookMenu", "TutorialMenu", "QuantityMenu"],
    ["FORenderedMenu", "FORenderedMenuRaceSex"],
    ["Tile", "Tile3D", "TileText", "TileRect", "TileMenu", "TileImage"],
    ["IMemoryHeap", "ZeroOverheadHeap", "AbstractHeap", "MemoryHeap"],
    ["hkMemoryAllocator", "hkThreadMemory"],
    ["hkBaseObject", "hkReferencedObject", "hkpCharacterContext", "hkpCharacterStateManager", "hkpWorldObject",
     "hkpCharacterProxy", "hkpPhantom", "hkpEntity", "hkpMotion", "hkpConstraintOwner", "hkpSimulationIsland",
     "hkpWorld", "hkpWorldCinfo", "hkpKeyframedRigidMotion", "hkpMaxSizeMotion", "hkpAabbPhantom", "hkpShapePhantom",
     "hkpSimpleShapePhantom", "hkpCachingShapePhantom", "hkpShape", "hkpShapeCollection", "hkpListShape",
     "hkpBroadPhase", "hkp3AxisSweep", "hkpMoppDefaultSplitter",
     "hkpCollisionFilter", "hkpDisableEntityCollisionFilter", "bhkCollisionFilter", "hkpGroupFilter",
     "hkpCollisionFilterList", "hkpPairCollisionFilter", "hkpConstraintCollisionFilter", "hkpPhantomCallbackShape",
     "bhkWaterPhantomCallbackShape", "bhkAcousticSpacePhantomCallbackShape", "hkpRemoveTerminalsMoppModifier",
     "hkpRemoveTerminalsMoppModifier2", "hkVisualDebugger", "hkpPhysicsContext"],
    ["hkpConstraintListener"],
    ["hkpShapeContainer"],
    ["hkpMoppModifier"],
    ["hkProcess"],
    ["hkDebugDisplayHandler"],
    ["hkDebugDisplayProcess"],
    ["hkpFirstPersonGun", "hkpProjectileGun"],
    ["hkMemoryResourceHandle"],
    ["hkMemoryResourceContainer"],
    ["hkMemoryMeshShape"],
    ["IOManager"],
    ["hkpEntityListener", "bhkEntityListener", "bhkWaterListener", "TESWaterListener", "bhkTrapListener"],
    ["hkpContactListener", "bhkContactListener", "FOCollisionListener"],
    ["hkpCollidableCollidableFilter"],
    ["hkpShapeCollectionFilter"],
    ["hkpRayShapeCollectionFilter"],
    ["hkpRayCollidableFilter"],
    ["BGSZoneTargetListener", "TESTrapListener"],
    ["ZoneEntry"],
    ["hkpCharacterProxyListener", "bhkCharacterListener"],
    ["hkpPhantomListener"],
    ["INISettingCollection", "RegSettingCollection", "RendererSettingCollection", "BlendSettingCollection",
     "INIPrefSettingCollection"],
    ["hkpMalleableConstraintData", "hkpPrismaticConstraintData", "hkpLimitedHingeConstraintData",
     "hkpPointToPathConstraintData"],
    ["hkpCdPointCollector", "hkpAllCdPointCollector", "bhkCharacterPointCollector"],
    ["hkKdTreeCastCollector"],
    ["hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, unsigned short>::Collector"],
    ["hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, unsigned short>::Collector"],
    ["NiCullingProcess", "BSCullingProcess"],
    ["GridArray", "GridCellArray"],
    ["PathingLocation", "PathingCoverLocation"],
    # Header wave: AI, combat, pathing, magic, region, save-load, face-gen
    ["ImageSpaceEffect", "ImageSpaceEffectHDR", "ImageSpaceEffectWaterDisplacement", "ImageSpaceEffectMap"],
    ["CombatProcedure", "CombatProcedureAttackMelee", "CombatProcedureEngageTarget", "CombatProcedureMove",
     "CombatProcedureAttackRanged", "CombatProcedureSearch", "CombatProcedureBeInCover",
     "CombatProcedureAttackGrenade"],
    ["CombatAction", "CombatActionMove", "CombatActionAttackGrenadeFlushTarget", "CombatActionSwitchWeapon",
     "CombatActionIgnoreBlockedTarget", "CombatActionDrawWeapon", "CombatActionAttackRanged",
     "CombatActionAttackGrenade", "CombatActionActivateCombatItem"],
    ["ActorPackageData", "SandBoxActorPackageData"],
    ["ActorPathHandler", "VirtualActorPathHandler"],
    ["PathingSearchRayCast"],
    ["PathingTaskData"],
    ["ActiveEffect", "AssociatedItemEffect", "BoundItemEffect", "SummonCreatureEffect", "ValueModifierEffect",
     "AbsorbEffect", "TelekinesisEffect"],
    ["BSFaceGenKeyframe", "BSFaceGenKeyframeMultiple"],
    ["BGSPrimitive", "BGSPrimitiveBox", "BGSPrimitivePlane"],
    ["CIdvCamera"],
    ["CTreeEngine"],
    ["BGSSaveLoadStatsMap"],
    ["BGSReconstructFormsInFileMap"],
    ["BGSSceneInfo"],
    ["TESRegionData", "TESRegionDataSound"],
    ["TES"],
    ["Setting"],
    ["SkyObject", "Stars", "Sun", "Clouds", "Atmosphere"],
    ["Sky"],
    ["Precipitation"],
    ["ActorMover"],
    ["BSAudio", "BSWin32Audio"],
    ["BSGameSound"],
    ["BSAudioManager"],
    ["NiSearchPath"],
    ["NiRefObject", "NiObject", "NiObjectNET", "NiAVObject", "NiNode", "NiProperty", "NiTexture", "NiGeometry",
     "NiTriBasedGeom", "NiGeometryData", "NiTriBasedGeomData", "NiDynamicEffect", "NiTimeController",
     "NiInterpController", "NiSingleInterpController", "NiInterpolator", "NiExtraData", "NiPSysModifier",
     "NiCollisionObject", "BSOcclusionPlane", "PathingLOSMap", "BSFaceGenImage", "BSTreeModel"],
    ["NiAllocator"],
    ["NiBinaryStream", "NiFile"],
    ["NiStream", "BSStream"],
    ["NiTriShape", "BSSegmentedTriShape", "BSResizableTriShape", "NiScreenElements", "NiScreenGeometry",
     "TallGrassTriShape", "NiTriStrips", "TallGrassTriStrips", "NiLines", "NiTriShapeData", "NiTriShapeDynamicData",
     "NiTriStripsData", "NiTriStripsDynamicData", "NiScreenElementsData", "NiScreenGeometryData", "NiLinesData",
     "NiSkinInstance", "BSDismemberSkinInstance", "NiSkinData", "NiSkinPartition", "NiAdditionalGeometryData",
     "BSPackedAdditionalGeometryData", "NiLODData", "NiRangeLODData", "NiScreenLODData", "NiScreenPolygon",
     "NiScreenTexture", "NiScreenSpaceCamera", "NiGeometryData::RendererData", "NiGeometryBufferData",
     "NiSkinPartition::Partition"],
    ["bhkRefObject", "bhkSerializable", "bhkWorldObject", "bhkEntity", "bhkRigidBody", "bhkRigidBodyT",
     "bhkPhantom", "bhkShapePhantom", "bhkSimpleShapePhantom", "bhkCollisionBox", "bhkCharacterProxy", "bhkWorld",
     "bhkNiCollisionObject", "bhkCollisionObject", "bhkBlendCollisionObject", "WeaponObject",
     "bhkCharacterController", "bhkCharacterListenerArrow"],
    ["NiIntegerExtraData", "BSXFlags", "BSDecalPlacementVectorExtraData", "NiIntegersExtraData", "BSWArray",
     "BSBound", "BSFurnitureMarker", "BSTextureData", "NiBinaryExtraData", "NiBooleanExtraData", "NiColorExtraData",
     "NiFloatExtraData", "NiFloatsExtraData", "NiStringExtraData", "NiStringsExtraData", "NiSwitchStringExtraData",
     "NiTextKeyExtraData", "NiVectorExtraData", "NiVertWeightsExtraData", "BSTempEffect",
     "BSTempEffectGeometryDecal", "BSTempEffectSimpleDecal", "MagicHitEffect", "MagicShaderHitEffect",
     "BSTempEffectParticle", "MagicModelHitEffect"],
    ["NiFloatController", "NiRollController", "NiBoneLODController", "NiBSBoneLODController", "NiControllerManager",
     "NiKeyframeManager", "NiLookAtController", "NiPathController", "NiUVController", "NiFloatInterpController",
     "NiAlphaController", "NiFlipController", "NiLightDimmerController", "NiTextureTransformController",
     "BSFrustumFOVController", "BSMaterialEmittanceMultController", "BSRefractionFirePeriodController",
     "BSRefractionStrengthController", "NiBoolInterpController", "NiVisController", "NiPoint3InterpController",
     "NiLightColorController", "NiMaterialColorController", "NiExtraDataController", "NiColorExtraDataController",
     "NiFloatExtraDataController", "NiFloatsExtraDataController", "NiFloatsExtraDataPoint3Controller",
     "NiTransformController", "NiGeomMorpherController", "NiMultiTargetTransformController",
     "BSMultiTargetTreadTransfController"],
    ["BSBlastNode", "BSDamageStage", "BSDebrisNode", "BSFadeNode", "BSMultiBound", "BSMultiBoundAABB",
     "BSMultiBoundCapsule", "BSMultiBoundNode", "BSMultiBoundOBB", "BSMultiBoundRoom", "BSMultiBoundShape",
     "BSMultiBoundSphere", "BSNiNode", "BSNodeReferences", "BSOrderedNode", "BSRangeNode", "BSReference",
     "BSTreeNode", "BSValueNode", "NiAVObjectPalette", "NiAccumulator", "NiAlphaAccumulator", "NiAmbientLight",
     "NiBSPNode", "NiBackToFrontAccumulator", "NiBillboardNode", "NiBoundingVolume", "NiBoxBV", "NiCamera",
     "NiCapsuleBV", "NiCollisionData", "NiDirectionalLight", "NiHalfSpaceBV", "NiLODNode", "NiLight", "NiPointLight",
     "NiSortAdjustNode", "NiSphereBV", "NiSpotLight", "NiSwitchNode", "NiTextureEffect", "NiUnionBV",
     "BSPortalGraph", "ShadowSceneLight", "BSShaderAccumulator", "ShadowSceneNode", "LoadedAreaBound",
     "BSCubeMapCamera"],
    ["BSAnimNote", "BSAnimNotes", "BSBlendTreadTransfInterpolator", "BSGrabIKNote", "BSLookIKNote",
     "BSRotAccumTransfInterpolator", "BSTreadTransfInterpolator", "NiBSplineBasisData", "NiBSplineColorInterpolator",
     "NiBSplineCompColorInterpolator", "NiBSplineCompFloatInterpolator", "NiBSplineCompPoint3Interpolator",
     "NiBSplineCompTransformInterpolator", "NiBSplineData", "NiBSplineFloatInterpolator", "NiBSplineInterpolator",
     "NiBSplinePoint3Interpolator", "NiBSplineTransformInterpolator", "NiBlendAccumTransformInterpolator",
     "NiBlendBoolInterpolator", "NiBlendColorInterpolator", "NiBlendFloatInterpolator", "NiBlendInterpolator",
     "NiBlendPoint3Interpolator", "NiBlendQuaternionInterpolator", "NiBlendTransformInterpolator", "NiBoolData",
     "NiBoolInterpolator", "NiBoolTimelineInterpolator", "NiColorData", "NiColorInterpolator",
     "NiControllerSequence", "NiDefaultAVObjectPalette", "NiFloatData", "NiFloatInterpolator",
     "NiKeyBasedInterpolator", "NiLookAtInterpolator", "NiMorphData", "NiPathInterpolator", "NiPoint3Interpolator",
     "NiPosData", "NiQuaternionInterpolator", "NiRotData", "NiSequence", "NiSequenceStreamHelper", "NiStringPalette",
     "NiTransformData", "NiTransformInterpolator", "NiUVData", "BSAnimGroupSequence"],
    ["BSMasterParticleSystem", "BSPSysArrayEmitter", "BSPSysMultiTargetEmitterCtlr", "BSPSysSimpleColorModifier",
     "BSPSysStripUpdateModifier", "BSParentVelocityModifier", "BSStripPSysData", "BSStripParticleSystem",
     "BSWindModifier", "NiMeshPSysData", "NiMeshParticleSystem", "NiPSysAgeDeathModifier",
     "NiPSysAirFieldAirFrictionCtlr", "NiPSysAirFieldInheritVelocityCtlr", "NiPSysAirFieldModifier",
     "NiPSysAirFieldSpreadCtlr", "NiPSysBombModifier", "NiPSysBoundUpdateModifier", "NiPSysBoxEmitter",
     "NiPSysCollider", "NiPSysColliderManager", "NiPSysColorModifier", "NiPSysCylinderEmitter", "NiPSysData",
     "NiPSysDragFieldModifier", "NiPSysDragModifier", "NiPSysEmitter", "NiPSysEmitterCtlr", "NiPSysEmitterCtlrData",
     "NiPSysEmitterDeclinationCtlr", "NiPSysEmitterDeclinationVarCtlr", "NiPSysEmitterInitialRadiusCtlr",
     "NiPSysEmitterLifeSpanCtlr", "NiPSysEmitterPlanarAngleCtlr", "NiPSysEmitterPlanarAngleVarCtlr",
     "NiPSysEmitterSpeedCtlr", "NiPSysFieldAttenuationCtlr", "NiPSysFieldMagnitudeCtlr",
     "NiPSysFieldMaxDistanceCtlr", "NiPSysFieldModifier", "NiPSysGravityFieldModifier", "NiPSysGravityModifier",
     "NiPSysGravityStrengthCtlr", "NiPSysGrowFadeModifier", "NiPSysInitialRotAngleCtlr",
     "NiPSysInitialRotAngleVarCtlr", "NiPSysInitialRotSpeedCtlr", "NiPSysInitialRotSpeedVarCtlr",
     "NiPSysMeshEmitter", "NiPSysMeshUpdateModifier", "NiPSysModifierActiveCtlr", "NiPSysModifierBoolCtlr",
     "NiPSysModifierCtlr", "NiPSysModifierFloatCtlr", "NiPSysPlanarCollider", "NiPSysPositionModifier",
     "NiPSysRadialFieldModifier", "NiPSysResetOnLoopCtlr", "NiPSysRotationModifier", "NiPSysSpawnModifier",
     "NiPSysSphereEmitter", "NiPSysSphericalCollider", "NiPSysTurbulenceFieldModifier", "NiPSysUpdateCtlr",
     "NiPSysVolumeEmitter", "NiPSysVortexFieldModifier", "NiParticleMeshes", "NiParticleMeshesData",
     "NiParticleSystem", "NiParticles", "NiParticlesData", "BSParticleSystemManager"],
    ["BSDistantTreeShaderProperty", "BSRenderedTexture", "BSShaderBloodSplatterProperty", "BSShaderLightingProperty",
     "BSShaderNoLightingProperty", "BSShaderPPLightingProperty", "BSShaderProperty", "BSShaderTextureSet",
     "BSTextureSet", "DistantLODShaderProperty", "GeometryDecalShaderProperty", "HairShaderProperty",
     "Lighting30ShaderProperty", "Ni2DBuffer", "NiAlphaProperty", "NiCullingProperty", "NiD3DDefaultShader",
     "NiD3DPass", "NiD3DPixelShader", "NiD3DRenderState", "NiD3DShader", "NiD3DShaderConstantMap",
     "NiD3DShaderDeclaration", "NiD3DShaderInterface", "NiD3DShaderProgram", "NiD3DVertexShader", "NiDX9RenderState",
     "NiDX9Renderer", "NiDX9ShaderDeclaration", "NiDX9TextureData", "NiDX9TextureManager",
     "NiDX9VertexBufferManager", "NiDepthStencilBuffer", "NiDitherProperty", "NiFogProperty", "NiMaterialProperty",
     "NiPalette", "NiPersistentSrcTextureRendererData", "NiPixelData", "NiRenderTargetGroup", "NiRenderedCubeMap",
     "NiRenderedTexture", "NiRenderer", "NiShadeProperty", "NiShader", "NiShaderConstantMap",
     "NiShaderConstantMapEntry", "NiShaderDeclaration", "NiSourceCubeMap", "NiSourceTexture", "NiSpecularProperty",
     "NiStencilProperty", "NiTexturingProperty", "NiVertexColorProperty", "NiZBufferProperty",
     "ParticleShaderProperty", "SkyShaderProperty",
     "SpeedTreeBillboardShaderProperty", "SpeedTreeBranchShaderProperty", "SpeedTreeLeafShaderProperty",
     "SpeedTreeShaderLightingProperty", "SpeedTreeShaderPPLightingProperty", "TallGrassShaderProperty",
     "TileShaderProperty", "VolumetricFogShaderProperty", "WaterShaderProperty",
     "BSShaderPPLightingProperty::TextureEffectData", "BSShaderPPLightingProperty::TangentSpaceData",
     "NiTexturingProperty::Map", "NiTexturingProperty::BumpMap", "NiTexturingProperty::ShaderMap",
     "NiTexturingProperty::ParallaxMap", "NiTexture::RendererData",
     # Header wave: shaders
     "BSShader", "ParticleShader", "GeometryDecalShader", "BoltShader", "TileShader", "BSDistantTreeShader",
     "DebugShader", "BSShaderBloodSplatter", "ShadowLightShader", "HairShader", "DistantLODShader",
     "BoltShaderProperty", "SkyShader", "BSShaderNoLighting", "Lighting30Shader", "ParallaxShader", "SkinShader",
     "SpeedTreeBranchShader", "WaterShader", "TallGrassShader", "SpeedTreeLeafShader", "SpeedTreeFrondShader"],
    ["hkDataRefCounted", "ByteArrayImplementation", "RealArrayImplementation", "StructArrayImplementation",
     "VecArrayImplementation", "PointerArrayImplementation", "IntArrayImplementation", "CstringArrayImplementation",
     "ArrayOfTuplesImplementation", "ArrayOfTuplesImplementation::View", "hkDataObjectDict", "hkDataObjectNative",
     "hkDataClassNative", "hkDataClassDict", "hkDataArrayNative"],
    ["hkDataWorldDict", "hkDataWorldNative"],
    ["msvc9::exception", "msvc9::runtime_error", "IdvFileError"],
    ["msvc9::exception", "msvc9::logic_error", "msvc9::out_of_range"],
    ["msvc9::exception", "msvc9::bad_alloc"],
    # Header batch b2: Havok base, bhk creation infos
    ["hkTraceStream"],
    ["hkWindowsSystemClock"],
    ["hkBinaryTagfileReader"],
    ["hkConstraintCinfo", "hkRagdollLimitsConstraintCinfo", "hkBallSocketChainCinfo"],
    # Header wave v1: vtable layouts
    ["IDebugText", "DebugText"],
    ["hkProcessContext"],
    ["hkpWorldDeletionListener"],
    ["hkpActionListener"],
    ["IBSAnimNoteReceiver", "BSAnimNoteReceiver"],
    ["MobIterOperator", "HighActorCuller", "Reset3DMobIterator"],
    ["hkaRagdollRaycastInterface", "bhkRagdollPenetrationUtil"],
    ["Pathing::CheckLineOfSightFilter", "DoorInPathFilter"],
    ["TESRegionDataManager"],
    ["CombatGroupStrategy", "CombatGroupStrategyFlank", "CombatGroupStrategyFallBack", "CombatGroupStrategyAmbush",
     "CombatGroupStrategyAdvance"],
    ["CombatGoal"],
    ["TeleportDoorSearch"],
    ["BGSSaveGameBuffer", "BGSSaveFormBuffer"],
    ["Tile::Action", "Tile::FloatAction", "Tile::RefValueAction"],
    ["TrapTargetEntry"],
]
# per class, the probe compiles that failed, (class, message) (reported as that class's mismatches; the other classes
# are still checked)
PROBE_ERRORS: list[tuple[str, str]] = []
# per class, the secondary tables whose record layout the tool cannot place (the class is otherwise checked)
RECORD_ERRORS: list[tuple[str, str]] = []
# per class, the name a probe spells it by where it is not the class's own (a template instance src models as a plain
# class of the template's name)
PROBE_SPELLING: dict[str, str] = {}


def spelled(cls: str) -> str:
    return PROBE_SPELLING.get(cls, cls)


# the classes per probe compile (the layout probe of a larger family is split; a failing one is halved)
CHUNK = 200


def work() -> str:
    """The probe folder of this process (--all checks families in several)."""
    return os.path.join(ROOT, "build", "vtable-check", str(os.getpid()))


LLVM = "/home/cnf/.local/opt/LLVM-23.1.3-Linux-X64/bin"
# MSVC's purecall stub in the exe (`UnimplementedVirtFunc`): the entry of every pure virtual slot
PURECALL = 0xEC60FA
# overload sets whose entries pop the same bytes, their slot order checked by hand against the exe bodies
# (2026-10-26): the int overloads convert with `fild`, the float ones take the value as is; the reference overload
# compares the pointer, the form id one calls GetFormID first; PriorityTest(uint32_t) compares a value, the
# BSGameSound* one the other sound's priority; NiStream::Load/Save(const char*) read the name, the (char*, int)
# overloads a buffer and its size; NiInterpolator::Update's six base bodies are one stub, the order follows
# NiFloatInterpolator overriding only slot 39 (the float overload); BSMultiBound*::CheckBound (2026-10-27): the
# BSMultiBound& bodies load the argument's shape pointer, the NiBound& ones read the center floats
ORDER_CHECKED = {
    ("BaseProcess", "TempModActorValue"), ("BaseProcess", "DamageModActorValue"),
    ("BaseProcess", "ClearObjectFromAcquireList"), ("LowProcess", "DamageModActorValue"),
    ("LowProcess", "ClearObjectFromAcquireList"), ("MiddleLowProcess", "TempModActorValue"),
    ("HighProcess", "TempModActorValue"), ("HighProcess", "DamageModActorValue"), ("BSGameSound", "PriorityTest"),
    ("NiStream", "Load"), ("NiStream", "Save"), ("NiInterpolator", "Update"),
    ("BSMultiBoundAABB", "CheckBound"), ("BSMultiBoundCapsule", "CheckBound"), ("BSMultiBoundOBB", "CheckBound"),
    ("BSMultiBoundSphere", "CheckBound"), ("BSMultiBoundNode", "CheckBound"), ("BSMultiBoundRoom", "CheckBound"),
    # DebugText (2026-11-05): the full PrintX/PrintXC overloads (0xA0E4B0, 0xA0E6D0) pass their location and
    # justification on, the short ones (0xA0E470, 0xA0E690) push 0 and 1
    ("DebugText", "PrintX"), ("DebugText", "PrintXC"),
}
# the member may be an operator (`FNVD_REPLACES_VIRTUAL(addr, NiBoxBV, operator==)`), spelled as in the vtable dump
REPLACES = re.compile(r"FNVD_REPLACES(_VIRTUAL(?:_OVERLOAD)?|_OVERLOAD|_MEMBER)?\(\s*(0x[0-9A-Fa-f]+)\s*,\s*([\w:~]+)"
                      r"(?:\s*,\s*(operator[^\s,()]+|~?\w+))?")
# an override the exe enters with a base subobject's this: FNVD_REPLACES_VIRTUAL_VIA(address, Class, Base, member)
REPLACES_VIA = re.compile(r"FNVD_REPLACES_VIRTUAL_VIA(?:_OVERLOAD)?\(\s*(0x[0-9A-Fa-f]+)\s*,\s*([\w:]+)\s*,\s*"
                          r"([\w:]+)\s*,\s*(operator[^\s,()]+|~?\w+)")
TABLE = re.compile(r"^VFTable for '[^']+'(?: in '[^']+')* \((\d+) entr(?:y|ies)\)\.$")
OWNER = re.compile(r"^VFTable for (?:'[^']+' in )*'([^']+)' \(")
ENTRY = re.compile(r"^\s+(\d+) \| (.*)$")
# a class's template arguments as clang spells them (`<TESObjectREFR *, 1024>`), nested up to six levels (NiTMapBase's
# `<DFALL<NiTMapItem<unsigned int, BSSimpleList<SavedFormData *> *>>, ...>` takes four)
TEMPLATE_ARGS = "[^<>]*"
for _ in range(6):
    TEMPLATE_ARGS = r"(?:[^<>]|<" + TEMPLATE_ARGS + r">)*"
TEMPLATE_ARGS = "<" + TEMPLATE_ARGS + ">"


def without_arguments(name: str) -> str:
    """A class name without the template arguments of any of its parts (`A<B>::C<6>` -> `A::C`)."""
    return re.sub(TEMPLATE_ARGS, "", name)


# a conversion operator (`operator bool() const`) has no return type before it; clang appends a template's arguments
# to its members (`[N = 6]`)
SIGNATURE = re.compile(r"^(?P<ret>.*?)\s*\b(?P<owner>\w+(?:" + TEMPLATE_ARGS + r")?(?:::\w+(?:" + TEMPLATE_ARGS
                       + r")?)*)::(?P<name>~?\w+|operator\s*[^(]+?)\((?P<params>.*)\)(?: const)?(?: \[[^\]]*\])*$")
# the line after an entry reached through an adjustor thunk (a slot of a secondary vtable)
ADJUSTMENT = re.compile(r"^\s+\[this adjustment: (.*)\]$")
# a base class in a -fdump-record-layouts tree: offset, indentation (2 per level), name
RECORD_BASE = re.compile(r"^\s*(\d+) \| ((?:  )*)(?:class|struct|union) (.+) \(((?:primary |virtual )?base)\)$")
RECORD_TOP = re.compile(r"^\s*0 \| (?:class|struct|union) (.+)$")
# MSVC's codes of the builtin types, spelled as clang prints them
MSVC_BUILTINS = {"C": "signed char", "D": "char", "E": "unsigned char", "F": "short", "G": "unsigned short",
                 "H": "int", "I": "unsigned int", "J": "long", "K": "unsigned long", "M": "float", "N": "double",
                 "O": "long double", "X": "void", "_J": "long long", "_K": "unsigned long long", "_N": "bool",
                 "_W": "wchar_t"}


def demangle_class(mangled: str) -> str | None:
    """An RTTI type descriptor's name (`.?AVB@A@@`, `.?AV?$C@PAVD@@$0EAA@@`) as clang spells the class (`A::B`,
    `C<D *, 1024>`), or None for an encoding this does not cover. Template argument lists get their own name and
    type back-reference tables, as in MSVC's mangling."""
    text, at = mangled, 0
    names: list[str] = []
    types: list[str] = []

    def take(count: int = 1) -> str:
        nonlocal at
        if at + count > len(text):
            raise ValueError(mangled)
        at += count
        return text[at - count:at]

    def identifier() -> str:
        nonlocal at
        end = text.find("@", at)
        if end < 0:
            raise ValueError(mangled)
        name, at = text[at:end], end + 1
        return name

    def qualified() -> str:
        nonlocal at, names, types
        fragments = []
        while (char := take()) != "@":
            if char.isdigit():
                fragments.append(names[int(char)])
                continue
            if char == "?" and text.startswith("$", at):
                take()
                template = identifier()
                outer = names, types
                names, types = [template], []
                arguments = []
                while not text.startswith("@", at):
                    start = at
                    arguments.append(argument())
                    if at - start > 1 and not text.startswith("$", start):
                        types.append(arguments[-1])
                take()
                names, types = outer
                fragment = f"{template}<{', '.join(arguments)}>"
            else:
                at -= 1
                fragment = identifier()
            names.append(fragment)
            fragments.append(fragment)
        return "::".join(reversed(fragments))

    def number() -> str:
        negative = text.startswith("?", at) and take() == "?"
        char = take()
        if char.isdigit():
            value = int(char) + 1
        else:
            value = 0
            while char != "@":
                if not "A" <= char <= "P":
                    raise ValueError(mangled)
                value = value * 16 + ord(char) - ord("A")
                char = take()
        return str(-value if negative else value)

    def argument() -> str:
        if text.startswith("$0", at):
            take(2)
            return number()
        return type_()

    def type_() -> str:
        char = take()
        if char == "_" and "_" + text[at:at + 1] in MSVC_BUILTINS:
            return MSVC_BUILTINS["_" + take()]
        if char in MSVC_BUILTINS:
            return MSVC_BUILTINS[char]
        if char.isdigit():
            return types[int(char)]
        if char in "PQRSA":
            cv = {"A": "", "B": "const ", "C": "volatile ", "D": "const volatile "}[take()]
            pointer = {"P": " *", "Q": " *const", "R": " *volatile", "S": " *const volatile", "A": " &"}[char]
            return cv + type_() + pointer
        if char in "VUT":
            return qualified()
        if char == "W":
            take()
            return qualified()
        raise ValueError(mangled)

    match = re.fullmatch(r"\.\?A[UV](.*)", mangled)
    if not match:
        return None
    at = 4
    try:
        name = qualified()
    except (ValueError, IndexError, KeyError):
        return None
    return name if at == len(text) else None




class Exe:
    def __init__(self):
        import pefile
        self.pe = pefile.PE(fn.EXE, fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.sections = [(self.base + s.VirtualAddress, s.get_data(), s.Name.rstrip(b"\0")) for s in self.pe.sections]
        # fn.py pickles its call graph as a script, so the cache refers to __main__.Function
        sys.modules["__main__"].Function = fn.Function
        self.graph = fn.load_graph()
        self.rets: dict[int, set[int]] = {}
        self.locators: dict[int, int] = {}
        # type descriptor -> {subobject offset: complete object locator} of the secondary vtables
        self.secondary: dict[int, dict[int, int]] = {}
        # Bethesda's shader library is built without RTTI: its vtables are found only by the IDA database's names
        # (`??_7X@@6B@`, ``const X::`vftable'``), each bounded by the next named address or the next table code
        # references
        self.named_vtables: dict[str, int] = {}
        for address, text in fn.vtable_names().items():
            table = re.fullmatch(r"const ([\w:]+)::`vftable'", text)
            if table:
                self.named_vtables[table.group(1)] = address
        self.publics = sorted(fn.ida_names())

    def section(self, name: bytes):
        return next((start, data) for start, data, n in self.sections if n == name)

    def dword(self, address: int) -> int:
        for start, data, _ in self.sections:
            if start <= address < start + len(data) - 3:
                return struct.unpack_from("<I", data, address - start)[0]
        raise KeyError(hex(address))

    def canonical(self, name: str) -> str:
        """A template instance spelled as clang prints it (`A<B, C>`), whatever its spacing (`A<B,C>`)."""
        if "<" not in name or name.startswith(".?A"):
            return name
        self.descriptor_templates()
        compact = re.sub(r"\s+", "", name)
        return next((spelled for spelled in self.templates if spelled and re.sub(r"\s+", "", spelled) == compact),
                    name)

    def descriptor_templates(self) -> None:
        if not hasattr(self, "templates"):
            data = self.section(b".data")[1]
            found = re.finditer(rb"\.\?A[UV][\x21-\x7e]*\?\$[\x21-\x7e]*?@@(?=\0)", data)
            self.templates = {demangle_class(m.group().decode()): m.group().decode() for m in found}

    def descriptor(self, name: str) -> int:
        """Address of the class's RTTI type descriptor (`name` plain, nested `A::B`, a template instance as clang
        spells it, `C<D *, 1024>`, or already mangled). Remembered per name, a missing one too."""
        if not hasattr(self, "descriptors"):
            self.descriptors: dict[str, int | None] = {}
        if name not in self.descriptors:
            try:
                self.descriptors[name] = self.find_descriptor(name)
            except SystemExit:
                self.descriptors[name] = None
        if self.descriptors[name] is None:
            sys.exit(f"vtable-check: no RTTI type descriptor for {name}")
        return self.descriptors[name]

    def find_descriptor(self, name: str) -> int:
        data_start, data = self.section(b".data")
        if "<" in name and not name.startswith(".?A"):
            self.descriptor_templates()
            name = self.templates.get(name, name)
        # src models VC9's standard library types in namespace msvc9 (src/SDK/MSVC/); the exe's RTTI names them std
        if name.startswith("msvc9::"):
            name = "std::" + name[len("msvc9::"):]
        scoped = "@".join(reversed(name.split("::")))
        for mangled in ([name] if name.startswith(".?A") else [f".?AV{scoped}@@", f".?AU{scoped}@@"]):
            at = data.find(mangled.encode() + b"\0")
            if at >= 0:
                return data_start + at - 8
        sys.exit(f"vtable-check: no RTTI type descriptor for {name}")

    def locator(self, name: str) -> int:
        """The class's complete object locator at offset 0 (its primary vtable's)."""
        if not self.locators:
            rdata_start, rdata = self.section(b".rdata")
            for offset in range(0, len(rdata) - 20, 4):
                signature, this_offset, _, type_descriptor, hierarchy = struct.unpack_from("<IIIII", rdata, offset)
                if signature != 0:
                    continue
                if this_offset == 0:
                    self.locators.setdefault(type_descriptor, rdata_start + offset)
                    continue
                # a locator at another offset is taken only if its hierarchy's first base is the class itself
                try:
                    if self.dword(self.dword(self.dword(hierarchy + 12))) != type_descriptor:
                        continue
                except KeyError:
                    continue
                self.secondary.setdefault(type_descriptor, {}).setdefault(this_offset, rdata_start + offset)
        found = self.locators.get(self.descriptor(name))
        if found is None:
            sys.exit(f"vtable-check: no complete object locator for {name}")
        return found

    def secondary_offsets(self, name: str) -> list[int]:
        """Offsets of the subobjects whose vtables are the class's secondary ones (their locators carry the offset);
        empty for a class without RTTI."""
        if not self.has_locator(name):
            return []
        return sorted(self.secondary.get(self.descriptor(name), {}))

    def base_layout(self, name: str) -> list[tuple[str, int]]:
        """The class's RTTI base class array: (base, offset in the class), in pre-order, the class itself first;
        empty without RTTI. A virtual base (pdisp not -1) has no fixed offset: offset None."""
        hierarchy = self.hierarchy(name)
        if hierarchy is None:
            return []
        layout = []
        for index in range(self.dword(hierarchy + 8)):
            base = self.dword(self.dword(hierarchy + 12) + 4 * index)
            fixed = self.dword(base + 12) == 0xFFFFFFFF
            layout.append((self.type_name(self.dword(base)), self.dword(base + 8) if fixed else None))
        return layout

    def base_sizes(self, name: str) -> list[int]:
        """Per entry of the class's base class array (see base_layout), the number of entries right after it that
        form its subtree (the base class descriptor's numContainedBases, +4). An entry's direct bases are the
        subtrees that follow it; empty without RTTI."""
        hierarchy = self.hierarchy(name)
        if hierarchy is None:
            return []
        array = self.dword(hierarchy + 12)
        return [self.dword(self.dword(array + 4 * index) + 4) for index in range(self.dword(hierarchy + 8))]

    def base_chain(self, name: str, top: int, base: tuple[str, int]) -> list[int] | None:
        """Indexes into the class's base class array from below entry `top` down to the first entry of its subtree
        that is `base` (name, offset), each a direct base of the one before; None if the subtree has no such entry."""
        layout, sizes = self.base_layout(name), self.base_sizes(name)
        end = top + sizes[top] + 1
        target = next((index for index in range(top + 1, end) if layout[index] == base), None)
        if target is None:
            return None
        chain, at = [], top
        while at != target:
            at += 1
            # skip the direct bases whose subtrees end before the target
            while at + sizes[at] < target:
                at += sizes[at] + 1
            chain.append(at)
        return chain

    def subobject(self, name: str, offset: int) -> str:
        """The outermost base the class's RTTI places at the offset (its base class array is in pre-order)."""
        return next((base for base, at in self.base_layout(name)[1:] if at == offset), "?")

    def type_name(self, descriptor: int) -> str:
        mangled = bytearray()
        while (byte := self.dword(descriptor + 8 + len(mangled)) & 0xFF) != 0:
            mangled.append(byte)
        simple = re.fullmatch(r"\.\?A[UV]((?:\w+@)+)@", mangled.decode())
        # nested and namespaced classes as A::B, template instances as clang spells them (A<B *, 6>); an encoding
        # the demangler does not cover keeps its mangled name
        if simple:
            name = "::".join(reversed(simple.group(1).rstrip("@").split("@")))
        else:
            name = demangle_class(mangled.decode()) or mangled.decode()
        # the exe's std:: classes are src's msvc9:: ones (see descriptor)
        return "msvc9::" + name[len("std::"):] if name.startswith("std::") else name

    def has_locator(self, name: str) -> bool:
        try:
            self.locator(name)
        except SystemExit:
            return False
        return True

    def has_vtable(self, name: str) -> bool:
        return name in self.named_vtables or self.has_locator(name)

    def heirs(self, name: str) -> set[str]:
        """The classes with a primary complete object locator whose RTTI base class array holds the class below
        themselves (every class deriving from it, directly or not); empty if it has no type descriptor."""
        if not hasattr(self, "heirs_of"):
            self.has_locator(name)  # builds self.locators
            self.heirs_of: dict[int, set[int]] = {}
            # the .rdata scan also takes dwords that only look like a locator (signature 0, offset 0): their
            # hierarchy pointers lead outside the image and are skipped
            for descriptor, found in self.locators.items():
                try:
                    hierarchy = self.dword(found + 16)
                    array = self.dword(hierarchy + 12)
                    bases = [self.dword(self.dword(array + 4 * i)) for i in range(1, self.dword(hierarchy + 8))]
                except KeyError:
                    continue
                for base in bases:
                    self.heirs_of.setdefault(base, set()).add(descriptor)
        try:
            descriptor = self.descriptor(name)
        except SystemExit:
            return set()
        return {self.type_name(heir) for heir in self.heirs_of.get(descriptor, set())}

    def hierarchy(self, name: str) -> int | None:
        """The class's RTTI class hierarchy descriptor: from its locator, or (a class whose vtable has no locator)
        from a base class descriptor naming it; None without RTTI."""
        try:
            descriptor = self.descriptor(name)
        except SystemExit:
            return None
        if self.has_locator(name):
            return self.dword(self.locator(name) + 16)
        rdata_start, rdata = self.section(b".rdata")
        needle = struct.pack("<I", descriptor)
        where = rdata.find(needle)
        while where >= 0:
            if where % 4 == 0 and where + 28 <= len(rdata):
                hierarchy = struct.unpack_from("<I", rdata, where + 24)[0]
                try:
                    if self.dword(self.dword(self.dword(hierarchy + 12))) == descriptor:
                        return hierarchy
                except KeyError:
                    pass
            where = rdata.find(needle, where + 1)
        return None

    def first_base(self, name: str) -> str | None:
        """The class's primary base: the first entry after itself in its RTTI base class array at offset 0, vtable or
        not, or None. That is the first declared base, except where MSVC moved a polymorphic base ahead of a
        non-polymorphic first one (Archive: BSArchive declared first, BSFile at 0). A class without RTTI (the shader
        library) takes the base its src header declares: unverified."""
        hierarchy = self.hierarchy(name)
        if hierarchy is None:
            return source_parents().get(name)
        return next((base for base, offset in self.base_layout(name)[1:] if offset == 0), None)

    def bases(self, name: str) -> list[str]:
        """The class's chain of first bases, nearest first, including those without a vtable of their own."""
        chain, at = [], self.first_base(name)
        # a src base list read as naming the class itself (an explicit specialization deriving from its primary
        # template) must not loop
        while at is not None and at != name and at not in chain:
            chain.append(at)
            at = self.first_base(at)
        return chain

    def parent(self, name: str) -> str | None:
        """The nearest first base with a vtable in the exe, or None. A base without one (`NiMemObject`, or a
        `novtable` class such as `NiGPUProgram`) is skipped: the class shares the vtable of the next base up, or
        introduces its own vfptr."""
        return next((base for base in self.bases(name) if self.has_vtable(base)), None)

    def vtable_address(self, name: str, offset: int = 0) -> int:
        """Where the class's vtable for the subobject at the offset starts (0: its primary vtable): RTTI type
        descriptor -> complete object locator of that offset -> the dword after the one naming it, or the IDA
        database's `X::`vftable'` for a class without a locator (primary only)."""
        if not self.has_locator(name):
            if name not in self.named_vtables or offset:
                sys.exit(f"vtable-check: no vtable for {name}{f'+{offset:#x}' if offset else ''} (no RTTI locator, "
                         f"no IDA name)")
            return self.named_vtables[name]
        locator = self.secondary.get(self.descriptor(name), {}).get(offset) if offset else self.locator(name)
        if locator is None:
            sys.exit(f"vtable-check: no vtable for {name}+{offset:#x}")
        rdata_start, rdata = self.section(b".rdata")
        needle = struct.pack("<I", locator)
        where = rdata.find(needle)
        while where >= 0:
            if where % 4 == 0:
                return rdata_start + where + 4
            where = rdata.find(needle, where + 1)
        sys.exit(f"vtable-check: no vtable for {name}")

    def vtable_class(self, address: int) -> tuple[str, int] | None:
        """(class, subobject offset) of the exe vtable at the address: its RTTI locator's type descriptor, else the
        IDA database's ``X::`vftable'`` name (offset 0; a ``{for `Base'}`` table: -1, a secondary one); None if
        neither names it."""
        self.has_locator("")  # builds self.locators and self.secondary
        if not hasattr(self, "tables_of"):
            self.tables_of: dict[int, tuple[int, int]] = {}
            for descriptor, found in self.locators.items():
                self.tables_of[found] = (descriptor, 0)
            for descriptor, at in self.secondary.items():
                for offset, found in at.items():
                    self.tables_of[found] = (descriptor, offset)
        try:
            located = self.tables_of.get(self.dword(address - 4))
        except KeyError:
            located = None
        if located:
            return self.type_name(located[0]), located[1]
        if not hasattr(self, "vtable_texts"):
            self.vtable_texts = fn.vtable_names()
        text = self.vtable_texts.get(address)
        table = re.fullmatch(r"const ([\w:]+)::`vftable'(\{for .*\})?", text or "")
        if table:
            return table.group(1), -1 if table.group(2) else 0
        return None

    def body(self, address: int) -> bytes:
        """The bytes of the function starting at the address (up to the next function's start)."""
        function = self.graph.containing(address)
        if function is None or function.address != address:
            return b""
        return self.pe.get_data(address - self.base, function.size)

    def code_references(self) -> dict[int, list[int]]:
        """The absolute addresses .text holds (each base relocation in .text: its target -> where it sits)."""
        if not hasattr(self, "referenced"):
            text_start, text = self.section(b".text")
            directory = self.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5]
            blocks = self.pe.get_data(directory.VirtualAddress, directory.Size)
            self.referenced, at = {}, 0
            while at + 8 <= len(blocks):
                page, size = struct.unpack_from("<II", blocks, at)
                if size < 8:
                    break
                for (entry,) in struct.iter_unpack("<H", blocks[at + 8:at + size]):
                    # IMAGE_REL_BASED_HIGHLOW: a 32-bit absolute address at page + low 12 bits
                    where = self.base + page + (entry & 0xFFF)
                    if entry >> 12 == 3 and text_start <= where < text_start + len(text) - 3:
                        target = struct.unpack_from("<I", text, where - text_start)[0]
                        self.referenced.setdefault(target, []).append(where)
                at += size
        return self.referenced

    def stored_as_vfptr(self, address: int) -> bool:
        """Whether some .text instruction stores the address into memory as an immediate (`mov dword ptr [esi],
        offset X::vftable`: a constructor or destructor setting a vfptr), which makes it a table's start. A call
        through a slot (`call dword ptr [X::vftable+N]`, MSVC's devirtualized calls) does not count."""
        import capstone
        disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        for where in self.code_references().get(address, []):
            # the immediate is the instruction's last four bytes; its opcode and operand bytes come 2 to 10 before
            for back in range(2, 11):
                code = self.pe.get_data(where - back - self.base, back + 4)
                instruction = next(disassembler.disasm_lite(code, where - back, 1), None)
                if instruction and instruction[1] == back + 4 and instruction[2] == "mov" and \
                        re.fullmatch(r"dword ptr \[[^\]]*\], 0x[0-9a-f]+", instruction[3]):
                    return True
        return False

    def vtable(self, name: str, offset: int = 0) -> list[int]:
        """The class's vtable for the subobject at the offset (see vtable_address). The table ends at the first dword
        outside .text (the next table's locator), at the next IDA name (data that happens to look like a .text
        address, e.g. the string after BSWArray's table) or at the next slot address code stores as a vfptr (a table
        without RTTI or label: the one after ImageSpaceEffect's, the cached distant LOD geometry map's at 0x10ADBAC)."""
        text_start, text = self.section(b".text")
        table = self.vtable_address(name, offset)
        at = bisect.bisect_right(self.publics, table)
        end = self.publics[at] if at < len(self.publics) else table + 4096
        referenced = self.code_references()
        entries = []
        while table + 4 * len(entries) < end:
            slot = table + 4 * len(entries)
            if entries and slot in referenced and self.stored_as_vfptr(slot):
                break
            value = self.dword(slot)
            if not text_start <= value < text_start + len(text):
                break
            entries.append(value)
        return entries

    def thunk(self, address: int) -> tuple[int, int]:
        """(this adjustment, target) of an adjustor thunk, the compiler's `sub ecx, N` (thiscall) or `sub dword ptr
        [esp + 4], N` (stdcall, COM: this is the first stack argument), then `jmp target`; (0, address) for any other
        entry. `add ecx, N; jmp target` is no thunk: it is a body that tail-calls a member of a member (N > 0) or of
        the full object (N < 0, an override in a secondary table: 0xCD2460 jumps to removeWorld)."""
        import capstone
        disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        code = self.pe.get_data(address - self.base, 16)
        first, second = (list(disassembler.disasm_lite(code, address, 2)) + [None, None])[:2]
        if first and second and first[2] == "sub" and second[2] == "jmp" and second[3].startswith("0x"):
            for this in ("ecx, ", "dword ptr [esp + 4], "):
                if first[3].startswith(this):
                    amount = int(first[3][len(this):], 0) & 0xFFFFFFFF
                    amount -= (amount >> 31) << 32
                    return -amount, int(second[3], 0)
        return 0, address

    def ret(self, address: int) -> set[int]:
        """Stack bytes the function pops (`ret N`; 0 for a plain `ret`). Follows the control flow from the entry,
        so the jump tables MSVC places inside a function are never decoded as code."""
        if address not in self.rets:
            import capstone
            function = self.graph.containing(address)
            end = function.address + function.size
            code = self.pe.get_data(address - self.base, end - address)
            disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
            found, seen, pending = set(), set(), [address]
            while pending:
                at = pending.pop()
                while address <= at < end and at not in seen:
                    seen.add(at)
                    instruction = next(disassembler.disasm_lite(code[at - address:], at, 1), None)
                    if instruction is None:
                        break
                    _, size, mnemonic, operands = instruction
                    if mnemonic == "ret":
                        found.add(int(operands, 0) if operands else 0)
                        break
                    if mnemonic.startswith("j") and operands.startswith("0x"):
                        pending.append(int(operands, 0))
                    if mnemonic == "jmp":
                        break
                    at += size
            self.rets[address] = found
        return self.rets[address]


def hybrid_command() -> tuple[str, list[str]]:
    if not HYBRID_COMMAND:
        with open(os.path.join(BUILD, "compile_commands.json")) as handle:
            entry = next(e for e in json.load(handle) if "/src/" in e["file"])
        words = [w for w in shlex.split(entry["command"]) if w != entry["file"] and w != "--"
                 and not w.startswith(("/Fo", "/Fd")) and w not in ("-c", "/c")]
        HYBRID_COMMAND.extend([entry["directory"], words])
    return HYBRID_COMMAND[0], HYBRID_COMMAND[1]


HYBRID_COMMAND: list = []


DEFINITIONS: dict[str, list[str]] = {}
# the classes only the second scan finds (indented by spaces, or after `template <...>` on the same line); header()
# falls back to them
SPACED: dict[str, list[str]] = {}
PARENTS: dict[str, str] = {}


def scan_headers() -> None:
    if DEFINITIONS:
        return
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        with open(path, encoding="utf-8") as source:
            # a nested class (indented by its depth, or defined out of line as A::B) is named A::B
            enclosing: list[str] = []
            # the base list may span lines (`class Actor : public MobileObject,` / `public MagicCaster, ...`)
            pattern = r"^(\t*)(?:class|struct)\s+(?:alignas\(\w+\)\s+)?([\w:]+)\b([^;{]*?)\s*\{"
            for match in re.finditer(pattern, source.read(), re.M):
                depth = len(match.group(1))
                enclosing[depth:] = [match.group(2)]
                name = "::".join(enclosing)
                # an explicit specialization (`class A<B> : public C {`) is defined with its template: one header
                # may define several, and the base list follows the template arguments
                paths = DEFINITIONS.setdefault(name, [])
                if path not in paths:
                    paths.append(path)
                rest = re.sub(r"^\s*" + TEMPLATE_ARGS, "", match.group(3))
                base = re.match(r"\s*(?:final\s*)?:\s*(?:public\s+)?([\w:]+)", rest)
                if base:
                    PARENTS[name] = base.group(1)
        # the same with indentation by spaces (4 per level) and template heads on the class's line
        # (`template <class T, uint32_t N = 1024> class BSSimpleArray ... {`), for the classes the tab scan misses
        with open(path, encoding="utf-8") as source:
            enclosing = []
            pattern = r"^([\t ]*)(template\s*<[^{;]*?>\s*)?(?:class|struct)\s+(?:alignas\(\w+\)\s+)?([\w:]+)\b" \
                      r"([^;{]*?)\s*\{"
            for match in re.finditer(pattern, source.read(), re.M):
                depth = len(match.group(1).expandtabs(4)) // 4
                enclosing[depth:] = [match.group(3)]
                name = "::".join(enclosing)
                if " " not in match.group(1) and not match.group(2):
                    continue
                paths = SPACED.setdefault(name, [])
                if path not in paths:
                    paths.append(path)
                rest = re.sub(r"^\s*" + TEMPLATE_ARGS, "", match.group(4))
                base = re.match(r"\s*(?:final\s*)?:\s*(?:public\s+)?([\w:]+)", rest)
                if base:
                    PARENTS.setdefault(name, base.group(1))


def source_parents() -> dict[str, str]:
    """Each src class's first declared base (for classes the exe has no RTTI for)."""
    scan_headers()
    return PARENTS


def header(name: str) -> str:
    """The src header that defines the class (a template instance: the template's)."""
    scan_headers()
    # src/SDK/MSVC/ defines msvc9's classes inside `namespace msvc9 {`, which the scan does not track
    plain = without_arguments(name.removeprefix("msvc9::"))
    found = DEFINITIONS.get(plain) or SPACED.get(plain, [])
    if len(found) != 1:
        sys.exit(f"vtable-check: {len(found)} headers define {name}")
    return found[0]


def headers(name: str) -> list[str]:
    """The headers a probe of the class needs: its own, and those of the src classes its template arguments name
    (`BSList<TriggerEntry::TriggerEvent *>` needs TriggerEntry.h)."""
    found = [header(name)]
    arguments = name[len(re.match(r"[\w:]*", name).group(0)):]
    for word in re.findall(r"[A-Za-z_]\w*(?:::\w+)*", arguments):
        paths = DEFINITIONS.get(without_arguments(word)) or SPACED.get(without_arguments(word), [])
        if len(paths) == 1 and paths[0] not in found:
            found.append(paths[0])
    return found


def source_includes(name: str) -> list[str]:
    """The quoted includes of the .cpp beside the class's header (none without one)."""
    path = os.path.splitext(header(name))[0] + ".cpp"
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8", errors="replace") as handle:
        return re.findall(r'^\s*#include\s+"([^"]+)"', handle.read(), re.M)


def compile_source(text: str, name: str, extra: list[str]) -> subprocess.CompletedProcess:
    os.makedirs(work(), exist_ok=True)
    path = os.path.join(work(), name)
    with open(path, "w") as source:
        source.write(text)
    directory, command = hybrid_command()
    env = dict(os.environ, PATH=LLVM + ":" + os.environ.get("PATH", ""))
    return subprocess.run(command + extra + ["--", path], cwd=directory, capture_output=True, text=True, env=env)


def split_params(text: str) -> list[str]:
    params, depth, current = [], 0, ""
    for char in text:
        if char == "," and depth == 0:
            params.append(current.strip())
            current = ""
            continue
        depth += char in "<(["
        depth -= char in ">)]"
        current += char
    if current.strip() and current.strip() != "void":
        params.append(current.strip())
    return params


def layouts(classes: list[str], chains: dict[str, set[str]]) -> tuple[dict[str, list[dict]],
                                                                       dict[str, dict[int, dict]]]:
    """Each class's vtables as clang lays them out: per slot the final overrider's owner, name, return, parameters
    and this adjustment. `chains[c]` is c with its primary bases: a table whose path names only those is c's primary
    vtable, any other a secondary one. Returns (primary tables, secondary tables): the secondary ones per class by the
    offset of their subobject, each with its path (c first, down to the base whose vfptr it is) and the offsets of
    the path's classes in c (from clang's record layout)."""
    # per class, the extra headers its probe includes: those of the class's own .cpp, where a member's type that its
    # header only forward declares is complete (TES's NiPointer<QueuedFile> members need QueuedFile for their
    # destructors, which constructing a TES instantiates)
    extra: dict[str, list[str]] = {}

    def source(names: list[str]) -> str:
        includes = "#include <new>\n#include <type_traits>\n"
        wanted = (h for c in names for h in headers(c) + extra.get(c, []))
        includes += "".join(f'#include "{h}"\n' for h in dict.fromkeys(wanted))
        # a destructor call lays out the vtable of a class with a virtual destructor; constructing one lays out any
        # other. Neither instantiates more than it needs: constructing every class would also instantiate its members'
        # destructors, which src never does for a class with a virtual destructor (those have no constructor in src),
        # and destroying a class without one would instantiate its members' destructors where src never destroys it
        # (TES's NiPointer members). A destructor the probe cannot call (a protected one) is left alone: the derived
        # class below lays out such a class's vtable instead.
        probes = ("template <class T> void fnvdVtableProbe(T* object) {\n"
                  "\tif constexpr (std::has_virtual_destructor_v<T> && std::is_destructible_v<T>)\n"
                  "\t\tobject->~T();\n"
                  "\tif constexpr (std::is_default_constructible_v<T> && !std::has_virtual_destructor_v<T>)\n"
                  "\t\t::new (static_cast<void*>(object)) T;\n}\n")
        probes += "".join(f"template void fnvdVtableProbe<{spelled(c)}>({spelled(c)}*);\n" for c in names)
        # an abstract class without a virtual destructor, or a class whose virtual destructor the probe cannot call:
        # the definition of a derived class's constructor lays out its vtable
        probes += ("template <class T, bool = (std::is_abstract_v<T> && !std::has_virtual_destructor_v<T>) ||\n"
                   "\t(std::has_virtual_destructor_v<T> && !std::is_destructible_v<T>)>\n"
                   "struct fnvdVtableDerived {};\n"
                   "template <class T> struct fnvdVtableDerived<T, true> : T {\n\tfnvdVtableDerived();\n};\n"
                   "template <class T> fnvdVtableDerived<T, true>::fnvdVtableDerived() {}\n")
        probes += "".join(f"template struct fnvdVtableDerived<{spelled(c)}>;\n" for c in names)
        return includes + probes

    flags = ["/c", "/Fo" + os.path.join(work(), "layout.obj"), "-Xclang", "-fdump-vtable-layouts"]
    failing: dict[str, str] = {}

    def probe(group: list[str]) -> list[tuple[list[str], subprocess.CompletedProcess]]:
        """The group's compiles that succeeded, (classes, result). A group that does not compile is halved, so one
        class that does not compile does not blind the others; a class failing alone is retried with its .cpp's
        includes, then reported in `failing`."""
        result = compile_source(source(group), "layout.cpp", flags)
        if not result.returncode:
            return [(group, result)]
        if len(group) > 1:
            return probe(group[:len(group) // 2]) + probe(group[len(group) // 2:])
        cls = group[0]
        if includes := source_includes(cls):
            extra[cls] = includes
            result = compile_source(source(group), "layout.cpp", flags)
            if not result.returncode:
                return [(group, result)]
            extra.pop(cls, None)
        # src models some template instances as a plain class of the template's name (BSTMessageQueue for the
        # exe's BSTMessageQueue<ActorPathingMessage>)
        if without_arguments(cls) != cls:
            PROBE_SPELLING[cls] = without_arguments(cls)
            plain = compile_source(source(group), "layout.cpp", flags)
            if not plain.returncode:
                return [(group, plain)]
            PROBE_SPELLING.pop(cls, None)
        failing[cls] = next((line for line in result.stderr.splitlines() if "error:" in line), result.stderr.strip())
        return []

    # clang names a template instance's tables without its arguments: two instances of one template go to different
    # compiles
    groups: list[list[str]] = []
    for cls in classes:
        stem = without_arguments(cls)
        group = next((g for g in groups if len(g) < CHUNK and all(without_arguments(c) != stem for c in g)), None)
        if group is None:
            groups.append(group := [])
        group.append(cls)
    compiled = [found for group in groups for found in probe(group)]
    for cls, error in failing.items():
        PROBE_ERRORS.append((cls, f"{cls}: the layout probe does not compile: {error}"))
    tables: dict[str, list[dict]] = {}
    secondary: dict[str, dict[int, dict]] = {}
    for group, result in compiled:
        group_tables, group_secondary = parse_layouts(result.stdout, group, chains, source(group))
        tables.update(group_tables)
        secondary.update(group_secondary)
    return tables, secondary


def type_words(text: str) -> set[str]:
    """The identifiers and numbers of a type's spelling (integer suffixes dropped, C's `_Bool` as bool)."""
    return set(re.findall(r"\w+", re.sub(r"\b(\d+)[uUlL]+\b", r"\1", plain_type(text).replace("_Bool", "bool"))))


def instance_of(owner: str, entries: list[dict]) -> bool:
    """Whether a table's entries fit the template instance (always for a class that is none): every template binding
    clang prints for the members of the owner's template names only words of the instance's arguments (clang drops
    default arguments: `[T = BSSimpleArray<NiPoint3>]` in BSSimpleArray<BSSimpleArray<NiPoint3, 1024>, 1024>)."""
    if "<" not in owner:
        return True
    stem, mine = without_arguments(owner), type_words(owner)
    return all(type_words(value) <= mine for entry in entries if without_arguments(entry["owner"]) == stem
               for value in entry["bindings"].values())


def parse_layouts(dump: str, classes: list[str], chains: dict[str, set[str]],
                  probe_source: str) -> tuple[dict[str, list[dict]], dict[str, dict[int, dict]]]:
    """layouts() of the classes one probe compile laid out: its -fdump-vtable-layouts output, and the probe's source
    for the record layouts of the classes with secondary tables."""
    tables, current = {}, None
    primaries: dict[str, list[list[dict]]] = {}
    paths: dict[str, list[tuple[list[str], list[dict]]]] = {}
    for line in dump.splitlines():
        if line.startswith("VFTable for"):
            current = None
            if TABLE.match(line):
                # clang names a template instance here without its arguments (`'TESImageSpaceModifiableCountForm'`):
                # the listed class of that name, and the record layout for the bases (base_offsets)
                owner = OWNER.match(line).group(1)
                owner = next((c for c in classes if c == owner), None) or \
                    next((c for c in classes if without_arguments(c) == owner), owner)
                path = re.findall(r"'([^']+)'", line)
                path[-1] = owner
                if owner in classes and {without_arguments(p) for p in path} <= \
                        {without_arguments(c) for c in chains[owner]}:
                    current = []
                    primaries.setdefault(owner, []).append(current)
                elif owner in classes:
                    current = []
                    paths.setdefault(owner, []).append((path[::-1], current))
            continue
        if current is None:
            continue
        adjustment = ADJUSTMENT.match(line)
        if adjustment and current:
            plain = re.fullmatch(r"(-?\d+) non-virtual", adjustment.group(1))
            # a vtordisp or virtual-base adjustment (virtual inheritance) is not supported
            current[-1]["adjust"] = int(plain.group(1)) if plain else adjustment.group(1)
            continue
        entry = ENTRY.match(line)
        if not entry:
            current = None if not line.startswith(" ") else current
            continue
        text = entry.group(2)
        if text.endswith(" RTTI"):
            continue
        signature = SIGNATURE.match(text)
        if not signature:
            sys.exit(f"vtable-check: cannot parse vtable entry '{text}'")
        # the template parameters clang names in the member's signature (`BSListItem<T> *... [T = X *]`)
        bindings = {}
        for group in re.findall(r" \[([^\]]*=[^\]]*)\]", text[signature.end("params"):]):
            for binding in split_params(group):
                name, _, value = binding.partition(" = ")
                if value:
                    bindings[name.strip()] = value.strip()
        current.append({"owner": signature["owner"], "name": signature["name"], "ret": signature["ret"],
                        "params": split_params(signature["params"]), "text": text, "adjust": 0,
                        "bindings": bindings})
    # a table clang names by a listed template's name may be another instance's (a member's, a base's): its entries'
    # template bindings (`[T = X]`) must be the instance's arguments; of the matching ones the last is taken
    for owner, found in primaries.items():
        kept = [entries for entries in found if instance_of(owner, entries)]
        if kept:
            tables[owner] = kept[-1]
    for owner in paths:
        paths[owner] = [(path, entries) for path, entries in paths[owner] if instance_of(owner, entries)]

    def respell(entries: list[dict], names: set[str]) -> None:
        """clang spells a template instance's members without its default arguments (`BSSimpleArray<X>::f` for
        BSSimpleArray<X, 1024>): an owner the names (the chain) hold under one spelling of the same template takes it."""
        by_stem: dict[str, list[str]] = {}
        for name in names:
            if "<" in name:
                by_stem.setdefault(without_arguments(name), []).append(name)
        for entry in entries:
            spelled = by_stem.get(without_arguments(entry["owner"]), [])
            # a plain owner too: src models some template instances as plain classes of the template's name
            # (BSTCommonMessageQueue for BSTCommonMessageQueue<ActorPathingMessage>)
            if entry["owner"] not in names and len(spelled) == 1:
                entry["owner"] = spelled[0]
    for owner, entries in tables.items():
        respell(entries, chains[owner])
    for owner, found in paths.items():
        for path, entries in found:
            respell(entries, set(path) | chains[owner])
    secondary: dict[str, dict[int, dict]] = {}
    if any(paths.values()):
        records = record_layouts(probe_source)
        # a template instance's name as the record layout spells it may differ by spaces, integer suffixes and
        # dropped default arguments (`BSSimpleArray<NiPointer<QueuedFile> >` for `<NiPointer<QueuedFile>, 1024>`)
        loose = {record_key(name): bases for name, bases in records.items()}

        def record_of(owner: str) -> list[tuple[int, int, str, str]]:
            if found := records.get(plain_type(owner)) or loose.get(record_key(owner)):
                return found
            # the instance of the same template whose words are the most of the owner's
            stem, mine = without_arguments(owner), type_words(owner)
            near = sorted(((len(type_words(name)), bases) for name, bases in records.items()
                           if without_arguments(name) == stem and type_words(name) <= mine), key=lambda n: -n[0])
            return near[0][1] if near and (len(near) == 1 or near[0][0] > near[1][0]) else []
        for owner, found in paths.items():
            for path, entries in found:
                offsets = base_offsets(record_of(owner), path)
                if offsets is None:
                    RECORD_ERRORS.append((owner, f"{owner}: no record layout places {' in '.join(path[::-1])} "
                                                 f"(its secondary table there is unchecked)"))
                    continue
                secondary.setdefault(owner, {})[offsets[-1]] = {"path": path, "offsets": offsets,
                                                                "entries": entries}
    return tables, secondary


def plain_type(name: str) -> str:
    """A type as the vtable dump spells it (the record layout dump adds `class `/`struct ` in template arguments)."""
    return re.sub(r"\b(?:class|struct|union|enum) ", "", name).strip()


def record_key(name: str) -> str:
    """A type's name without spaces, `class `/`struct `, integer literal suffixes (`1024U`) and C's `_Bool`."""
    return re.sub(r"\b(\d+)[uUlL]+\b", r"\1", re.sub(r"\s+", "", plain_type(name))).replace("_Bool", "bool")


def record_layouts(source: str) -> dict[str, list[tuple[int, int, str, str]]]:
    """Per class laid out by the source, its bases as clang places them: (offset, depth, name, kind) in pre-order,
    kind `base`, `primary base` or `virtual base`."""
    result = compile_source(source, "records.cpp",
                            ["/c", "/Fo" + os.path.join(work(), "records.obj"), "-Xclang", "-fdump-record-layouts"])
    if result.returncode:
        sys.exit("vtable-check: the headers do not compile:\n" + result.stderr)
    records: dict[str, list[tuple[int, int, str, str]]] = {}
    current = None
    lines = result.stdout.splitlines()
    for at, line in enumerate(lines):
        if line == "*** Dumping AST Record Layout" and at + 1 < len(lines):
            top = RECORD_TOP.match(lines[at + 1])
            current = records.setdefault(plain_type(top.group(1)), []) if top else None
            continue
        if current is None:
            continue
        if "[sizeof=" in line:
            current = None
            continue
        base = RECORD_BASE.match(line)
        if base:
            current.append((int(base.group(1)), len(base.group(2)) // 2, plain_type(base.group(3)), base.group(4)))
    return records


def base_offsets(bases: list[tuple[int, int, str, str]], path: list[str]) -> list[int] | None:
    """Offsets in path[0] of each class of the path (each a direct base of the one before), or None if the record
    layout has no such chain or it goes through a virtual base. A template instance the vtable dump names without
    its arguments takes the record layout's full name in the path."""
    offsets, depth, at = [0], 0, 0
    for index, name in enumerate(path[1:], 1):
        while at < len(bases) and not (bases[at][1] == depth + 1 and (bases[at][2] == name or (
                "<" not in name and without_arguments(bases[at][2]) == name))):
            if bases[at][1] <= depth:
                return None
            at += 1
        if at == len(bases) or bases[at][3] == "virtual base":
            return None
        path[index] = bases[at][2]
        offsets.append(bases[at][0])
        depth += 1
        at += 1
    return offsets


HEADER_TEXTS: dict[str, str] = {}


def declared_stdcall(entry: dict) -> bool:
    """Whether the final overrider's header declares it stdcall (the vtable dump does not show the convention)."""
    try:
        path = header(entry["owner"])
    except SystemExit:
        return False
    if path not in HEADER_TEXTS:
        with open(path, encoding="utf-8", errors="replace") as source:
            HEADER_TEXTS[path] = source.read()
    convention = r"\b(?:FNVD_STDCALL|__stdcall|STDMETHODCALLTYPE|WINAPI)\s+"
    return re.search(convention + re.escape(entry["name"]) + r"\s*\(", HEADER_TEXTS[path]) is not None


def stack_errors(slots: list[tuple[str, str, int, dict, set[int]]]) -> list[tuple[str, str | None, int | None, str]]:
    """Slots whose parameter stack bytes (plus a hidden result pointer), computed by the compiler, differ from the
    exe's `ret N` (one static_assert per slot). Each slot: (class, table label, slot, clang entry, exe pops). Returns
    (class, label, slot, message) per differing slot, and (class, None, None, message) per class whose probe does not
    compile."""
    # the asserts per class; each names its slot by an index into `named` (a label with template arguments or nested
    # names is no valid part of a message regex)
    blocks: dict[str, list[str]] = {}
    named: list[tuple[str, str, int, str]] = []
    for cls, label, slot, entry, want in slots:
        # no `ret` reachable (a tail jump through another vtable): nothing to compare
        if not want:
            continue
        index = len(named)
        if len(want) != 1:
            named.append((cls, label, slot, f"the exe entry pops {sorted(want)}"))
            blocks.setdefault(cls, []).append(f'static_assert(false, "fnvdSlot{index}");\n')
            continue
        # a variadic entry is cdecl: the caller pops, so the exe's `ret` pops nothing
        if "..." in entry["params"]:
            if want != {0}:
                named.append((cls, label, slot, f"variadic, but the exe entry pops {sorted(want)}"))
                blocks.setdefault(cls, []).append(f'static_assert(false, "fnvdSlot{index}");\n')
            continue
        named.append((cls, label, slot, "stack bytes differ from the exe"))
        # clang prints a function pointer's convention as a trailing `__attribute__((cdecl))`, which a template
        # argument cannot spell: any pointer takes the same 4 bytes
        params = ", ".join("void *" if "__attribute__" in param else param for param in entry["params"])
        ret = "void *" if "__attribute__" in (entry["ret"] or "") else entry["ret"] or "void"
        # each assert in its own struct: the template parameters clang prints (`T`) aliased to this entry's
        # arguments (entries of different base templates may bind the same name differently); a non-type parameter
        # (`[N = 6]`) becomes a constant
        aliases = "".join(f"static constexpr auto {name} = {value}; "
                          if re.fullmatch(r"-?\d+[uUlL]*|true|false|'.'", value) else f"using {name} = {value}; "
                          for name, value in entry.get("bindings", {}).items())
        # a stdcall member (COM: FNVD_STDCALL, STDMETHODCALLTYPE) takes this on the stack, and its `ret N` pops it
        this = " + 4" if declared_stdcall(entry) else ""
        blocks.setdefault(cls, []).append(
            f"struct fnvdSlot{index} {{ {aliases}static_assert(::fnvd::detail::stackBytesOf<{params}>(){this} + "
            f"(::fnvd::detail::returnsThroughPointer<{ret}, true>() ? 4 : 0) == {next(iter(want))}, "
            f'"fnvdSlot{index}"); }};\n')

    def block(at: int, cls: str, derived: bool) -> str:
        """cls's asserts. Derived from cls, the names clang prints unqualified (a base's nested types) resolve."""
        asserts = "".join("\t" + line for line in blocks[cls])
        if derived:
            return f"struct fnvdStackBytes{at} : {spelled(cls)} {{\n{asserts}}};\n"
        return f"namespace fnvdStackBytes{at} {{\n{asserts}}}\n"

    def run(classes: list[str], derived: bool) -> tuple[list[tuple[str, str, int, str]], list[str]]:
        """(slot errors, other compile errors) of the classes' asserts compiled together."""
        text = "".join(f'#include "{h}"\n' for h in dict.fromkeys(h for c in classes for h in headers(c)))
        text += "".join(block(at, c, derived) for at, c in enumerate(classes))
        result = compile_source(text, "bytes.cpp", ["/Zs", "-ferror-limit=0"])
        found, others, last = [], [], None
        for line in (result.stdout + result.stderr).splitlines():
            failed = re.search(r"static assertion failed.*\bfnvdSlot(\d+)\b", line)
            if failed:
                last = named[int(failed.group(1))]
                found.append((last[0], last[1], last[2], f"{last[1]} slot {last[2]}: {last[3]}"))
                continue
            value = re.search(r"expression evaluates to '(\d+) == (\d+)'", line)
            if value and last:
                found[-1] = (last[0], last[1], last[2], f"{last[1]} slot {last[2]}: takes {value.group(1)} stack "
                                                       f"bytes, the exe pops {value.group(2)}")
                last = None
            elif "error:" in line and "static assertion" not in line:
                others.append(line)
        return found, others

    def probe(classes: list[str]) -> list[tuple[str, str | None, int | None, str]]:
        """One class whose probe does not compile must not blind the rest: a group that does not compile is halved;
        a class alone is derived from (a final class cannot be) or else checked at namespace scope."""
        found, others = run(classes, True)
        if not others:
            return found
        if len(classes) > 1:
            return probe(classes[:len(classes) // 2]) + probe(classes[len(classes) // 2:])
        cls = classes[0]
        found, others = run([cls], False)
        failed = [(cls, None, None, f"{cls}: the stack-bytes probe does not compile: {others[0]}")] if others else []
        return failed + found

    classes = list(blocks)
    if not classes:
        return []
    return probe(classes)


# the kinds of findings (see the docstring); SKIPPED: the class is not checked, PARTIAL: its stack bytes are not
EXE_ONLY = "exe overrides, src does not"
SRC_ONLY = "src overrides, exe does not"
OWNER_DIFFERS = "overrider differs"
UNSHARED = "base body unshared"
SLOT_COUNT = "slot count"
STACK_BYTES = "stack bytes"
ADJUSTMENT_KIND = "this adjustment"
PURECALL_KIND = "purecall"
REGISTRATION = "registration"
SECONDARY_LAYOUT = "secondary layout"
BASE_CHAIN = "base chain"
SKIPPED = "skipped"
PARTIAL = "partly skipped"


@dataclasses.dataclass
class Finding:
    kind: str
    cls: str
    label: str
    slot: int | None
    text: str
    entry: int | None = None
    src: str = ""
    fold: str = ""


# address -> the `Same function as ... registered there` comments naming it (file:line), the src names registering it
# (Class::member), the exe tables holding it (their classes); filled by fold_data()
FOLD_COMMENTS: dict[int, list[str]] = {}
REGISTERED_NAMES: dict[int, list[str]] = {}
TABLE_HOLDERS: dict[int, set[str]] = {}
# the IDA database's names, address -> name
IDA_NAMES: dict[int, str] = {}
# (class, member) -> the addresses src registers it at
REGISTERED_AT: dict[tuple[str, str], list[int]] = {}


def registrations() -> list[tuple[bool, int, str, str | None, str, str | None]]:
    """src's registrations: (virtual, address, class, member, file:line, VIA base or None)."""
    registered = []
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True):
        with open(path, encoding="utf-8") as source:
            text = source.read()
        for match in REPLACES.finditer(text):
            kind, address, first, member = match.groups()
            line = text.count("\n", 0, match.start()) + 1
            where = f"{os.path.relpath(path, ROOT)}:{line}"
            if kind and kind.startswith("_VIRTUAL"):
                registered.append((True, int(address, 16), first, member, where, None))
            elif kind == "_MEMBER":
                registered.append((False, int(address, 16), first, member, where, None))
            elif "::" in first:
                cls, _, name = first.rpartition("::")
                registered.append((False, int(address, 16), cls, name, where, None))
        for match in REPLACES_VIA.finditer(text):
            address, cls, base, member = match.groups()
            line = text.count("\n", 0, match.start()) + 1
            registered.append((True, int(address, 16), cls, member, f"{os.path.relpath(path, ROOT)}:{line}", base))
    return registered


def fold_data(registered: list[tuple[bool, int, str, str | None, str, str | None]]) -> None:
    if TABLE_HOLDERS:
        return
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.*"), recursive=True):
        if not path.endswith((".h", ".inl", ".cpp")):
            continue
        with open(path, encoding="utf-8", errors="replace") as source:
            text = source.read()
        for match in fn.FOLD.finditer(text):
            line = text.count("\n", 0, match.start()) + 1
            FOLD_COMMENTS.setdefault(int(match.group(2), 16), []).append(f"{os.path.relpath(path, ROOT)}:{line}")
    for _, address, cls, member, _, _ in registered:
        REGISTERED_NAMES.setdefault(address, []).append(f"{cls}::{member}" if member else cls)
        REGISTERED_AT.setdefault((cls, member), []).append(address)
    IDA_NAMES.update(fn.ida_names())
    for name, _, slots in fn.vtables().values():
        for address in slots:
            TABLE_HOLDERS.setdefault(address, set()).add(re.sub(r"\s*\{for .*\}$", "", name))


def fold_note(exe: "Exe", order: list[str], address: int, other: int | None = None,
              overrider: tuple[str, str] | None = None) -> str:
    """What marks the exe entry at the address as a fold (see the docstring); `other`: the address whose body src's
    overrider stands for, compared byte for byte; `overrider`: src's final overrider (class, member), whose
    registration at the address makes the entry that very function."""
    notes = []
    named = f"{overrider[0]}::{overrider[1]}" if overrider else None
    # the entry is only `jmp target` (a wrapper tail-calling the base's body)
    head = exe.pe.get_data(address - exe.base, 5)
    target = address + 5 + int.from_bytes(head[1:5], "little", signed=True) if head[:1] == b"\xe9" else None
    if overrider and address in REGISTERED_AT.get(overrider, []):
        notes.append("src's overrider is registered at this entry")
    elif named and IDA_NAMES.get(address) == named:
        notes.append("the entry's IDA name is src's overrider")
    elif target is not None and (target == other or target in REGISTERED_AT.get(overrider, []) or
                                 IDA_NAMES.get(target) == named):
        notes.append(f"the entry tail-jumps to src's overrider ({target:#x})")
    if FOLD_COMMENTS.get(address):
        notes.append("src fold comment " + ", ".join(FOLD_COMMENTS[address]))
    if REGISTERED_NAMES.get(address):
        notes.append("registered as " + ", ".join(sorted(set(REGISTERED_NAMES[address]))))
    plain = {without_arguments(c) for c in order}
    others = sorted(c for c in TABLE_HOLDERS.get(address, ()) if without_arguments(c) not in plain)
    if others:
        notes.append(f"in {len(others)} other exe tables ({', '.join(others[:3])}{', ...' if len(others) > 3 else ''})")
    if other is not None and other != address:
        mine = exe.body(address)
        if mine and mine == exe.body(other):
            notes.append(f"equal body to {other:#x}")
    return "; ".join(notes)


def overrider_kind(order: list[str], cls: str, exe_owner: str, src_owner: str) -> str:
    """An owner mismatch's kind: which of the two owners is deeper in the chain (order: the class first)."""
    def depth(name: str) -> int | None:
        plain = without_arguments(name)
        return next((at for at, c in enumerate(order) if c == name or without_arguments(c) == plain), None)
    exe_at, src_at = depth(exe_owner), depth(src_owner)
    if exe_at is not None and src_at is not None and exe_at != src_at:
        return EXE_ONLY if exe_at < src_at else SRC_ONLY
    if exe_owner == cls:
        return EXE_ONLY
    if src_owner == cls:
        return SRC_ONLY
    return OWNER_DIFFERS


def check_family(exe: "Exe", family: list[str], listed: set[str], listed_as: str,
                 registered: list[tuple[bool, int, str, str | None, str, str | None]],
                 verbose: bool) -> tuple[list[str], list[Finding], list[str]]:
    """One family's checks: (lines to print, findings, warnings)."""
    out: list[str] = []
    errors: list[Finding] = []
    warnings: list[str] = []

    def report(kind: str, cls: str, label: str, slot: int | None, text: str, entry: int | None = None,
               src: str = "", fold: str = "") -> None:
        errors.append(Finding(kind, cls, label, slot, text, entry, src, fold))

    # a class the exe or src cannot provide (no exe vtable, no header) is reported alone, the rest checked
    kept = []
    for cls in family:
        try:
            header(cls)
            exe.vtable(cls)
            exe.bases(cls)
        except SystemExit as failure:
            report(SKIPPED, cls, cls, None, f"{cls}: {str(failure.code).removeprefix('vtable-check: ')}")
            continue
        kept.append(cls)
    family = kept
    if not family:
        return out, errors, warnings
    chains: dict[str, set[str]] = {cls: {cls, *exe.bases(cls)} for cls in family}
    tables, secondaries = layouts(family, chains)
    for cls, error in PROBE_ERRORS:
        report(SKIPPED, cls, cls, None, error)
    PROBE_ERRORS.clear()
    for cls, error in RECORD_ERRORS:
        report(PARTIAL, cls, cls, None, error)
    RECORD_ERRORS.clear()
    exe_tables = {c: exe.vtable(c) for c in family}
    exe_secondary: dict[tuple[str, int], list[int]] = {}

    def exe_table(cls: str, offset: int = 0) -> list[int]:
        if offset:
            if (cls, offset) not in exe_secondary:
                exe_secondary[cls, offset] = exe.vtable(cls, offset)
            return exe_secondary[cls, offset]
        if cls not in exe_tables:
            exe_tables[cls] = exe.vtable(cls)
        return exe_tables[cls]
    checked = []
    # cls's own entries in its secondary tables: (cls, label, slot, name, entry, exe this adjustment, the classes
    # of the table's path at its subobject's offset)
    own_secondary = []
    owners: dict[str, list[str]] = {}
    secondary_owners: dict[tuple[str, int], list[str]] = {}

    def owners_of(cls: str) -> list[str]:
        """Per exe slot, the deepest class of cls's primary-base chain whose entry differs from its parent's."""
        if cls not in owners:
            parent = exe.parent(cls)
            # a base outside the list (checking a subtree) still gives its exe owners
            theirs, inherited = exe_table(cls), owners_of(parent) if parent else []
            base = exe_table(parent) if parent else []
            owners[cls] = [cls if slot >= len(base) or theirs[slot] != base[slot] else inherited[slot]
                           for slot in range(len(theirs))]
        return owners[cls]

    def owners_at(cls: str, offset: int, path: list[str], offsets: list[int]) -> list[str]:
        """Per exe slot of cls's vtable for the subobject at the offset, the deepest class of the path down to
        that subobject whose entry differs from the next class's (that class's table for the same subobject;
        at offset 0 within it, its primary table)."""
        if not offset:
            return owners_of(cls)
        if (cls, offset) not in secondary_owners:
            parent, at = path[1], offset - offsets[1]
            inherited = owners_at(parent, at, path[1:], [o - offsets[1] for o in offsets[1:]])
            theirs, base = exe_table(cls, offset), exe_table(parent, at)
            secondary_owners[cls, offset] = [
                cls if slot >= len(base) or theirs[slot] != base[slot] else inherited[slot]
                for slot in range(len(theirs))]
        return secondary_owners[cls, offset]

    def entry_of(order: list[str], offsets: list[int], offset: int, owner: str, slot: int) -> int | None:
        """The exe entry, for the same subobject, of the class of the chain src names as a slot's final overrider:
        the body src's overrider stands for (None if the exe has no such table)."""
        plain = without_arguments(owner)
        at = next((i for i, c in enumerate(order) if c == owner or without_arguments(c) == plain), None)
        if at is None:
            return None
        try:
            table = exe_table(order[at], offset - offsets[at])
        except (SystemExit, KeyError):
            return None
        return table[slot] if slot < len(table) else None

    # (base, slot) -> [(class label, exe entry)] of the classes inheriting that base's own body for the slot
    shared: dict[tuple[str, int], list[tuple[str, int]]] = {}

    def compare(cls: str, label: str, ours: list[dict], theirs: list[int], owned: list[str], secondary: bool,
                order: list[str], offsets: list[int], offset: int, at_offset: frozenset = frozenset()):
        """Slot count, final overrider owners, purecall and (secondary tables) this adjustments; own entries go
        to `checked` as (class, label, slot, entry, exe body), the body behind an adjustor thunk. `order`: the
        classes of the table's path, cls first, at `offsets` in cls; the table's subobject at `offset`."""
        if len(ours) != len(theirs):
            report(SLOT_COUNT, cls, label, None, f"{label}: {len(ours)} slots, the exe has {len(theirs)}")
        for slot in range(min(len(ours), len(theirs))):
            owner = owned[slot]
            entry = ours[slot]
            # a derived class always gets its own (implicit) destructor; the exe may fold it into its base's
            folded = entry["name"].startswith("~") and entry["owner"] == cls
            pure = "[pure]" in entry["text"]
            # a pure redeclaration over a slot that is already _purecall leaves the exe's table unchanged, so the
            # exe cannot show who redeclared it (MagicItemObject::IsAutoCalc over MagicItem's at +0x30)
            hidden = pure and theirs[slot] == PURECALL
            # src names the template instances of the setting collections without their argument
            src_owner = owner.replace("<Setting>", "")
            # a slot the exe table gives to cls whose src owner is a base the exe has no vtable for (an abstract
            # class such as hkpConstraintData): the base's body, which the exe shows only as the same entry in
            # every class inheriting it; confirmed once the whole family is compared
            if entry["owner"] != src_owner and entry["owner"] != cls and src_owner == cls and not secondary \
                    and not pure and not exe.has_vtable(entry["owner"]):
                shared.setdefault((entry["owner"], slot), []).append((label, theirs[slot]))
            elif entry["owner"] not in (src_owner, owner) and not folded and not hidden:
                kind = overrider_kind(order, cls, owner, entry["owner"])
                # the body to compare the exe's entry with: for a missing override, the entry src's overrider
                # stands for; for an extra one, the address src registers its override at
                if kind == EXE_ONLY:
                    other = entry_of(order, offsets, offset, entry["owner"], slot)
                else:
                    other = next(iter(REGISTERED_AT.get((entry["owner"], entry["name"]), [])), None)
                report(kind, cls, label, slot, f"{label} slot {slot}: final overrider {entry['text']!r}, the exe's "
                       f"entry {theirs[slot]:#x} belongs to {owner}", theirs[slot], entry["text"],
                       fold_note(exe, order, theirs[slot], other, (entry["owner"], entry["name"])))
            if pure != (theirs[slot] == PURECALL):
                report(PURECALL_KIND, cls, label, slot, f"{label} slot {slot}: {entry['text']!r} is "
                       f"{'' if pure else 'not '}pure, the exe's entry {theirs[slot]:#x} is "
                       f"{'a body' if pure else 'the purecall stub'}", theirs[slot], entry["text"])
            body = theirs[slot]
            if secondary and theirs[slot] != PURECALL:
                adjust, body = exe.thunk(theirs[slot])
                if not isinstance(entry["adjust"], int):
                    report(ADJUSTMENT_KIND, cls, label, slot, f"{label} slot {slot}: {entry['text']!r} adjusts this "
                           f"by {entry['adjust']} (virtual inheritance is not supported)", theirs[slot], entry["text"])
                elif entry["adjust"] != adjust:
                    report(ADJUSTMENT_KIND, cls, label, slot, f"{label} slot {slot}: {entry['text']!r} adjusts this "
                           f"by {entry['adjust']}, the exe's entry {theirs[slot]:#x} by {adjust}", theirs[slot],
                           entry["text"])
            if entry["owner"] == cls and not pure:
                checked.append((cls, label, slot, entry, body))
                if secondary and theirs[slot] != PURECALL:
                    own_secondary.append((cls, label, slot, entry["name"], theirs[slot], adjust, at_offset))

    for cls in family:
        theirs = exe_tables[cls]
        try:
            owners_of(cls)
        except SystemExit as failure:
            report(SKIPPED, cls, cls, None, f"{cls}: {str(failure.code).removeprefix('vtable-check: ')}")
            continue
        ours = tables.get(cls)
        at_zero = secondaries.get(cls, {}).get(0)
        if ours is None and at_zero:
            # src's vfptr at offset 0 comes through bases outside the exe's primary chain: bases the exe has no
            # vtable for (BSSimpleArray's src-only BSSimpleArrayRoot) are src's own way to declare the slots, the table
            # is the primary one; a base with an exe vtable of its own is a different chain
            plain = {without_arguments(c) for c in chains[cls]}
            foreign = [c for c in at_zero["path"][1:] if without_arguments(c) not in plain]
            if any(exe.has_vtable(c) for c in foreign):
                report(BASE_CHAIN, cls, cls, None, f"{cls}: src's primary vtable comes through "
                       f"{' > '.join(at_zero['path'])}, the exe's primary chain is {' > '.join([cls, *exe.bases(cls)])}")
                continue
            ours = tables[cls] = secondaries[cls].pop(0)["entries"]
        if ours is None:
            # a class whose layout probe did not compile is already reported
            if not any(e.text.startswith(f"{cls}: the layout probe") for e in errors):
                report(SKIPPED, cls, cls, None, f"{cls}: no vtable layout from the compiler")
            continue
        order = [cls, *exe.bases(cls)]
        compare(cls, cls, ours, theirs, owners[cls], False, order, [0] * len(order), 0)

    for (base, slot), users in shared.items():
        # a base with no exe vtable (shared is filled only for those) that the exe's RTTI shows with exactly one
        # derived class, the class taking its body: one class can never show the entry shared, so its entry is
        # the base's. BasicArrayImplementation<unsigned char, 1> and <float, 3> (hkDataObjectDict.cpp) are each
        # the base of only ByteArrayImplementation / RealArrayImplementation.
        sole_heir = len(users) == 1 and exe.heirs(base) == {users[0][0]}
        if (len(users) < 2 and not sole_heir) or len({entry for _, entry in users}) != 1:
            report(UNSHARED, base, base, slot, f"{base} slot {slot}: the classes "
                   f"{', '.join(f'{c} ({e:#x})' for c, e in users)} take the base's body, which the exe does not "
                   f"show as one entry shared by two classes", users[0][1])

    # secondary vtables: matched with the exe's by the offset of their subobject (clang's record layout, the
    # exe's complete object locators)
    secondary_counts = []
    # the classes with a secondary table the exe gives no base table to compare with (a COM base's)
    unchecked: set[str] = set()
    for cls in family:
        ours_at = secondaries.get(cls, {})
        if not exe.has_locator(cls):
            if ours_at:
                warnings.append(f"{cls}: no RTTI in the exe, its secondary vtables (at "
                                f"{', '.join(f'{o:#x}' for o in sorted(ours_at))}) are unchecked")
            continue
        exe_offsets = exe.secondary_offsets(cls)
        for offset in exe_offsets:
            if offset not in ours_at:
                warnings.append(f"{cls}+{offset:#x}: the exe's vtable for {exe.subobject(cls, offset)} at this "
                                f"offset has no polymorphic base in src (unchecked)")
        for offset, table in sorted(ours_at.items()):
            label, path, offsets = f"{cls}+{offset:#x}", table["path"], table["offsets"]
            where = " in ".join(path[::-1])
            # the path's bases where src places them, against the exe's RTTI (the exe's owners are computed along
            # src's path, so a base out of place must not reach the slot comparison)
            exe_bases = exe.base_layout(cls)
            misplaced = [f"{c} at {at:#x} (the exe: "
                         f"{', '.join(f'{o:#x}' for b, o in exe_bases if b == c and o is not None) or 'none'})"
                         for c, at in zip(path[1:], offsets[1:]) if (c, at) not in exe_bases]
            if misplaced:
                report(SECONDARY_LAYOUT, cls, label, None, f"{label}: src places {'; '.join(misplaced)}")
                continue
            # each hop of the path a direct base in the exe: a class the exe has in between would vanish from the
            # owner walk, its overrides credited to the class above it
            skipped, at = None, 0
            for hop, (base, base_at) in enumerate(zip(path[1:], offsets[1:])):
                chain = exe.base_chain(cls, at, (base, base_at))
                if chain is None:
                    skipped = f"src's path {path[hop]} > {base}, the exe's {path[hop]} has no base {base} at " \
                              f"{base_at - offsets[hop]:#x}"
                    break
                if len(chain) > 1:
                    skipped = f"src's path {path[hop]} > {base}, the exe's " + \
                              " > ".join([path[hop]] + [exe_bases[index][0] for index in chain])
                    break
                at = chain[0]
            if skipped:
                report(SECONDARY_LAYOUT, cls, label, None, f"{label}: {skipped}")
                continue
            if offset not in exe_offsets:
                report(SECONDARY_LAYOUT, cls, label, None, f"{label}: src has a vtable for {where}, the exe has none "
                       f"at this offset (its secondary vtables: {', '.join(f'{o:#x}' for o in exe_offsets) or 'none'})")
                continue
            unlisted = [c for c in path[1:] if c not in listed]
            if unlisted:
                report(SECONDARY_LAYOUT, cls, label, None, f"{label}: {', '.join(unlisted)} (of {where}) not in "
                       f"{listed_as}: the tables this one derives from are unchecked")
                continue
            missing = [f"{c}+{offset - at:#x}" for c, at in zip(path[1:], offsets[1:])
                       if not (exe.has_vtable(c) if offset == at else offset - at in exe.secondary_offsets(c))]
            if missing:
                # the exe has no own table for a base of the path (DirectShow's abstract CBasePin): the slots' owners
                # cannot be told, the table is not compared
                unchecked.add(cls)
                report(PARTIAL, cls, label, None, f"{label}: no exe vtable for {', '.join(missing)} (of {where})")
                continue
            theirs = exe_table(cls, offset)
            secondary_counts.append((label, len(theirs)))
            compare(cls, label, table["entries"], theirs, owners_at(cls, offset, path, offsets), True, path, offsets,
                    offset, frozenset(c for c, at in zip(path, offsets) if at == offset))

    sized = [(cls, label, slot, entry, exe.ret(body)) for cls, label, slot, entry, body in checked
             if not entry["name"].startswith("~")]
    bodies = {(label, slot): body for _, label, slot, _, body in checked}
    entries = {(label, slot): entry for _, label, slot, entry, _ in sized}

    def report_stack(found: list[tuple[str, str | None, int | None, str]]) -> None:
        for cls, label, slot, text in found:
            if slot is None:
                report(PARTIAL, cls, cls, None, text)
            else:
                report(STACK_BYTES, cls, label, slot, text, bodies.get((label, slot)),
                       entries.get((label, slot), {}).get("text", ""))
    report_stack(stack_errors(sized))
    if verbose:
        for _, label, slot, entry, want in sized:
            out.append(f"{label} slot {slot}: {entry['text']} = {bodies[label, slot]:#x}, ret {sorted(want)}")
    # overloads of one name whose exe entries pop the same bytes: the stack check cannot tell their order
    groups: dict[tuple[str, str, str], list[tuple[int, frozenset[int]]]] = {}
    for cls, label, slot, entry, want in sized:
        groups.setdefault((cls, label, entry["name"]), []).append((slot, frozenset(want)))
    for (cls, label, name), members in groups.items():
        pops = [want for _, want in members]
        if len(members) > 1 and len(set(pops)) < len(pops) and (cls, name) not in ORDER_CHECKED:
            warnings.append(f"{label}::{name} slots {', '.join(str(s) for s, _ in members)}: overloads pop the "
                            f"same bytes, their order is unchecked")

    # a pure virtual with a body: its slots hold the purecall stub, derived overrides call it directly, so the
    # registered address is checked by its `ret N` alone
    pure_bodies = []
    for virtual, address, cls, member, where, via in registered:
        if cls not in family:
            continue
        own = [s for s, e in enumerate(tables.get(cls, [])) if e["owner"] == cls]
        # an override a secondary table holds is registered at its body (the slot may hold an adjustor thunk)
        other = [(label, slot, entry["name"], body) for c, label, slot, entry, body in checked
                 if c == cls and label != cls]
        # an entry without a thunk takes the subobject as this (MS ABI: the base that introduced the slot)
        direct = [(label, slot, bases) for c, label, slot, name, entry, adjust, bases in own_secondary
                  if c == cls and name == member and entry == address and adjust == 0]
        primary = any(exe_tables[cls][s] == address for s in own if s < len(exe_tables[cls]))
        if via is not None:
            if not any(via in bases for _, _, bases in direct):
                expected = ", ".join(f"{label} slot {s} ({entry:#x}, "
                                     f"{'thunk ' + str(adjust) if adjust else 'subobject ' + '/'.join(sorted(b))})"
                                     for c, label, s, name, entry, adjust, b in own_secondary
                                     if c == cls and name == member)
                # a class whose secondary tables were not compared has no entries to find the registration in
                report(PARTIAL if cls in unchecked and not expected else REGISTRATION, cls, cls, None, f"{where}: "
                       f"{cls}::{member} registered VIA {via} at {address:#x}: not {cls}'s thunk-less entry in a "
                       f"secondary vtable of {via} ({expected or 'no such entry'})", address)
            continue
        if virtual and direct and not primary:
            label, slot, bases = direct[0]
            report(REGISTRATION, cls, label, slot, f"{where}: {cls}::{member} at {address:#x} is {label} slot {slot} "
                   f"without an adjustor thunk: the exe passes the {'/'.join(sorted(bases))} subobject, register it "
                   f"with FNVD_REPLACES_VIRTUAL_VIA({address:#x}, {cls}, {sorted(bases)[-1]}, {member})", address)
            continue
        if virtual:
            slots = [s for s in own if tables[cls][s]["name"] == member]
            exe_slots = [s for s in slots if s < len(exe_tables[cls])]
            if exe_slots and all(exe_tables[cls][s] == PURECALL for s in exe_slots):
                pure_bodies.append((cls, cls, exe_slots[0], tables[cls][exe_slots[0]], exe.ret(address)))
                if verbose:
                    out.append(f"{cls} slot {exe_slots[0]}: {member} pure, body registered at {address:#x}, "
                               f"ret {sorted(exe.ret(address))}")
            elif not any(exe_tables[cls][s] == address for s in exe_slots) and \
                    not any(body == address for _, _, name, body in other if name == member):
                expected = ", ".join([f"slot {s}: {exe_tables[cls][s]:#x}" for s in exe_slots] +
                                     [f"{label} slot {s}: {body:#x}" for label, s, name, body in other
                                      if name == member])
                report(REGISTRATION, cls, cls, None, f"{where}: {cls}::{member} registered at {address:#x}, its exe "
                       f"entries are {expected or 'none (not a virtual of ' + cls + ')'}", address)
        else:
            slots = [f"vtable slot {s}" for s, owner in enumerate(owners[cls])
                     if owner == cls and exe_tables[cls][s] == address]
            slots += [f"{label} slot {s}" for label, s, _, body in other if body == address]
            if slots:
                report(REGISTRATION, cls, cls, None, f"{where}: {cls}::{member} at {address:#x} is {cls}'s "
                       f"{slots[0]}: make it a virtual member and register it with FNVD_REPLACES_VIRTUAL", address)
    if pure_bodies:
        for cls, label, slot, entry, want in pure_bodies:
            entries[label, slot] = entry
        report_stack(stack_errors(pure_bodies))
    counts = [f"{c} {len(exe_tables[c])}" for c in family] + [f"{l} {n}" for l, n in secondary_counts]
    out.append(f"vtable-check: {', '.join(counts)} slots, {len(checked)} own entries checked")
    return out, errors, warnings


VFTABLE_CONSTANT = re.compile(r"FNVD_VFTABLE\(\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\)")


def all_families(exe: "Exe", skips: list[tuple[str, str]]) -> list[list[str]]:
    """--all's families (see the docstring); a constant the tool cannot place goes to `skips` as (constant,
    reason)."""
    scan_headers()
    constants: dict[int, str] = {}
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.*"), recursive=True)):
        if not path.endswith((".h", ".inl", ".cpp")):
            continue
        with open(path, encoding="utf-8", errors="replace") as source:
            for match in VFTABLE_CONSTANT.finditer(source.read()):
                constants.setdefault(int(match.group(2), 16), match.group(1))
    classes: list[str] = []
    for address, constant in sorted(constants.items()):
        placed = exe.vtable_class(address)
        if placed is None:
            stem = re.fullmatch(r"(\w+)_vftable", constant)
            name = stem.group(1) if stem else None
            if name in DEFINITIONS and not exe.has_vtable(name):
                # no RTTI and no IDA name: the constant's address is the class's table
                exe.named_vtables[name] = address
                placed = (name, 0)
            else:
                secondary = re.fullmatch(r"(\w+?)_vftable_(\w+)", constant)
                if name in DEFINITIONS:
                    reason = f"the exe's vtable of {name} is elsewhere"
                elif secondary and secondary.group(1) in DEFINITIONS:
                    reason = f"a base subobject's table of {secondary.group(1)} (for {secondary.group(2)}), which " \
                             f"has no RTTI: unchecked"
                else:
                    reason = "the constant's name is no class src defines (a template instance: its arguments " \
                             "cannot be read back from the name)"
                skips.append((constant, f"{address:#x}: no RTTI locator or IDA vtable name; {reason}"))
                continue
        cls, offset = placed
        if offset:
            # a base subobject's table: its class's check covers it
            continue
        try:
            found = exe.vtable_address(cls)
        except SystemExit as failure:
            skips.append((cls, f"{constant} ({address:#x}): {str(failure.code).removeprefix('vtable-check: ')}"))
            continue
        if found != address:
            skips.append((cls, f"{constant} ({address:#x}): the class's exe vtable is at {found:#x}"))
            continue
        classes.append(cls)
    for cls in list(classes):
        # its primary chain and the bases its secondary tables come through (the RTTI base class array)
        for base in dict.fromkeys([*exe.bases(cls), *(name for name, _ in exe.base_layout(cls)[1:])]):
            plain = without_arguments(base.removeprefix("msvc9::"))
            if exe.has_vtable(base) and (DEFINITIONS.get(plain) or SPACED.get(plain)):
                classes.append(base)
    classes += [exe.canonical(cls) for family in FAMILIES for cls in family]
    grouped: dict[str, list[str]] = {}
    for cls in dict.fromkeys(classes):
        grouped.setdefault(([cls] + exe.bases(cls))[-1], []).append(cls)
    families = [sorted(members, key=lambda c: (len(exe.bases(c)), c)) for members in grouped.values()]
    return sorted(families, key=lambda members: members[0])


# the arguments of the --all worker processes (set before they fork)
STATE: list = []


def run_family(task: tuple[int, list[str]]) -> tuple[int, tuple[list[str], list[Finding], list[str]], float]:
    """check_family in an --all run: a family the probes cannot handle at all is skipped as a whole."""
    import time
    index, family = task
    exe, listed, listed_as, registered, verbose = STATE
    started = time.time()
    try:
        result = check_family(exe, family, listed, listed_as, registered, verbose)
    except SystemExit as failure:
        PROBE_ERRORS.clear()
        reason = str(failure.code).removeprefix("vtable-check: ").splitlines()[0]
        result = [], [Finding(SKIPPED, cls, cls, None, f"{cls}: family {family[0]}: {reason}") for cls in family], []
    shutil.rmtree(work(), ignore_errors=True)
    return index, result, time.time() - started


def cell(text: object) -> str:
    return re.sub(r"[\t\r\n]+", " ", "" if text is None else str(text))


def main() -> int:
    global BUILD
    arguments = sys.argv[1:]

    def option(name: str, default: str | None = None) -> str | None:
        if name not in arguments:
            return default
        at = arguments.index(name)
        value = arguments[at + 1]
        del arguments[at:at + 2]
        return value
    verbose = "-v" in arguments
    everything = "--all" in arguments
    # commas inside template arguments (`NiTMap<const char *, Setting *>`) do not separate classes
    chosen = option("--family")
    tsv, skipped_tsv, jobs = option("--tsv"), option("--skipped"), int(option("--jobs", "8"))
    folders = [a for a in arguments if a not in ("-v", "--all")]
    BUILD = os.path.abspath(folders[0]) if folders else os.path.join(ROOT, "build", "hybrid")
    exe = Exe()
    registered = registrations()
    fold_data(registered)
    placement: list[tuple[str, str]] = []
    if everything:
        families = all_families(exe, placement)
        listed, listed_as = {name for family in families for name in family}, "--all's classes"
        # a base without an exe vtable of its own (a COM interface) has no table to check in a family of its own
        listed |= {name for cls in list(listed) for name, _ in exe.base_layout(cls)[1:] if not exe.has_vtable(name)}
        if chosen:
            # template instances may be spelled without clang's spaces (`A<B,C>`, as IDA spells them)
            wanted = {exe.canonical(cls) for cls in split_params(chosen)}
            families = [family for family in families if wanted & set(family)]
    else:
        families = [split_params(chosen)] if chosen else FAMILIES
        families = [[exe.canonical(cls) for cls in family] for family in families]
        listed, listed_as = {name for family in FAMILIES for name in family}, "FAMILIES"
    STATE[:] = [exe, listed, listed_as, registered, verbose]
    tasks = list(enumerate(families))
    if everything and jobs > 1:
        # what the workers share, built once before they fork
        exe.has_locator("")
        exe.heirs("")
        exe.descriptor_templates()
        exe.code_references()
        scan_headers()
        hybrid_command()
        # the largest families first: the run ends with the small ones
        tasks.sort(key=lambda task: -len(task[1]))
        results: dict[int, tuple[list[str], list[Finding], list[str]]] = {}
        with multiprocessing.get_context("fork").Pool(jobs) as pool:
            for index, result, seconds in pool.imap_unordered(run_family, tasks):
                results[index] = result
                print(f"vtable-check: [{len(results)}/{len(tasks)}] {families[index][0]} ({len(families[index])} "
                      f"classes) {seconds:.0f} s", file=sys.stderr, flush=True)
        ordered = [results[index] for index in range(len(families))]
    elif everything:
        ordered = [run_family(task)[1] for task in tasks]
    else:
        ordered = [check_family(exe, family, listed, listed_as, registered, verbose) for family in families]

    errors: list[Finding] = []
    warnings: list[str] = []
    for out, found, warned in ordered:
        for line in out:
            print(line)
        errors += found
        warnings += warned
    for warning in warnings:
        print("warning: " + warning)
    for error in errors:
        print(error.text)
    mismatches = [e for e in errors if e.kind not in (SKIPPED, PARTIAL)] if everything else errors
    if everything:
        for constant, reason in placement:
            print(f"skipped: {constant}: {reason}")
        family_of = {cls: family[0] for family in families for cls in family}
        not_checked = {e.cls for e in errors if e.kind == SKIPPED}
        print(f"vtable-check: {len(family_of) - len(not_checked)} classes in {len(families)} families checked, "
              f"{len(not_checked)} skipped, {len(placement)} constants not placed")
        names = fn.ida_names()

        def folder(cls: str) -> str:
            try:
                return os.path.relpath(header(cls), os.path.join(ROOT, "src")).split(os.sep)[0]
            except SystemExit:
                return ""
        if tsv:
            with open(tsv, "w") as table:
                table.write("class\tfamily\tfolder\tvtable\tslot\tkind\texe entry\tsrc overrider\tfold\tdetail\n")
                for e in mismatches:
                    where = "primary" if e.label == e.cls else e.label[len(e.cls):] if e.label.startswith(e.cls) \
                        else e.label
                    named = ""
                    if e.entry is not None:
                        named = f"{e.entry:#x} " + (REGISTERED_NAMES.get(e.entry, [None])[0] or
                                                    names.get(e.entry, ""))
                    table.write("\t".join(cell(v) for v in (
                        e.cls, family_of.get(e.cls, ""), folder(e.cls), where, e.slot, e.kind, named.strip(), e.src,
                        e.fold, e.text)) + "\n")
        if skipped_tsv:
            with open(skipped_tsv, "w") as table:
                table.write("class\tfamily\tscope\treason\n")
                for constant, reason in placement:
                    table.write(f"{cell(constant)}\t\tnot placed\t{cell(reason)}\n")
                for e in errors:
                    if e.kind in (SKIPPED, PARTIAL):
                        scope = "class" if e.kind == SKIPPED else "partly"
                        table.write(f"{cell(e.cls)}\t{cell(family_of.get(e.cls, ''))}\t{scope}\t{cell(e.text)}\n")
    print(f"vtable-check: {len(mismatches)} mismatch(es)")
    shutil.rmtree(work(), ignore_errors=True)
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
