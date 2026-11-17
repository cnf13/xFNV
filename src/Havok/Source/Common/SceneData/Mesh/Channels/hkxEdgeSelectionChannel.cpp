#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxEdgeSelectionChannel.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxEdgeSelectionChannel(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxEdgeSelectionChannel();
static void cleanupLoadedObjecthkxEdgeSelectionChannel(void* p);

// The class record's member table.
using hkxEdgeSelectionChannelClassMemberArray = const hkClassMember[1];
static hkxEdgeSelectionChannelClassMemberArray hkxEdgeSelectionChannelClass_Members{
    {"selectedEdges", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, 0, 0x8, nullptr}
};
hkClass hkxEdgeSelectionChannelClass(
    "hkxEdgeSelectionChannel",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkxEdgeSelectionChannelClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxEdgeSelectionChannelTypeInfo = {
    "hkxEdgeSelectionChannel",
    finishLoadedObjecthkxEdgeSelectionChannel,
    cleanupLoadedObjecthkxEdgeSelectionChannel,
    getVtablehkxEdgeSelectionChannel(),
    &typeid(hkxEdgeSelectionChannel),
    sizeof(hkxEdgeSelectionChannel)
};

// The type info's finish-loaded-object callback for hkxEdgeSelectionChannel: its finish constructor, when the
// object is not null.
static void finishLoadedObjecthkxEdgeSelectionChannel(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxEdgeSelectionChannel(flag);
    }
}

static const void* getVtablehkxEdgeSelectionChannel() {
    return hkGetVtableOf<hkxEdgeSelectionChannel>();
}

static void cleanupLoadedObjecthkxEdgeSelectionChannel(void* p) {
    static_cast<hkxEdgeSelectionChannel*>(p)->~hkxEdgeSelectionChannel();
}

// hkxEdgeSelectionChannel's reflection data and objects.

const hkClass& hkxEdgeSelectionChannel::staticClass() {
    return hkxEdgeSelectionChannelClass;
}
