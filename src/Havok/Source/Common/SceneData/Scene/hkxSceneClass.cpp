#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Camera/hkxCamera.h"
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"
#include "Havok/Source/Common/SceneData/Light/hkxLight.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialClass.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureFile.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureInplace.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"
#include "Havok/Source/Common/SceneData/Scene/hkxScene.h"
#include "Havok/Source/Common/SceneData/Selection/hkxNodeSelectionSet.h"

namespace {
    // Each member's offset in this struct (-1: no default), then appliedTransform's default, the identity (_hkMatrix3,
    // twelve floats: no 16-byte alignment here).
    struct hkxScene_DefaultStruct {
        int32_t s_defaultOffsets[13];
        float m_appliedTransform[12];
    };
} // namespace

// KEEP: getVtablehkxScene is not written. It placement-constructs an hkxScene in a 16-byte aligned stack union with
// the finish-loading constructor, inlined: the class's vtable goes to the object's first word and a finish-loading
// constructor that only returns its object (by its use hkStringPtr's) runs on m_modeller and m_asset. It returns that
// first word. src cannot construct hkxScene (an inlined constructor that stores the vtable).

// The reflection's member table.
using hkxSceneClass_MembersArray = const hkClassMember[13];
static hkxSceneClass_MembersArray hkxSceneClass_Members{
    {"modeller", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"asset", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"sceneLength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"rootNode",
     &hkxNodeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x14,
     nullptr},
    {"selectionSets",
     &hkxNodeSelectionSetClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x18,
     nullptr},
    {"cameras",
     &hkxCameraClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x24,
     nullptr},
    {"lights",
     &hkxLightClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x30,
     nullptr},
    {"meshes",
     &hkxMeshClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x3C,
     nullptr},
    {"materials",
     &hkxMaterialClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x48,
     nullptr},
    {"inplaceTextures",
     &hkxTextureInplaceClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x54,
     nullptr},
    {"externalTextures",
     &hkxTextureFileClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x60,
     nullptr},
    {"skinBindings",
     &hkxSkinBindingClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x6C,
     nullptr},
    {"appliedTransform",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_MATRIX3},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x80,
     nullptr}
};

// The defaults the class record names.
static const hkxScene_DefaultStruct hkxScene_Default{
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x34},
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}
};
hkClass hkxSceneClass(
    "hkxScene",
    &hkReferencedObjectClass,
    0xB0,
    nullptr,
    0,
    nullptr,
    0,
    hkxSceneClass_Members,
    13,
    &hkxScene_Default,
    nullptr,
    0,
    1
);
hkTypeInfo hkxSceneTypeInfo{
    "hkxScene",
    finishLoadedObjecthkxScene,
    cleanupLoadedObjecthkxScene,
    getVtablehkxScene(),
    &typeid(hkxScene),
    sizeof(hkxScene)
};

const void* getVtablehkxScene() {
    return hkGetVtableOf<hkxScene>();
}

void cleanupLoadedObjecthkxScene(void* p) {
    static_cast<hkxScene*>(p)->~hkxScene();
}

const hkClass& hkxScene::staticClass() {
    return hkxSceneClass;
}
