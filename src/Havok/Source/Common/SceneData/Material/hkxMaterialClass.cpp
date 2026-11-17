#include "Havok/Source/Common/SceneData/Material/hkxMaterialClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

namespace {
// The PDB's `anonymous-namespace'::hkxMaterialTextureStage_DefaultStruct: each member's offset in this struct (-1:
// texture and usageHint have no default), then tcoordChannel's default.
struct hkxMaterialTextureStage_DefaultStruct {
	int32_t s_defaultOffsets[3];
	int32_t m_tcoordChannel;
};
} // namespace

// KEEP: getVtablehkxMaterial (0xDFD9B0, cdecl) is not written. It placement-constructs an hkxMaterial in a 16-byte
// aligned stack union with the finish-loading constructor, inlined: the exe's vtable 0x10E33DC goes to the object's
// first word and 0xCB0A70 (thiscall, ret 4, `mov eax, ecx`) runs with 0 on the member at +0x14. It returns that first
// word. src cannot construct hkxMaterial (an inlined constructor that stores the exe's vtable).

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxMaterialTextureStageTypeInfo = {"hkxMaterialTextureStage", finishLoadedObjecthkxMaterialTextureStage,
	cleanupLoadedObjecthkxMaterialTextureStage, nullptr, &typeid(hkxMaterial::TextureStage),
	sizeof(hkxMaterial::TextureStage)};

// The reflection's enum records: the names of TextureType's values, and the enum (names chosen here, the Havok
// SDK's; .rdata).
using hkxMaterialTextureTypeEnumItemsArray = const hkClassEnum::Item[13];
static hkxMaterialTextureTypeEnumItemsArray hkxMaterialTextureTypeEnumItems{{0, "TEX_UNKNOWN"}, {1, "TEX_DIFFUSE"},
	{2, "TEX_REFLECTION"}, {3, "TEX_BUMP"}, {4, "TEX_NORMAL"}, {5, "TEX_DISPLACEMENT"}, {6, "TEX_SPECULAR"},
	{7, "TEX_SPECULARANDGLOSS"}, {8, "TEX_OPACITY"}, {9, "TEX_EMISSIVE"}, {10, "TEX_REFRACTION"}, {11, "TEX_GLOSS"},
	{12, "TEX_NOTEXPORTED"}};
using hkxMaterialEnumsArray = const hkClassEnum[1];
static hkxMaterialEnumsArray hkxMaterialEnums{{"TextureType", hkxMaterialTextureTypeEnumItems, 13, nullptr, {0}}};

// The PDB's name: the enum of hkxMaterial::TextureStage's usageHint, by pointer (.data).
static const hkClassEnum* hkxMaterialTextureTypeEnum = &hkxMaterialEnums[0];

// The reflection's member table of hkxMaterial::TextureStage (name chosen here, the Havok SDK's; .data). The
// usageHint member's enum is null in the image: the exe's initialiser 0xFC3430 (below) stores it; the data line holds
// it itself for the standalone build.
using hkxMaterial_TextureStageClass_MembersArray = const hkClassMember[3];
static hkxMaterial_TextureStageClass_MembersArray hkxMaterial_TextureStageClass_Members{
	{"texture", &hkReferencedObjectClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, {0},
		0x0, nullptr},
	{"usageHint", nullptr, &hkxMaterialEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT32}, 0, {0}, 0x4,
		nullptr},
	{"tcoordChannel", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr}};

// The defaults the class record names (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
static const hkxMaterialTextureStage_DefaultStruct hkxMaterialTextureStage_Default{{-1, -1, 0xC}, -1};

// The reflection's member table of hkxMaterial (name chosen here, the Havok SDK's; .rdata).
using hkxMaterialClass_MembersArray = const hkClassMember[8];
static hkxMaterialClass_MembersArray hkxMaterialClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
	{"stages", &hkxMaterialTextureStageClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0,
		{0}, 0x18, nullptr},
	{"diffuseColor", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30,
		nullptr},
	{"ambientColor", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40,
		nullptr},
	{"specularColor", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50,
		nullptr},
	{"emissiveColor", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x60,
		nullptr},
	{"subMaterials", &hkxMaterialClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0},
		0x70, nullptr},
	{"extraData", &hkReferencedObjectClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0,
		{0}, 0x7C, nullptr}};
hkClass hkxMaterialTextureStageClass("hkxMaterialTextureStage", nullptr, 0xC, nullptr, 0, nullptr, 0,
	hkxMaterial_TextureStageClass_Members, 3, &hkxMaterialTextureStage_Default, nullptr, 0, 0);
hkClass hkxMaterialClass("hkxMaterial", &hkxAttributeHolderClass, 0x80, nullptr, 0, hkxMaterialEnums, 1,
	hkxMaterialClass_Members, 8, nullptr, nullptr, 0, 0);
hkTypeInfo hkxMaterialTypeInfo = {"hkxMaterial", finishLoadedObjecthkxMaterial, cleanupLoadedObjecthkxMaterial,
	getVtablehkxMaterial(), &typeid(hkxMaterial), sizeof(hkxMaterial)};

void cleanupLoadedObjecthkxMaterial(void* p) {
	static_cast<hkxMaterial*>(p)->~hkxMaterial();
}

const void* getVtablehkxMaterial() {
	return hkGetVtableOf<hkxMaterial>();
}

// The type info's finish-loaded-object callback for hkxMaterial: the null check, then its finish constructor, which
// the exe inlines.
void finishLoadedObjecthkxMaterial(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkxMaterial(aFlag);
	}
}

const hkClass& hkxMaterial::TextureStage::staticClass() {
	return hkxMaterialTextureStageClass;
}

const hkClass& hkxMaterial::staticClass() {
	return hkxMaterialClass;
}
