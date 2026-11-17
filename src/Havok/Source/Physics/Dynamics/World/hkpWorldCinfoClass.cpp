#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"

// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpWorldCinfo();

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enums of
// hkpWorldCinfo, as the exe's .rdata holds them.
using hkpWorldCinfoSolverTypeEnumItemsArray = const hkClassEnum::Item[11];
static hkpWorldCinfoSolverTypeEnumItemsArray hkpWorldCinfoSolverTypeEnumItems{
	{hkpWorldCinfo::SOLVER_TYPE_INVALID, "SOLVER_TYPE_INVALID"},
	{hkpWorldCinfo::SOLVER_TYPE_2ITERS_SOFT, "SOLVER_TYPE_2ITERS_SOFT"},
	{hkpWorldCinfo::SOLVER_TYPE_2ITERS_MEDIUM, "SOLVER_TYPE_2ITERS_MEDIUM"},
	{hkpWorldCinfo::SOLVER_TYPE_2ITERS_HARD, "SOLVER_TYPE_2ITERS_HARD"},
	{hkpWorldCinfo::SOLVER_TYPE_4ITERS_SOFT, "SOLVER_TYPE_4ITERS_SOFT"},
	{hkpWorldCinfo::SOLVER_TYPE_4ITERS_MEDIUM, "SOLVER_TYPE_4ITERS_MEDIUM"},
	{hkpWorldCinfo::SOLVER_TYPE_4ITERS_HARD, "SOLVER_TYPE_4ITERS_HARD"},
	{hkpWorldCinfo::SOLVER_TYPE_8ITERS_SOFT, "SOLVER_TYPE_8ITERS_SOFT"},
	{hkpWorldCinfo::SOLVER_TYPE_8ITERS_MEDIUM, "SOLVER_TYPE_8ITERS_MEDIUM"},
	{hkpWorldCinfo::SOLVER_TYPE_8ITERS_HARD, "SOLVER_TYPE_8ITERS_HARD"},
	{hkpWorldCinfo::SOLVER_TYPE_MAX_ID, "SOLVER_TYPE_MAX_ID"}};
using hkpWorldCinfoSimulationTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpWorldCinfoSimulationTypeEnumItemsArray hkpWorldCinfoSimulationTypeEnumItems{
	{hkpWorldCinfo::SIMULATION_TYPE_INVALID, "SIMULATION_TYPE_INVALID"},
	{hkpWorldCinfo::SIMULATION_TYPE_DISCRETE, "SIMULATION_TYPE_DISCRETE"},
	{hkpWorldCinfo::SIMULATION_TYPE_CONTINUOUS, "SIMULATION_TYPE_CONTINUOUS"},
	{hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED, "SIMULATION_TYPE_MULTITHREADED"}};
using hkpWorldCinfoContactPointGenerationEnumItemsArray = const hkClassEnum::Item[3];
static hkpWorldCinfoContactPointGenerationEnumItemsArray hkpWorldCinfoContactPointGenerationEnumItems{
	{hkpWorldCinfo::CONTACT_POINT_ACCEPT_ALWAYS, "CONTACT_POINT_ACCEPT_ALWAYS"},
	{hkpWorldCinfo::CONTACT_POINT_REJECT_DUBIOUS, "CONTACT_POINT_REJECT_DUBIOUS"},
	{hkpWorldCinfo::CONTACT_POINT_REJECT_MANY, "CONTACT_POINT_REJECT_MANY"}};
using hkpWorldCinfoBroadPhaseBorderBehaviourEnumItemsArray = const hkClassEnum::Item[4];
static hkpWorldCinfoBroadPhaseBorderBehaviourEnumItemsArray hkpWorldCinfoBroadPhaseBorderBehaviourEnumItems{
	{hkpWorldCinfo::BROADPHASE_BORDER_ASSERT, "BROADPHASE_BORDER_ASSERT"},
	{hkpWorldCinfo::BROADPHASE_BORDER_FIX_ENTITY, "BROADPHASE_BORDER_FIX_ENTITY"},
	{hkpWorldCinfo::BROADPHASE_BORDER_REMOVE_ENTITY, "BROADPHASE_BORDER_REMOVE_ENTITY"},
	{hkpWorldCinfo::BROADPHASE_BORDER_DO_NOTHING, "BROADPHASE_BORDER_DO_NOTHING"}};
