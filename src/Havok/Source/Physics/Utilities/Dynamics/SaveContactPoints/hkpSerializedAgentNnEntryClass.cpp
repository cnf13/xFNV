#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntryClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"

// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
static const void* getVtablehkpSerializedAgentNnEntry();

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected members of
// hkpSerializedTrack1nInfo and hkpSerializedSubTrack1nInfo, as the exe's .rdata holds them.
using hkpSerializedTrack1nInfoClass_MembersArray = const hkClassMember[2];
static hkpSerializedTrack1nInfoClass_MembersArray hkpSerializedTrack1nInfoClass_Members{
	{"sectors", &hkpAgent1nSectorClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0, 0x0,
		nullptr},
	{"subTracks", &hkpSerializedSubTrack1nInfoClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_POINTER}, 0, 0, 0xC, nullptr}};
using hkpSerializedSubTrack1nInfoClass_MembersArray = const hkClassMember[2];
static hkpSerializedSubTrack1nInfoClass_MembersArray hkpSerializedSubTrack1nInfoClass_Members{
	{"sectorIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x18, nullptr},
	{"offsetInSector", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x1C, nullptr}};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enum of
// hkpSerializedAgentNnEntry, as the exe's .rdata holds it.
using hkpSerializedAgentNnEntrySerializedAgentTypeEnumItemsArray = const hkClassEnum::Item[10];
static hkpSerializedAgentNnEntrySerializedAgentTypeEnumItemsArray hkpSerializedAgentNnEntrySerializedAgentTypeEnumItems{
	{hkpSerializedAgentNnEntry::INVALID_AGENT_TYPE, "INVALID_AGENT_TYPE"},
	{hkpSerializedAgentNnEntry::BOX_BOX_AGENT3, "BOX_BOX_AGENT3"},
	{hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3, "CAPSULE_TRIANGLE_AGENT3"},
	{hkpSerializedAgentNnEntry::PRED_GSK_AGENT3, "PRED_GSK_AGENT3"},
	{hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3, "PRED_GSK_CYLINDER_AGENT3"},
	{hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3, "CONVEX_LIST_AGENT3"},
	{hkpSerializedAgentNnEntry::LIST_AGENT3, "LIST_AGENT3"},
	{hkpSerializedAgentNnEntry::BV_TREE_AGENT3, "BV_TREE_AGENT3"},
	{hkpSerializedAgentNnEntry::COLLECTION_COLLECTION_AGENT3, "COLLECTION_COLLECTION_AGENT3"},
	{hkpSerializedAgentNnEntry::COLLECTION_AGENT3, "COLLECTION_AGENT3"}};
using hkpSerializedAgentNnEntryEnumsArray = const hkClassEnum[1];
static hkpSerializedAgentNnEntryEnumsArray hkpSerializedAgentNnEntryEnums{
	{"SerializedAgentType", hkpSerializedAgentNnEntrySerializedAgentTypeEnumItems, 10, nullptr, 0}};

// The class's enum, by pointer.
static const hkClassEnum* hkpSerializedAgentNnEntrySerializedAgentTypeEnum = &hkpSerializedAgentNnEntryEnums[0];

// The PDB's name. The reflected members of hkpSerializedAgentNnEntry as the exe's .data holds them.
using hkpSerializedAgentNnEntryClass_MembersArray = hkClassMember[14];
static hkpSerializedAgentNnEntryClass_MembersArray hkpSerializedAgentNnEntryClass_Members{
	{"bodyA", &hkpEntityClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x8,
		nullptr},
	{"bodyB", &hkpEntityClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0xC,
		nullptr},
	{"bodyAId", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
	{"bodyBId", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, 0, 0x14, nullptr},
	{"useEntityIds", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x18, nullptr},
	{"agentType", nullptr, hkpSerializedAgentNnEntrySerializedAgentTypeEnum, {hkClassMember::TYPE_ENUM},
		{hkClassMember::TYPE_INT8}, 0, 0, 0x19, nullptr},
	{"atom", &hkpSimpleContactConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0,
		0, 0x20, nullptr},
	{"propertiesStream", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x50,
		nullptr},
	{"contactPoints", &hkContactPointClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, 0,
		0x5C, nullptr},
	{"cpIdMgr", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x68, nullptr},
	{"nnEntryData", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 160, 0, 0x74, nullptr},
	{"trackInfo", &hkpSerializedTrack1nInfoClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0,
		0, 0x114, nullptr},
	{"endianCheckBuffer", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 4, 0, 0x12C,
		nullptr},
	{"version", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x130, nullptr}};
hkClass hkpSerializedTrack1nInfoClass("hkpSerializedTrack1nInfo", nullptr, 24, nullptr, 0, nullptr, 0,
	hkpSerializedTrack1nInfoClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkpSerializedSubTrack1nInfoClass("hkpSerializedSubTrack1nInfo", &hkpSerializedTrack1nInfoClass, 32, nullptr, 0,
	nullptr, 0, hkpSerializedSubTrack1nInfoClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkpSerializedAgentNnEntryClass("hkpSerializedAgentNnEntry", &hkReferencedObjectClass, 320, nullptr, 0,
	hkpSerializedAgentNnEntryEnums, 1, hkpSerializedAgentNnEntryClass_Members, 14, nullptr, nullptr, 0, 0);
hkTypeInfo hkpSerializedAgentNnEntryTypeInfo = {"hkpSerializedAgentNnEntry",
	finishLoadedObjecthkpSerializedAgentNnEntry, cleanupLoadedObjecthkpSerializedAgentNnEntry,
	getVtablehkpSerializedAgentNnEntry(), &typeid(hkpSerializedAgentNnEntry), sizeof(hkpSerializedAgentNnEntry)};

// The SDK's getVtable: hkpSerializedAgentNnEntry's finish constructor (flag 0), inlined, on a 16-aligned stack object
// (its vptr store, then m_atom's finish constructor); returns the object's first word, the vptr.
static const void* getVtablehkpSerializedAgentNnEntry() {
	return hkGetVtableOf<hkpSerializedAgentNnEntry>();
}

void cleanupLoadedObjecthkpSerializedAgentNnEntry(void* p) {
	static_cast<hkpSerializedAgentNnEntry*>(p)->~hkpSerializedAgentNnEntry();
}

void finishLoadedObjecthkpSerializedTrack1nInfo(void* p, int32_t finishing) {
	;
}

void finishLoadedObjecthkpSerializedSubTrack1nInfo(void* p, int32_t finishing) {
	;
}

// The type info's finish-loaded-object callback for hkpSerializedAgentNnEntry: its finish constructor, inlined, with
// the exe's flag word.
void finishLoadedObjecthkpSerializedAgentNnEntry(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpSerializedAgentNnEntry(kFlag);
	}
}

void cleanupLoadedObjecthkpSerializedTrack1nInfo(void* p) {
	static_cast<hkpSerializedTrack1nInfo*>(p)->~hkpSerializedTrack1nInfo();
}

void cleanupLoadedObjecthkpSerializedSubTrack1nInfo(void* p) {
	static_cast<hkpSerializedTrack1nInfo*>(p)->~hkpSerializedTrack1nInfo();
}

const hkClass& hkpSerializedTrack1nInfo::staticClass() {
	return hkpSerializedTrack1nInfoClass;
}

const hkClass& hkpSerializedSubTrack1nInfo::staticClass() {
	return hkpSerializedSubTrack1nInfoClass;
}

const hkClass& hkpSerializedAgentNnEntry::staticClass() {
	return hkpSerializedAgentNnEntryClass;
}
