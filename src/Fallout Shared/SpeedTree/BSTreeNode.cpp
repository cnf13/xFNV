#include "Fallout Shared/SpeedTree/BSTreeNode.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBoxShape.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkMultiSphereShape.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSPCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkTransformShape.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <xmmintrin.h>

// Whether trees get Havok phantoms.
static SettingT<INISettingCollection> bTreeTops("bTreeTops:HAVOK", false);
SettingT<GameSettingCollection> gs_iCollFreq("iCollFreq", 2);
SettingT<GameSettingCollection> iLowDamp("iLowDamp", 50);
SettingT<GameSettingCollection> gs_iHighDamp("iHighDamp", 80);
SettingT<GameSettingCollection> gs_iUpdateGroups("iUpdateGroups", 5);
SettingT<GameSettingCollection> fUpdateInterval("fUpdateInterval", 0.25f);
NiRTTI BSTreeNode::ms_RTTI("BSTreeNode", &NiNode::ms_RTTI);

BSTreeNode::BSTreeNode(BSTreeModel* apModel, bhkRigidBody* apRigid)
  : spModel(apModel), pspBranches(nullptr), pspLeaves(nullptr), spBillboard(nullptr), fNextCollisionCheck(0.0f) {
    SetLODMultType(LOD_MULT_DISTANTLOD);
    SetSelectiveUpdate(true);
    NiNode::SetAt(0, new NiNode(0));
    NiNode::SetAt(1u, new NiNode(0));
    NiBillboardNode* pnodeBB = new NiBillboardNode();
    pnodeBB->SetMode(NiBillboardNode::ROTATE_ABOUT_UP);
    pnodeBB->SetRotate(-1.5707964f, 1.0f, 0.0f, 0.0f);
    NiNode::SetAt(2u, pnodeBB);
    uint16_t slevels = spModel.m_pObject->GetNumBranchLODLevels();
    if (slevels) {
        pspBranches = new NiPointer<NiTriBasedGeom>[slevels];
    }
    uint16_t slevels_2 = spModel.m_pObject->GetNumLeafLODLevels();
    if (slevels_2) {
        pspLeaves = new NiPointer<NiTriBasedGeom>[slevels_2];
    }
    AddExtraData(BSTreeManager::GetHavokFlags());
    bhkRigidBody* prigid = apRigid ? apRigid : spModel.m_pObject->GetRigidBody();
    if (prigid) {
        bhkCollisionObject* pcoll = new bhkCollisionObject();
        pcoll->SetWorldObject(prigid);
        pcoll->SetSceneGraphObject(this);
    }
}

BSTreeNode::~BSTreeNode() {
    delete[] pspBranches;
    pspBranches = nullptr;
    delete[] pspLeaves;
    pspLeaves = nullptr;
}

void BSTreeNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    char* message_2;
    NiNode::GetViewerStrings(pkStrings);
    char* message = NiGetViewerString("BSTreeNode");
    pkStrings->Add(message);
    if (spModel.m_pObject) {
        message_2 = static_cast<char*>(ourMalloc(32u));
        if (spModel.m_pObject->IsInstance()) {
            BSsprintf(message_2, 32, "Type = Instance");
        }
        else {
            BSsprintf(message_2, 32, "Type = Base");
        }
        pkStrings->Add(message_2);
        message_2 = static_cast<char*>(ourMalloc(32u));
        int32_t ParentCell = spModel.m_pObject->iSeed;
        BSsprintf(message_2, 32, "Seed = %d", ParentCell);
        pkStrings->Add(message_2);
    }
    else {
        message_2 = static_cast<char*>(ourMalloc(32u));
        BSsprintf(message_2, 32, "-No Model-");
        pkStrings->Add(message_2);
    }
}

void BSTreeNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    NiCamera* Camera;
    AutoMemContext apMemory(
        MEM_CONTEXT_TREEART, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeNode.cpp",
        525
    );
    if (BSTreeManager::GetTreesEnabled() &&
        (!(UpdateData.fTime > 0.0) || (Camera = BSTreeManager::GetCamera(), NodeInFrustum(this, Camera)))) {
        UpdateBSTreeNode(UpdateData.fTime);
        NiNode::UpdateDownwardPass(UpdateData, uFlags);
    }
    else {}
}