using hkpWorldCinfoTreeUpdateTypeEnumItemsArray = const hkClassEnum::Item[2];
static hkpWorldCinfoTreeUpdateTypeEnumItemsArray hkpWorldCinfoTreeUpdateTypeEnumItems{
	{hkpWorldCinfo::REBUILD_ACTIVE, "REBUILD_ACTIVE"}, {hkpWorldCinfo::REBUILD_ALL, "REBUILD_ALL"}};
using hkpWorldCinfoEnumsArray = const hkClassEnum[5];
static hkpWorldCinfoEnumsArray hkpWorldCinfoEnums{{"SolverType", hkpWorldCinfoSolverTypeEnumItems, 11, nullptr, {0}},
	{"SimulationType", hkpWorldCinfoSimulationTypeEnumItems, 4, nullptr, {0}},
	{"ContactPointGeneration", hkpWorldCinfoContactPointGenerationEnumItems, 3, nullptr, {0}},
	{"BroadPhaseBorderBehaviour", hkpWorldCinfoBroadPhaseBorderBehaviourEnumItems, 4, nullptr, {0}},
	{"TreeUpdateType", hkpWorldCinfoTreeUpdateTypeEnumItems, 2, nullptr, {0}}};

// The PDB's names: the enums by pointer (.data).
static const hkClassEnum* hkpWorldCinfoSolverTypeEnum = &hkpWorldCinfoEnums[0];
static const hkClassEnum* hkpWorldCinfoSimulationTypeEnum = &hkpWorldCinfoEnums[1];
static const hkClassEnum* hkpWorldCinfoContactPointGenerationEnum = &hkpWorldCinfoEnums[2];
static const hkClassEnum* hkpWorldCinfoBroadPhaseBorderBehaviourEnum = &hkpWorldCinfoEnums[3];
static const hkClassEnum* hkpWorldCinfoTreeUpdateTypeEnum = &hkpWorldCinfoEnums[4];

