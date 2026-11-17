#include "Havok/Source/Common/SceneData/AlignScene/hkAlignSceneToNodeOptions.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkAlignSceneToNodeOptions();
static void cleanupLoadedObjecthkAlignSceneToNodeOptions(void* p);

// The class record's member table.
using hkAlignSceneToNodeOptionsClassMemberTable = const hkClassMember[9];
static hkAlignSceneToNodeOptionsClassMemberTable hkAlignSceneToNodeOptionsClass_Members{
	{"invert", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"transformPositionX", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x9,
		nullptr},
	{"transformPositionY", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA,
		nullptr},
	{"transformPositionZ", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xB,
		nullptr},
	{"transformRotation", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr},
	{"transformScale", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xD, nullptr},
	{"transformSkew", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xE, nullptr},
	{"keyframe", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
	{"nodeName", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x14, nullptr}};
hkClass hkAlignSceneToNodeOptionsClass("hkAlignSceneToNodeOptions", &hkReferencedObjectClass, 24, nullptr, 0, nullptr,
	0, hkAlignSceneToNodeOptionsClass_Members, 9, nullptr, nullptr, 0, 2);
hkTypeInfo hkAlignSceneToNodeOptionsTypeInfo{"hkAlignSceneToNodeOptions", finishLoadedObjecthkAlignSceneToNodeOptions,
	cleanupLoadedObjecthkAlignSceneToNodeOptions, getVtablehkAlignSceneToNodeOptions(),
	&typeid(hkAlignSceneToNodeOptions), sizeof(hkAlignSceneToNodeOptions)};

static const void* getVtablehkAlignSceneToNodeOptions() {
	return hkGetVtableOf<hkAlignSceneToNodeOptions>();
}

static void cleanupLoadedObjecthkAlignSceneToNodeOptions(void* p) {
	static_cast<hkAlignSceneToNodeOptions*>(p)->~hkAlignSceneToNodeOptions();
}

// The flags and the keyframe are deliberately left unset, as the game does.
hkAlignSceneToNodeOptions::hkAlignSceneToNodeOptions() : m_nodeName() {}

// The member frees the node name.
hkAlignSceneToNodeOptions::~hkAlignSceneToNodeOptions() {}

void hkAlignSceneToNodeOptions::setNodeName(const char* name) {
	m_nodeName.operator=(name);
}

const hkStringPtr& hkAlignSceneToNodeOptions::getNodeName() {
	return m_nodeName;
}

const hkClass& hkAlignSceneToNodeOptions::staticClass() {
	return hkAlignSceneToNodeOptionsClass;
}
