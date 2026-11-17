#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// The reflection data of hkpAction's class record, as the Havok SDK's class file has it: the member table and the
// class's custom attributes (a modeler-node-type attribute in .data, the one-attribute list and the record over it
// in .rdata). Not named in the PDB or IDA; names chosen here.
using hkpActionClass_MembersArray = const hkClassMember[4];
static hkpActionClass_MembersArray hkpActionClass_Members{{"world", nullptr, nullptr, {hkClassMember::TYPE_POINTER},
															  {hkClassMember::TYPE_VOID}, 0,
															  {hkClassMember::SERIALIZE_IGNORED}, 0x8, nullptr},
	{"island", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0xC, nullptr},
	{"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr}};
static hkModelerNodeTypeAttribute hkpAction_ModelerNodeType{{hkModelerNodeTypeAttribute::LOCATOR}};
using hkpActionAttributeArray = const hkCustomAttributes::Attribute[1];
static hkpActionAttributeArray hkpAction_AttributeList{
	{"hk.ModelerNodeType", {&hkpAction_ModelerNodeType, &hkModelerNodeTypeAttributeClass}}};
static const hkCustomAttributes hkpAction_Attributes{hkpAction_AttributeList, 1};

hkClass hkpActionClass("hkpAction", &hkReferencedObjectClass, 24, nullptr, 0, nullptr, 0, hkpActionClass_Members, 4,
	nullptr, &hkpAction_Attributes, 0, 0);

hkpAction::hkpAction(hkUlong userData) : m_world(nullptr), m_island(nullptr), m_userData(userData) {}

hkpAction::hkpAction(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_name(aFlag) {}

// The name goes with its member.
hkpAction::~hkpAction() {}