// The PDB's hkpWorldCinfoClass_Members (.data).
using hkpWorldCinfoClass_MembersArray = hkClassMember[54];
static hkpWorldCinfoClass_MembersArray hkpWorldCinfoClass_Members{
	{"gravity", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
	{"broadPhaseQuerySize", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
	{"contactRestingVelocity", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x24,
		nullptr},
	{"broadPhaseBorderBehaviour", nullptr, hkpWorldCinfoBroadPhaseBorderBehaviourEnum, hkClassMember::TYPE_ENUM,
		hkClassMember::TYPE_INT8, 0, 0, 0x28, nullptr},
	{"mtPostponeAndSortBroadPhaseBorderCallbacks", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID,
		0, 0, 0x29, nullptr},
	{"broadPhaseWorldAabb", &hkAabbClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x30,
		nullptr},
	{"useKdTree", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x50, nullptr},
	{"useMultipleTree", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x51, nullptr},
	{"treeUpdateType", nullptr, hkpWorldCinfoTreeUpdateTypeEnum, hkClassMember::TYPE_ENUM, hkClassMember::TYPE_INT8, 0,
		0, 0x52, nullptr},
	{"autoUpdateKdTree", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x53, nullptr},
	{"collisionTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x54, nullptr},
	{"collisionFilter", &hkpCollisionFilterClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0,
		0, 0x58, nullptr},
	{"convexListFilter", &hkpConvexListFilterClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0,
		0, 0x5C, nullptr},
	{"expectedMaxLinearVelocity", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x60,
		nullptr},
	{"sizeOfToiEventQueue", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x64, nullptr},
	{"expectedMinPsiDeltaTime", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x68,
		nullptr},
	{"memoryWatchDog", &hkWorldMemoryAvailableWatchDogClass, nullptr, hkClassMember::TYPE_POINTER,
		hkClassMember::TYPE_STRUCT, 0, 0, 0x6C, nullptr},
	{"broadPhaseNumMarkers", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x70,
		nullptr},
	{"contactPointGeneration", nullptr, hkpWorldCinfoContactPointGenerationEnum, hkClassMember::TYPE_ENUM,
		hkClassMember::TYPE_INT8, 0, 0, 0x74, nullptr},
	{"allowToSkipConfirmedCallbacks", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x75,
		nullptr},
	{"solverTau", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x78, nullptr},
	{"solverDamp", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x7C, nullptr},
	{"solverIterations", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x80, nullptr},
	{"solverMicrosteps", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x84, nullptr},
	{"maxConstraintViolation", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x88,
		nullptr},
	{"forceCoherentConstraintOrderingInSolver", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0,
		0, 0x8C, nullptr},
	{"snapCollisionToConvexEdgeThreshold", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0,
		0x90, nullptr},
	{"snapCollisionToConcaveEdgeThreshold", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0,
		0x94, nullptr},
	{"enableToiWeldRejection", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x98,
		nullptr},
	{"enableDeprecatedWelding", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x99,
		nullptr},
	{"iterativeLinearCastEarlyOutDistance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0,
		0x9C, nullptr},
	{"iterativeLinearCastMaxIterations", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0,
		0xA0, nullptr},
	{"deactivationNumInactiveFramesSelectFlag0", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID,
		0, 0, 0xA4, nullptr},
	{"deactivationNumInactiveFramesSelectFlag1", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID,
		0, 0, 0xA5, nullptr},
	{"deactivationIntegrateCounter", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0xA6,
		nullptr},
	{"shouldActivateOnRigidBodyTransformChange", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID,
		0, 0, 0xA7, nullptr},
	{"deactivationReferenceDistance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xA8,
		nullptr},
	{"toiCollisionResponseRotateNormal", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0,
		0xAC, nullptr},
	{"maxSectorsPerCollideTask", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0xB0,
		nullptr},
	{"processToisMultithreaded", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0xB4,
		nullptr},
	{"maxEntriesPerToiCollideTask", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0xB8,
		nullptr},
	{"maxNumToiCollisionPairsSinglethreaded", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0,
		0, 0xBC, nullptr},
	{"numToisTillAllowedPenetrationSimplifiedToi", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID,
		0, 0, 0xC0, nullptr},
	{"numToisTillAllowedPenetrationToi", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0,
		0xC4, nullptr},
	{"numToisTillAllowedPenetrationToiHigher", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0,
		0, 0xC8, nullptr},
	{"numToisTillAllowedPenetrationToiForced", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0,
		0, 0xCC, nullptr},
	{"enableDeactivation", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0xD0, nullptr},
	{"simulationType", nullptr, hkpWorldCinfoSimulationTypeEnum, hkClassMember::TYPE_ENUM, hkClassMember::TYPE_INT8, 0,
		0, 0xD1, nullptr},
	{"enableSimulationIslands", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0xD2,
		nullptr},
	{"minDesiredIslandSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0xD4,
		nullptr},
	{"processActionsInSingleThread", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0xD8,
		nullptr},
	{"allowIntegrationOfIslandsWithoutConstraintsInASeparateJob", nullptr, nullptr, hkClassMember::TYPE_BOOL,
		hkClassMember::TYPE_VOID, 0, 0, 0xD9, nullptr},
	{"frameMarkerPsiSnap", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xDC, nullptr},
	{"fireCollisionCallbacks", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0xE0,
		nullptr}};

// The SDK's getVtable: hkpWorldCinfo's finish constructor (flag 0), out of line, on a 16-aligned stack object;
// returns the object's first word, the vptr that constructor stores: the exe's table, an untyped exe address.
static const void* getVtablehkpWorldCinfo() {
	return hkGetVtableOf<hkpWorldCinfo>();
}

namespace {
// The PDB's `anonymous-namespace'::hkpWorldCinfo_DefaultStruct: per member its offset into this struct (-1: no
// default), then the defaults. m_gravity is the SDK's _hkVector4 (four floats, no alignment), the hkBool defaults
// its one-byte _hkBool.
struct hkpWorldCinfo_DefaultStruct {
	int32_t s_defaultOffsets[54];
	float m_gravity[4];
	int32_t m_broadPhaseQuerySize;
	uint8_t m_autoUpdateKdTree;
	float m_collisionTolerance;
	float m_expectedMaxLinearVelocity;
	int32_t m_sizeOfToiEventQueue;
	float m_expectedMinPsiDeltaTime;
	float m_solverDamp;
	int32_t m_solverIterations;
	int32_t m_solverMicrosteps;
	float m_maxConstraintViolation;
	float m_snapCollisionToConvexEdgeThreshold;
	float m_snapCollisionToConcaveEdgeThreshold;
	float m_iterativeLinearCastEarlyOutDistance;
	int32_t m_iterativeLinearCastMaxIterations;
	uint8_t m_shouldActivateOnRigidBodyTransformChange;
	float m_deactivationReferenceDistance;
	float m_toiCollisionResponseRotateNormal;
	int32_t m_maxSectorsPerCollideTask;
	uint8_t m_processToisMultithreaded;
	int32_t m_maxEntriesPerToiCollideTask;
	int32_t m_maxNumToiCollisionPairsSinglethreaded;
	float m_numToisTillAllowedPenetrationSimplifiedToi;
	float m_numToisTillAllowedPenetrationToi;
	float m_numToisTillAllowedPenetrationToiHigher;
	float m_numToisTillAllowedPenetrationToiForced;
	uint8_t m_enableDeactivation;
	uint8_t m_enableSimulationIslands;
	uint32_t m_minDesiredIslandSize;
	uint8_t m_processActionsInSingleThread;
	float m_frameMarkerPsiSnap;
};
} // namespace
// The defaults the class record names (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
static const hkpWorldCinfo_DefaultStruct hkpWorldCinfo_Default{
	{0xD8, 0xE8, -1, -1, -1, -1, -1, -1, -1, 0xEC, 0xF0, -1, -1, 0xF4, 0xF8, 0xFC, -1, -1, -1, -1, -1, 0x100, 0x104,
		0x108, 0x10C, -1, 0x110, 0x114, -1, -1, 0x118, 0x11C, -1, -1, -1, 0x120, 0x124, 0x128, 0x12C, 0x130, 0x134,
		0x138, 0x13C, 0x140, 0x144, 0x148, 0x14C, -1, 0x14D, 0x150, 0x154, -1, 0x158, -1},
	{0.0f, -9.8f, 0.0f, 0.0f}, 1024, 1, 0.1f, 200.0f, 250, 0.033333335f, 0.6f, 4, 1, 3.40282e+38f, 0.524f, 0.698f,
	0.01f, 20, 1, 0.02f, 0.2f, 4, 1, 1, 1, 3.0f, 3.0f, 4.0f, 20.0f, 1, 1, 64, 1, 0.0001f};

hkClass hkpWorldCinfoClass("hkpWorldCinfo", &hkReferencedObjectClass, 240, nullptr, 0, hkpWorldCinfoEnums, 5,
	hkpWorldCinfoClass_Members, 54, &hkpWorldCinfo_Default, nullptr, 0, 8);
hkTypeInfo hkpWorldCinfoTypeInfo = {"hkpWorldCinfo", finishLoadedObjecthkpWorldCinfo, cleanupLoadedObjecthkpWorldCinfo,
	getVtablehkpWorldCinfo(), &typeid(hkpWorldCinfo), sizeof(hkpWorldCinfo)};

// The finish function the reflection registers for hkpWorldCinfo (cdecl, 2 stack arguments, 0xD6AE90): the null
// check, then the info's out-of-line finish constructor, which sets the vtable and defaults
// m_contactRestingVelocity when the load left it zero. The flag's dword is handed on unchanged (0xD6AE98).
void finishLoadedObjecthkpWorldCinfo(void* p, int32_t finishing) {
	hkpWorldCinfo* apInfo = static_cast<hkpWorldCinfo*>(p);
	if (!apInfo) {
		return;
	}
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = finishing;
	::new (apInfo) hkpWorldCinfo(flag);
}

void cleanupLoadedObjecthkpWorldCinfo(void* p) {
	static_cast<hkpWorldCinfo*>(p)->~hkpWorldCinfo();
}

const hkClass& hkpWorldCinfo::staticClass() {
	return hkpWorldCinfoClass;
}
