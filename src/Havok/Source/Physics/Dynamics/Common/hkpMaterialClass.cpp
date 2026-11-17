#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void cleanupLoadedObjecthkpMaterial(void* p);
static void finishLoadedObjecthkpMaterial(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpMaterialTypeInfo = {"hkpMaterial", finishLoadedObjecthkpMaterial, cleanupLoadedObjecthkpMaterial, nullptr,
	&typeid(hkpMaterial), sizeof(hkpMaterial)};

// The reflection data of hkpMaterial's class record, as the Havok SDK's class file has it: the names of
// ResponseType's values and the enum record (.rdata), the enum pointer and the member table (.data). Not named in the
// PDB or IDA unless said; names chosen here.
using hkpMaterialResponseTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpMaterialResponseTypeEnumItemsArray hkpMaterialResponseTypeEnumItems{{0, "RESPONSE_INVALID"},
	{1, "RESPONSE_SIMPLE_CONTACT"}, {2, "RESPONSE_REPORTING"}, {3, "RESPONSE_NONE"}, {4, "RESPONSE_MAX_ID"}};
using hkpMaterialEnumsArray = const hkClassEnum[1];
static hkpMaterialEnumsArray hkpMaterialEnums{{"ResponseType", hkpMaterialResponseTypeEnumItems, 5, nullptr, {0}}};

// The PDB's name. The ResponseType enum record, as the member table's responseType row names it.
static const hkClassEnum* hkpMaterialResponseTypeEnum = &hkpMaterialEnums[0];

// The responseType row's enum is null in the image: the exe's initialiser 0xFBFA90 stores it.
using hkpMaterialClass_MembersArray = hkClassMember[3];
static hkpMaterialClass_MembersArray hkpMaterialClass_Members{{"responseType", nullptr, &hkpMaterialEnums[0],
																  {hkClassMember::TYPE_ENUM},
																  {hkClassMember::TYPE_INT8}, 0, {0}, 0x0, nullptr},
	{"friction", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
	{"restitution", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr}};
hkClass hkpMaterialClass("hkpMaterial", nullptr, 12, nullptr, 0, hkpMaterialEnums, 1, hkpMaterialClass_Members, 3,
	nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkpMaterial(void* p) {
	;
}

static void finishLoadedObjecthkpMaterial(void* p, int32_t finishing) {
	;
}

const hkClass& hkpMaterial::staticClass() {
	return hkpMaterialClass;
}
