#include "Havok/Source/Common/SceneData/Scene/hkxScene.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Camera/hkxCamera.h"
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"
#include "Havok/Source/Common/SceneData/Light/hkxLight.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureFile.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureInplace.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"
#include "Havok/Source/Common/SceneData/Selection/hkxNodeSelectionSet.h"
#include "Havok/Source/Common/SceneData/Skin/hkxSkinBinding.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

// The strings, the root node and the arrays start empty (their constructors), the transform as the identity.
hkxScene::hkxScene() : m_sceneLength(0.0f), m_appliedTransform(hkMatrix3::getIdentity()) {}

// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
hkxScene::hkxScene(hkFinishLoadedObjectFlag aFlag)
  : hkReferencedObject(aFlag), m_modeller(aFlag), m_asset(aFlag), m_rootNode(aFlag), m_selectionSets(aFlag),
    m_cameras(aFlag), m_lights(aFlag), m_meshes(aFlag), m_materials(aFlag), m_inplaceTextures(aFlag),
    m_externalTextures(aFlag), m_skinBindings(aFlag) {}

// The members' destructors release the referenced objects and the strings and free the arrays.
hkxScene::~hkxScene() {}

hkxNode* hkxScene::findNodeByName(const char* name) const {
    if (!name) {
        return nullptr;
    }
    hkxNode* m_pntr = m_rootNode.val();
    if (!m_pntr) {
        return nullptr;
    }
    const char* m_stringAndFlag = m_pntr->m_name.m_stringAndFlag;
    if ((reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~static_cast<uintptr_t>(1)) == 0 ||
        hkString::strCasecmp(
            reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~static_cast<uintptr_t>(1)),
            name
        )) {
        return m_rootNode.val()->findDescendantByName(name);
    }

    return m_rootNode.val();
}

hkResult
hkxScene::getFullPathToNode(const hkxNode* theNode, hkArray<const hkxNode*, hkContainerHeapAllocator>& pathOut) {
    return m_rootNode.val()->getPathToNode(theNode, pathOut);
}

// The product of the key frames (key, clamped to each node's last) along the path from the root to theNode.
hkResult hkxScene::getWorldFromNodeTransform(const hkxNode* theNode, hkMatrix4& worldFromNodeOut, int32_t key) {
    alignas(16) hkInplaceArray<const hkxNode*, 32> pathOut;
    hkResult result;
    if (getFullPathToNode(theNode, pathOut)) {
        result = HK_FAILURE;
    }
    else {
        worldFromNodeOut.setIdentity();
        for (const hkxNode* pNode : pathOut) {
            int32_t numKeys = pNode->m_keyFrames.m_size;
            int32_t frame = key >= numKeys ? numKeys - 1 : key;
            worldFromNodeOut.mul(&pNode->m_keyFrames.m_data[frame]);
        }
        result = HK_SUCCESS;
    }
    return result;
}