void BSTreeNode::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    NiCamera* Camera;
    AutoMemContext apMemory(
        MEM_CONTEXT_TREEART, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeNode.cpp",
        547
    );
    if (BSTreeManager::GetTreesEnabled() &&
        (!(UpdateData.fTime > 0.0) || (Camera = BSTreeManager::GetCamera(), NodeInFrustum(this, Camera)))) {
        UpdateBSTreeNode(UpdateData.fTime);
        NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
    }
    else {}
}

// Trees are drawn only while the tree manager has them enabled.
void BSTreeNode::OnVisible(NiCullingProcess& arCuller) {
    if (BSTreeManager::GetTreesEnabled()) {
        NiNode::OnVisible(arCuller);
    }
}

BSTreeModel* BSTreeNode::GetModel() const {
    return spModel.m_pObject;
}

NiNode* BSTreeNode::GetBranchNode() {
    return static_cast<NiNode*>(GetAt(0));
}

NiNode* BSTreeNode::GetLeafNode() {
    return static_cast<NiNode*>(GetAt(1));
}

NiNode* BSTreeNode::GetBillboardNode() {
    return static_cast<NiNode*>(GetAt(2));
}

NiTriBasedGeom* BSTreeNode::GetBranchLevel(uint16_t asLevel) {
    if (!spModel.m_pObject || !pspBranches || asLevel >= spModel.m_pObject->GetNumBranchLODLevels()) {
        return nullptr;
    }

    return pspBranches[asLevel].m_pObject;
}

NiTriBasedGeom* BSTreeNode::GetLeafLevel(uint16_t asLevel) {
    if (!spModel.m_pObject || !pspLeaves || asLevel >= spModel.m_pObject->GetNumLeafLODLevels()) {
        return nullptr;
    }

    return pspLeaves[asLevel].m_pObject;
}

NiTriBasedGeom* BSTreeNode::GetBillboard() {
    return spBillboard.m_pObject;
}

// Replaces the geometry of one level of detail under the branch node. Constructs an empty string it never uses and
// destroys it on every return, as the game does.
bool BSTreeNode::AddBranchLevel(uint16_t asLevel, NiTriBasedGeom* apGeom) {
    BSStringT<char> kUnused;
    if (!spModel.m_pObject || !pspBranches || !apGeom) {
        return false;
    }
    if (asLevel >= spModel.m_pObject->GetNumBranchLODLevels()) {
        return false;
    }

    NiNode* pNode = GetBranchNode();
    if (!pNode) {
        return false;
    }

    if (pspBranches[asLevel].m_pObject) {
        pNode->DetachChild(pspBranches[asLevel].m_pObject);
    }
    pNode->AttachChild(apGeom, true);
    pspBranches[asLevel] = apGeom;
    return true;
}

// Replaces the geometry of one level of detail under the leaf node. An empty string is constructed, never used, and
// destroyed on every return, as the game does.
bool BSTreeNode::AddLeafLevel(uint16_t asLevel, NiTriBasedGeom* apGeom) {
    BSStringT<char> kUnused;
    if (!spModel.m_pObject || !pspLeaves || !apGeom) {
        return false;
    }
    if (asLevel >= spModel.m_pObject->GetNumLeafLODLevels()) {
        return false;
    }

    NiNode* pNode = GetLeafNode();
    if (!pNode) {
        return false;
    }

    if (pspLeaves[asLevel].m_pObject) {
        pNode->DetachChild(pspLeaves[asLevel].m_pObject);
    }
    pNode->AttachChild(apGeom, true);
    pspLeaves[asLevel] = apGeom;
    return true;
}

void BSTreeNode::SetBillboard(NiTriBasedGeom* apGeom) {
    if (!apGeom) {
        return;
    }

    NiNode* pNode = GetBillboardNode();
    if (!pNode) {
        return;
    }

    if (spBillboard.m_pObject) {
        pNode->DetachChild(spBillboard.m_pObject);
    }
    pNode->AttachChild(apGeom, true);
    spBillboard = apGeom;
}

