#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"

class hkxCamera;
class hkxLight;
class hkxMaterial;
class hkxMesh;
class hkxNodeSelectionSet;
class hkxSkinBinding;
class hkxTextureFile;
class hkxTextureInplace;

// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxScene();
// The finish-loading function of hkxScene's type info.
void finishLoadedObjecthkxScene(void* p, int32_t finishing);
void cleanupLoadedObjecthkxScene(void* p);

// The objects, resources and coordinate transform of an exported scene.
class hkxScene : public hkReferencedObject {
public:
    hkxScene();
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote (defined in hkxScene.cpp, where
    // the referenced classes are complete).
    hkxScene(hkFinishLoadedObjectFlag aFlag);
    ~hkxScene() override;

    hkxNode* findNodeByName(const char* name) const;
    hkResult getFullPathToNode(const hkxNode* theNode, hkArray<const hkxNode*, hkContainerHeapAllocator>& pathOut);
    hkResult getWorldFromNodeTransform(const hkxNode* theNode, hkMatrix4& worldFromNodeOut, int32_t key);

    static const hkClass& staticClass();

    hkStringPtr m_modeller;
    hkStringPtr m_asset;
    float m_sceneLength;
    hkRefPtr<hkxNode> m_rootNode;
    hkArray<hkRefPtr<hkxNodeSelectionSet>> m_selectionSets;
    hkArray<hkRefPtr<hkxCamera>> m_cameras;
    hkArray<hkRefPtr<hkxLight>> m_lights;
    hkArray<hkRefPtr<hkxMesh>> m_meshes;
    hkArray<hkRefPtr<hkxMaterial>> m_materials;
    hkArray<hkRefPtr<hkxTextureInplace>> m_inplaceTextures;
    hkArray<hkRefPtr<hkxTextureFile>> m_externalTextures;
    hkArray<hkRefPtr<hkxSkinBinding>> m_skinBindings;
    hkMatrix3 m_appliedTransform;
};

extern hkClass hkxSceneClass;
extern hkTypeInfo hkxSceneTypeInfo;
