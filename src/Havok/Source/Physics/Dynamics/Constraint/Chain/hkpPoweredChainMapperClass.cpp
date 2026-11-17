#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Utilities/Constraint/Chain/hkpPoweredChainMapper.h"

static void finishLoadedObjecthkpPoweredChainMapper(void* p, int32_t finishing);

using hkpPoweredChainMapper_TargetClass_MembersArray = const hkClassMember[2];
static hkpPoweredChainMapper_TargetClass_MembersArray hkpPoweredChainMapper_TargetClass_Members{
	{"chain", &hkpPoweredChainDataClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, {0},
		0x0, nullptr},
	{"infoIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}};
using hkpPoweredChainMapper_LinkInfoClass_MembersArray = const hkClassMember[3];
static hkpPoweredChainMapper_LinkInfoClass_MembersArray hkpPoweredChainMapper_LinkInfoClass_Members{
	{"firstTargetIdx", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"numTargets", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
	{"limitConstraint", &hkpConstraintInstanceClass, nullptr, {hkClassMember::TYPE_POINTER},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr}};
using hkpPoweredChainMapperClass_MembersArray = const hkClassMember[3];
static hkpPoweredChainMapperClass_MembersArray hkpPoweredChainMapperClass_Members{
	{"links", &hkpPoweredChainMapperLinkInfoClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT},
		0, {0}, 0x8, nullptr},
	{"targets", &hkpPoweredChainMapperTargetClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT},
		0, {0}, 0x14, nullptr},
	{"chains", &hkpConstraintChainInstanceClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0,
		{0}, 0x20, nullptr}};
hkClass hkpPoweredChainMapperTargetClass("hkpPoweredChainMapperTarget", nullptr, 8, nullptr, 0, nullptr, 0,
	hkpPoweredChainMapper_TargetClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkpPoweredChainMapperLinkInfoClass("hkpPoweredChainMapperLinkInfo", nullptr, 12, nullptr, 0, nullptr, 0,
	hkpPoweredChainMapper_LinkInfoClass_Members, 3, nullptr, nullptr, 0, 0);
hkClass hkpPoweredChainMapperClass("hkpPoweredChainMapper", &hkReferencedObjectClass, 44, nullptr, 0, nullptr, 0,
	hkpPoweredChainMapperClass_Members, 3, nullptr, nullptr, 0, 0);
hkTypeInfo hkpPoweredChainMapperTypeInfo = {"hkpPoweredChainMapper", finishLoadedObjecthkpPoweredChainMapper,
	cleanupLoadedObjecthkpPoweredChainMapper, getVtablehkpPoweredChainMapper(), &typeid(hkpPoweredChainMapper),
	sizeof(hkpPoweredChainMapper)};

// The type info's finish-loaded-object callback for hkpPoweredChainMapper: the null check, then its finish constructor.
static void finishLoadedObjecthkpPoweredChainMapper(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpPoweredChainMapper(kFlag);
	}
}

const void* getVtablehkpPoweredChainMapper() {
	return hkGetVtableOf<hkpPoweredChainMapper>();
}

const hkClass& hkpPoweredChainMapper::LinkInfo::staticClass() {
	return hkpPoweredChainMapperLinkInfoClass;
}

const hkClass& hkpPoweredChainMapper::Target::staticClass() {
	return hkpPoweredChainMapperTargetClass;
}

const hkClass& hkpPoweredChainMapper::staticClass() {
	return hkpPoweredChainMapperClass;
}

void cleanupLoadedObjecthkpPoweredChainMapperTarget(void* p) {
	;
}

void cleanupLoadedObjecthkpPoweredChainMapperLinkInfo(void* p) {
	;
}

void cleanupLoadedObjecthkpPoweredChainMapper(void* p) {
	static_cast<hkpPoweredChainMapper*>(p)->~hkpPoweredChainMapper();
}