bool BSTreeNode::SetVisibilityForBranches(bool abVisible) {
    NiNode* pNode = GetBranchNode();
    if (!pNode) {
        return false;
    }

    pNode->SetAppCulled(!abVisible);
    return true;
}

bool BSTreeNode::SetVisibilityForLeaves(bool abVisible) {
    NiNode* pNode = GetLeafNode();
    if (!pNode) {
        return false;
    }

    pNode->SetAppCulled(!abVisible);
    return true;
}

bool BSTreeNode::SetVisibilityForBillboards(bool abVisible) {
    NiNode* pNode = GetBillboardNode();
    if (!pNode) {
        return false;
    }

    pNode->SetAppCulled(!abVisible);
    return true;
}

// An alpha test reference of 255 hides the geometry; any other shows it and becomes its alpha test reference.
bool BSTreeNode::SetVisibilityOnBranch(uint16_t asLevel, uint8_t acTest) {
    NiTriBasedGeom* pGeometry = GetBranchLevel(asLevel);
    if (!pGeometry) {
        return false;
    }

    if (acTest == 255) {
        pGeometry->SetAppCulled(true);
    }
    else {
        pGeometry->SetAppCulled(false);
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(pGeometry->GetProperty(NiProperty::ALPHA));
        if (pAlpha) {
            pAlpha->SetTestRef(acTest);
        }
    }
    return true;
}

bool BSTreeNode::SetVisibilityOnLeaf(uint16_t asLevel, uint8_t acTest) {
    NiTriBasedGeom* pGeometry = GetLeafLevel(asLevel);
    if (!pGeometry) {
        return false;
    }

    if (acTest == 255) {
        pGeometry->SetAppCulled(true);
    }
    else {
        pGeometry->SetAppCulled(false);
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(pGeometry->GetProperty(NiProperty::ALPHA));
        if (pAlpha) {
            pAlpha->SetTestRef(acTest);
        }
    }
    return true;
}

bool BSTreeNode::SetVisibilityOnBillboard(uint8_t acTest) {
    if (!spBillboard.m_pObject) {
        return false;
    }

    if (acTest == 255) {
        spBillboard.m_pObject->SetAppCulled(true);
    }
    else {
        spBillboard.m_pObject->SetAppCulled(false);
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(spBillboard.m_pObject->GetProperty(NiProperty::ALPHA));
        if (pAlpha) {
            pAlpha->SetTestRef(acTest);
        }
    }
    return true;
}

// Swaps the billboard's texturing property; fails when it is already the given one.
bool BSTreeNode::SetTexturePropOnBillboard(NiTexturingProperty* apProp) {
    if (!spBillboard.m_pObject || !apProp) {
        return false;
    }

    NiProperty* pOld = spBillboard.m_pObject->GetProperty(NiProperty::TEXTURING);
    if (pOld) {
        if (pOld == apProp) {
            return false;
        }
        spBillboard.m_pObject->RemoveProperty(NiProperty::TEXTURING);
    }
    spBillboard.m_pObject->AttachProperty(apProp);
    return true;
}

void BSTreeNode::UpdateBSTreeNode(float afTime) {
    ImpactMixer::COLLISION_SOUND csdata_2;
    ImpactMixer::COLLISION_SOUND csdata;
    if (spModel.m_pObject) {
        BSTreeModel* v59 = this->spModel.m_pObject;
        v59->Update(this);
    }
    if (this->CheckForCollision(afTime)) {
        bhkSimpleShapePhantom* pTree = this->GetLeafObject();
        if ((pTree) && (pTree->GethkWorld())) {
            int32_t v56;
            const char* UserData = this->spModel.m_pObject->pSpeedTree->GetUserData();
            const char* pCollisionFreq = std::strstr(UserData, "~CF");
            if (pCollisionFreq) {
                v56 = pCollisionFreq[3] - 48;
            }
            else {
                v56 = gs_iCollFreq.Int();
            }
            double v55 = static_cast<double>(v56);
            float v54 = fUpdateInterval.Float() * v55 * 100.0;
            alignas(16) hkpAllCdBodyPairCollector collector;
            pTree->GetPenetrations(&collector);
            const hkArray<hkpRootCdBodyPair, hkContainerHeapAllocator>* collArray = &collector.m_hits;
            for (int32_t i = 0;; ++i) {
                int32_t FormHealth_0 = collArray->m_size;
                if (i >= FormHealth_0) {
                    break;
                }
                hkpPhantom* phkpPhantom =
                    hkpGetPhantom((&collArray->operator[]<hkpRootCdBodyPair>(i))->m_rootCollidableB);
                hkpRigidBody* pRigidBody =
                    hkpGetRigidBody((&collArray->operator[]<hkpRootCdBodyPair>(i))->m_rootCollidableB);
                if (phkpPhantom) {
                    MobileObject* pMobObj =
                        static_cast<MobileObject*>(bhkCharacterController::FindBSReference(1000u, phkpPhantom));
                    if (pMobObj) {
                        bhkCharacterController* pc = pMobObj->GetCharController();
                        if (pc) {
                            alignas(16) NiPoint3 dum;
                            hkpRootCdPoint point;
                            float a4 = RandomFloat0To1();
                            float a3 = RandomFloat0To1();
                            float v10 = RandomFloat0To1();
                            hkVector4 normal(v10, a3, a4, 0.0f);
                            normal.fastNormalize3();
                            hkVector4 vLocation((&collArray->operator[]<hkpRootCdBodyPair>(i))
                                                    ->m_rootCollidableB->m_motion.pState->m_transform.m_translation);
                            point.m_contact.set(vLocation, normal, 0.0f);
                            point.m_rootCollidableA = pTree->GetCollidable();
                            point.m_rootCollidableB = (&collArray->operator[]<hkpRootCdBodyPair>(i))->m_rootCollidableB;
                            point.m_shapeKeyB = 0;
                            point.m_shapeKeyA = 0;
                            pc->contactPointAddedCallback(nullptr, point);
                            ::new (&csdata) NiBound;
                            csdata.cMaterialType1 = 4;
                            csdata.cMaterialType2 = 7;
                            csdata.fFriction = 1.0f;
                            pc->GetLinearVelocity(dum);
                            csdata.fVelocity = dum.Length();
                            // The source's sound ID is the sum of the two objects' (32-bit) addresses.
                            csdata.iID = static_cast<int32_t>(
                                reinterpret_cast<uintptr_t>(phkpPhantom) + reinterpret_cast<uintptr_t>(pTree)
                            );
                            csdata.Location = HK2NI(dum, vLocation);
                            csdata.pBody1 = nullptr;
                            csdata.pBody2 = nullptr;
                            ImpactMixer::PlayCollisionSound(csdata);
                        }
                    }
                }
                else {
                    if (pRigidBody) {
                        int32_t v41 = RandomInt(0, 100);
                        if (v54 >= static_cast<double>(v41)) {
                            HavokMaterialType Material;
                            alignas(16) NiPoint3 dum_2;
                            hkVector4 vLV(pRigidBody->getLinearVelocity());
                            hkVector4 vAV(pRigidBody->getAngularVelocity());
                            if (vLV.lengthSquared3().operator float() > 1.0) {
                                double v36 = vLV.length3().operator float();
                                int32_t v35 = RandomInt(iLowDamp.Int(), gs_iHighDamp.Int());
                                float v34 = (1.0 - static_cast<double>(v35) / 100.0) * v36;
                                float fNewX = RandomFloatNeg1To1();
                                float fNewY = RandomFloatNeg1To1();
                                float fNewZ = RandomFloatNeg1To1() - 0.25;
                                hkVector4 vNewVel(fNewX, fNewY, fNewZ, 0.0f);
                                vNewVel.fastNormalize3();
                                hkSimdReal v29(v34);
                                vNewVel.mul4(v29);
                                pRigidBody->setLinearVelocity(vNewVel);
                            }
                            if (vAV.lengthSquared3().operator float() > 1.0) {
                                double v26 = vAV.length3().operator float();
                                int32_t v25 = RandomInt(0, 20);
                                float v24 = (static_cast<double>(v25) / 100.0 + 90.0) * v26;
                                float fNewX_2 = RandomFloatNeg1To1();
                                float fNewY_2 = RandomFloatNeg1To1();
                                float fNewZ_2 = RandomFloatNeg1To1() - 0.25;
                                hkVector4 vNewVel_2(fNewX_2, fNewY_2, fNewZ_2, 0.0f);
                                vNewVel_2.fastNormalize3();
                                hkSimdReal v19(v24);
                                vNewVel_2.mul4(v19);
                                pRigidBody->setAngularVelocity(vNewVel_2);
                            }
                            ::new (&csdata_2) NiBound;
                            bhkShape* pShape = bhkShape::Getbhk(
                                (&collArray->operator[]<hkpRootCdBodyPair>(i))->m_rootCollidableB->m_shape
                            );
                            csdata_2.cMaterialType1 = 4;
                            if (pShape) {
                                Material = pShape->GetMaterial();
                            }
                            else {
                                Material = BHK_MATERIAL_STONE;
                            }
                            csdata_2.cMaterialType2 = Material;
                            csdata_2.fFriction = 1.0f;
                            float a5 = vLV.length3().operator float();
                            csdata_2.fVelocity = HK2NI(a5);
                            // The source's sound ID is the sum of the two objects' (32-bit) addresses.
                            csdata_2.iID = static_cast<int32_t>(
                                reinterpret_cast<uintptr_t>(pRigidBody) + reinterpret_cast<uintptr_t>(pTree)
                            );
                            const hkTransform* Transform = &pRigidBody->getTransform();
                            csdata_2.Location = HK2NI(dum_2, Transform->m_translation);
                            csdata_2.pBody1 = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pRigidBody));
                            csdata_2.pBody2 = nullptr;
                            ImpactMixer::PlayCollisionSound(csdata_2);
                        }
                    }
                }
            }
        }
    }
}

bool BSTreeNode::CheckForCollision(float afTime) {
    float* v3;
    double v4;
    int8_t v6;
    if (afTime == 0.0) {
        return true;
    }
    if (fNextCollisionCheck == 0.0) {
        v4 = (double)(afTime + (&fUpdateInterval)->Float());
        v3 = &(&fUpdateInterval)->Float();
        fNextCollisionCheck =
            (float)((((*v3) / ((double)(&gs_iUpdateGroups)->Int())) * ((double)BSTreeNode::ucNextGroup)) + v4);
        ++BSTreeNode::ucNextGroup;
        BSTreeNode::ucNextGroup =
            (uint8_t)(((int32_t)BSTreeNode::ucNextGroup) % ((int32_t)(&gs_iUpdateGroups)->UInt()));
        return true;
    }

    v6 = 0;
    if (!(fNextCollisionCheck < ((double)afTime))) {
        return (bool)v6;
    }
    fNextCollisionCheck = (afTime + (&fUpdateInterval)->Float());
    return true;
}

// Gives the tree's leaf node a simple-shape phantom built from the tree's collision objects, converted to Havok units
// and scaled by fTreeSizeConversion times afScale: one sphere (placed by a transform shape), capsule or box (placed by
// a transform shape, its dimensions halved), or a multi-sphere shape of several spheres. The Havok infos are aligned to
// 16. The capsule's lower end is computed in double, as the game does, then rounded to float.
void BSTreeNode::MakeHavokLeaves(CSpeedTreeRT* apST, float afScale) {
    if (!(bTreeTops == true)) {
        return;
    }
    uint32_t uiCount = apST->GetCollisionObjectCount();
    if (!uiCount) {
        return;
    }
    float fScale = GameSetting::fTreeSizeConversion::Get() * afScale;
    NiNode* pLeafNode = GetLeafNode();
    alignas(16) uint8_t aPhantomInfoStorage[sizeof(hkSimpleShapePhantomCinfo)];
    hkSimpleShapePhantomCinfo* pPhantomInfo = ::new (aPhantomInfoStorage) hkSimpleShapePhantomCinfo;
    if (uiCount == 1) {
        CSpeedTreeRT::ECollisionObjectType eType;
        float afPosition[3];
        float afDimensions[3];
        apST->GetCollisionObject(0, eType, afPosition, afDimensions);
        afPosition[0] = NI2HK(afPosition[0]) * fScale;
        afPosition[1] = NI2HK(afPosition[1]) * fScale;
        afPosition[2] = NI2HK(afPosition[2]) * fScale;
        afDimensions[0] = NI2HK(afDimensions[0]) * fScale;
        afDimensions[1] = NI2HK(afDimensions[1]) * fScale;
        afDimensions[2] = NI2HK(afDimensions[2]) * fScale;
        if (eType == CSpeedTreeRT::CO_SPHERE) {
            alignas(16) uint8_t aTransformInfoStorage[sizeof(hkTransformShapeCinfo)];
            hkTransformShapeCinfo* pTransformInfo = ::new (aTransformInfoStorage) hkTransformShapeCinfo;
            alignas(16) hkVector4 kTranslation(afPosition[0], afPosition[1], afPosition[2], 0.0f);
            pTransformInfo->m_transform.Trans = kTranslation;
            void* pMemory = NiMemObject::operator new(sizeof(bhkSphereShape));
            bhkSphereShape* pSphere = pMemory ? ::new (pMemory) bhkSphereShape(afDimensions[0], false) : nullptr;
            pTransformInfo->m_childShape = pSphere->operator*();
            pMemory = NiMemObject::operator new(sizeof(bhkTransformShape));
            bhkShape* pTransformShape = pMemory ? ::new (pMemory) bhkTransformShape(*pTransformInfo) : nullptr;
            pPhantomInfo->m_shape = pTransformShape->operator*();
        }
        else if (eType == CSpeedTreeRT::CO_CYLINDER) {
            alignas(16) uint8_t aCapsuleInfoStorage[sizeof(hkCapsuleShapeCinfo)];
            hkCapsuleShapeCinfo* pCapsuleInfo = ::new (aCapsuleInfoStorage) hkCapsuleShapeCinfo;
            float fTop = afPosition[2] + afDimensions[0];
            pCapsuleInfo->m_vertexA.set(afPosition[0], afPosition[1], fTop, 0.0f);
            float fBottom = static_cast<float>(
                static_cast<double>(afPosition[2]) + afDimensions[1] - static_cast<double>(afDimensions[0])
            );
            pCapsuleInfo->m_vertexB.set(afPosition[0], afPosition[1], fBottom, 0.0f);
            pCapsuleInfo->m_radius = afDimensions[0];
            void* pMemory = NiMemObject::operator new(sizeof(bhkCapsuleShape));
            bhkShape* pCapsule = pMemory ? ::new (pMemory) bhkCapsuleShape(*pCapsuleInfo) : nullptr;
            pPhantomInfo->m_shape = pCapsule->operator*();
        }
        else if (eType == CSpeedTreeRT::CO_BOX) {
            alignas(16) uint8_t aTransformInfoStorage[sizeof(hkTransformShapeCinfo)];
            hkTransformShapeCinfo* pTransformInfo = ::new (aTransformInfoStorage) hkTransformShapeCinfo;
            for (int32_t i = 0; i < 3; ++i) {
                afDimensions[i] = afDimensions[i] * 0.5;
            }
            float fCenterZ = afPosition[2] + afDimensions[2];
            alignas(16) hkVector4 kTranslation(afPosition[0], afPosition[1], fCenterZ, 0.0f);
            alignas(16) hkVector4 kHalfExtents(afDimensions[0], afDimensions[1], afDimensions[2], 0.0f);
            pTransformInfo->m_transform.Trans = kTranslation;
            void* pMemory = NiMemObject::operator new(sizeof(bhkBoxShape));
            bhkShape* pBox = pMemory ? ::new (pMemory) bhkBoxShape(kHalfExtents) : nullptr;
            pTransformInfo->m_childShape = pBox->operator*();
            pMemory = NiMemObject::operator new(sizeof(bhkTransformShape));
            bhkShape* pTransformShape = pMemory ? ::new (pMemory) bhkTransformShape(*pTransformInfo) : nullptr;
            pPhantomInfo->m_shape = pTransformShape->operator*();
        }
    }
    else {
        alignas(16) uint8_t aMultiInfoStorage[sizeof(hkMultiSphereShapeCinfo)];
        hkMultiSphereShapeCinfo* pMultiInfo = ::new (aMultiInfoStorage) hkMultiSphereShapeCinfo;
        for (uint32_t i = 0; i < uiCount; ++i) {
            CSpeedTreeRT::ECollisionObjectType eType;
            float afPosition[3];
            float afDimensions[3];
            apST->GetCollisionObject(i, eType, afPosition, afDimensions);
            if (eType != CSpeedTreeRT::CO_SPHERE) {
                Warning("HAVOK: Multiple bounding volumes around a tree must all be spheres.");
                continue;
            }
            afPosition[0] = NI2HK(afPosition[0]) * fScale;
            afPosition[1] = NI2HK(afPosition[1]) * fScale;
            afPosition[2] = NI2HK(afPosition[2]) * fScale;
            afDimensions[0] = NI2HK(afDimensions[0]) * fScale;
            alignas(16) hkVector4 kSphere(afPosition[0], afPosition[1], afPosition[2], afDimensions[0]);
            pMultiInfo->m_spheres.pushBack(kSphere);
        }
        void* pMemory = NiMemObject::operator new(sizeof(bhkMultiSphereShape));
        bhkShape* pMulti = pMemory ? ::new (pMemory) bhkMultiSphereShape(*pMultiInfo) : nullptr;
        pPhantomInfo->m_shape = pMulti->operator*();
        pMultiInfo->~hkMultiSphereShapeCinfo();
    }
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(L_TREES, 4, P_OTHER);
    pPhantomInfo->m_collisionFilterInfo = kFilter;
    void* pMemory = NiMemObject::operator new(sizeof(bhkSimpleShapePhantom));
    bhkSimpleShapePhantom* pPhantom = pMemory ? ::new (pMemory) bhkSimpleShapePhantom(*pPhantomInfo) : nullptr;
    pMemory = NiMemObject::operator new(sizeof(bhkSPCollisionObject));
    bhkSPCollisionObject* pCollisionObject = pMemory ? ::new (pMemory) bhkSPCollisionObject(pLeafNode) : nullptr;
    pCollisionObject->SetWorldObject(pPhantom);
    pPhantomInfo->~hkSimpleShapePhantomCinfo();
}

bhkSimpleShapePhantom* BSTreeNode::GetLeafObject() {
    bhkSimpleShapePhantom* pp = nullptr;
    NiNode* pln = GetLeafNode();
    if (pln) {
        NiPointer<bhkNiCollisionObject> pnico(static_cast<bhkNiCollisionObject*>(pln->GetCollisionObject()));
        if (pnico.m_pObject) {
            pp = static_cast<bhkSimpleShapePhantom*>(
                NiObject::DynamicCastStatic(&bhkSimpleShapePhantom::ms_RTTI, pnico.m_pObject->GetWorldObject())
            );
        }
    }
    return pp;
}

hkCapsuleShapeCinfo::hkCapsuleShapeCinfo() {
    m_vertexB.set(1.0, 0.0, 0.0, 0.0);
}

// This getter is const but GethkWorldRw is not; it is called on the const proxy.
void bhkCharacterProxy::GetLinearVelocity(NiPoint3& arVel) const {
    hkpCharacterProxy* pProxy = static_cast<hkpCharacterProxy*>(GethkObject());
    if (pProxy) {
        const_cast<bhkCharacterProxy*>(this)->bhkCharacterProxy::GethkWorldRw();
        HK2NI(arVel, pProxy->getLinearVelocity());
        const_cast<bhkCharacterProxy*>(this)->bhkCharacterProxy::GethkWorldRw();
    }
}

bhkCollisionObject::bhkCollisionObject() {}

bhkCollisionObject::~bhkCollisionObject() {}

// Zero: a plain collision object does not move.
NiPoint3 bhkNiCollisionObject::GetVelocity() {
    return NiPoint3_ZERO;
}

bhkPCollisionObject::~bhkPCollisionObject() {}

bhkSPCollisionObject::~bhkSPCollisionObject() {}

hkpTransformShape* bhkTransformShape::operator*() const {
    if (this) {
        return static_cast<hkpTransformShape*>(GethkObject());
    }

    return nullptr;
}

hkTransformShapeCinfo::hkTransformShapeCinfo() : m_childShape(nullptr) {
    m_transform.setIdentity();
}

const hkClass& hkContactPoint::staticClass() {
    return hkContactPointClass;
}
