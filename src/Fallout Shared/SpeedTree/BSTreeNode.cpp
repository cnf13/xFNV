#include "Fallout Shared/SpeedTree/BSTreeNode.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkBoxShape.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvert.h"
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
#include "BSHavok/bhkTransformShape.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Settings/XGameSetting.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
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
SettingT<GameSettingCollection> iCollFreq("iCollFreq", 2);
SettingT<GameSettingCollection> iLowDamp("iLowDamp", 50);
SettingT<GameSettingCollection> iHighDamp("iHighDamp", 80);
SettingT<GameSettingCollection> iNumGroups("iUpdateGroups", 5);
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
    uint16_t slevels = spModel->GetNumBranchLODLevels();
    if (slevels) {
        pspBranches = new NiPointer<NiTriBasedGeom>[slevels];
    }
    slevels = spModel->GetNumLeafLODLevels();
    if (slevels) {
        pspLeaves = new NiPointer<NiTriBasedGeom>[slevels];
    }
    AddExtraData(BSTreeManager::GetHavokFlags());
    bhkRigidBody* prigid = apRigid ? apRigid : spModel->GetRigidBody();
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
    NiNode::GetViewerStrings(pkStrings);
    char* message = NiGetViewerString("BSTreeNode");
    pkStrings->Add(message);
    if (spModel.m_pObject) {
        message = static_cast<char*>(ourMalloc(32u));
        if (spModel->IsInstance()) {
            BSsprintf(message, 32, "Type = Instance");
        }
        else {
            BSsprintf(message, 32, "Type = Base");
        }
        pkStrings->Add(message);
        message = static_cast<char*>(ourMalloc(32u));
        int32_t iSeed = spModel->iSeed;
        BSsprintf(message, 32, "Seed = %d", iSeed);
        pkStrings->Add(message);
    }
    else {
        message = static_cast<char*>(ourMalloc(32u));
        BSsprintf(message, 32, "-No Model-");
        pkStrings->Add(message);
    }
}

void BSTreeNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    NiCamera* Camera;
    AutoMemContext apMemory(
        MEM_CONTEXT_TREEART,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeNode.cpp",
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
        MEM_CONTEXT_TREEART,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeNode.cpp",
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
    if (!spModel.m_pObject || !pspBranches || asLevel >= spModel->GetNumBranchLODLevels()) {
        return nullptr;
    }

    return pspBranches[asLevel].m_pObject;
}

NiTriBasedGeom* BSTreeNode::GetLeafLevel(uint16_t asLevel) {
    if (!spModel.m_pObject || !pspLeaves || asLevel >= spModel->GetNumLeafLODLevels()) {
        return nullptr;
    }

    return pspLeaves[asLevel].m_pObject;
}

NiTriBasedGeom* BSTreeNode::GetBillboard() {
    return spBillboard.m_pObject;
}

// Replaces the geometry of one level of detail under the branch node. Constructs an empty string it never uses and
// destroys it on every return.
bool BSTreeNode::AddBranchLevel(uint16_t asLevel, NiTriBasedGeom* apGeom) {
    BSStringT<char> kUnused;
    if (!spModel.m_pObject || !pspBranches || !apGeom) {
        return false;
    }
    if (asLevel >= spModel->GetNumBranchLODLevels()) {
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
// destroyed on every return.
bool BSTreeNode::AddLeafLevel(uint16_t asLevel, NiTriBasedGeom* apGeom) {
    BSStringT<char> kUnused;
    if (!spModel.m_pObject || !pspLeaves || !apGeom) {
        return false;
    }
    if (asLevel >= spModel->GetNumLeafLODLevels()) {
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
        spBillboard->SetAppCulled(true);
    }
    else {
        spBillboard->SetAppCulled(false);
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(spBillboard->GetProperty(NiProperty::ALPHA));
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

    NiProperty* pOld = spBillboard->GetProperty(NiProperty::TEXTURING);
    if (pOld) {
        if (pOld == apProp) {
            return false;
        }
        spBillboard->RemoveProperty(NiProperty::TEXTURING);
    }
    spBillboard->AttachProperty(apProp);
    return true;
}

void BSTreeNode::UpdateBSTreeNode(float afTime) {
    ImpactMixer::COLLISION_SOUND csrigidData;
    ImpactMixer::COLLISION_SOUND csdata;
    if (spModel.m_pObject) {
        this->spModel->Update(this);
    }
    if (this->CheckForCollision(afTime)) {
        bhkSimpleShapePhantom* pTree = this->GetLeafObject();
        if (pTree && pTree->GethkWorld()) {
            const char* pUserData = this->spModel->pSpeedTree->GetUserData();
            const char* pCollisionFreq = std::strstr(pUserData, "~CF");
            int32_t iCollFreq = pCollisionFreq ? pCollisionFreq[3] - 48 : ::iCollFreq.Int();
            double dCollFreq = static_cast<double>(iCollFreq);
            float fCollChance = fUpdateInterval.Float() * dCollFreq * 100.0;
            alignas(16) hkpAllCdBodyPairCollector collector;
            pTree->GetPenetrations(&collector);
            const hkArray<hkpRootCdBodyPair, hkContainerHeapAllocator>* collArray = &collector.m_hits;
            for (int32_t i = 0; i < collArray->m_size; ++i) {
                hkpPhantom* phkpPhantom = hkpGetPhantom((*collArray)[i].m_rootCollidableB);
                hkpRigidBody* pRigidBody = hkpGetRigidBody((*collArray)[i].m_rootCollidableB);
                if (phkpPhantom) {
                    MobileObject* pMobObj = static_cast<MobileObject*>(
                        bhkCharacterController::FindBSReference(1000u, phkpPhantom)
                    );
                    if (pMobObj) {
                        bhkCharacterController* pc = pMobObj->GetCharController();
                        if (pc) {
                            alignas(16) NiPoint3 dum;
                            hkpRootCdPoint point;
                            float fZ = RandomFloat0To1();
                            float fY = RandomFloat0To1();
                            float fX = RandomFloat0To1();
                            hkVector4 normal(fX, fY, fZ, 0.0f);
                            normal.fastNormalize3();
                            hkVector4 vLocation(
                                (*collArray)[i].m_rootCollidableB->m_motion.pState->m_transform.m_translation
                            );
                            point.m_contact.set(vLocation, normal, 0.0f);
                            point.m_rootCollidableA = pTree->GetCollidable();
                            point.m_rootCollidableB = (*collArray)[i].m_rootCollidableB;
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
                else if (pRigidBody) {
                    int32_t iRandom = RandomInt(0, 100);
                    if (fCollChance >= static_cast<double>(iRandom)) {
                        alignas(16) NiPoint3 dum;
                        hkVector4 vLV(pRigidBody->getLinearVelocity());
                        hkVector4 vAV(pRigidBody->getAngularVelocity());
                        if (vLV.lengthSquared3() > 1.0) {
                            double dLength = vLV.length3();
                            int32_t iDamp = RandomInt(iLowDamp.Int(), iHighDamp.Int());
                            float fSpeed = (1.0 - static_cast<double>(iDamp) / 100.0) * dLength;
                            float fNewX = RandomFloatNeg1To1();
                            float fNewY = RandomFloatNeg1To1();
                            float fNewZ = RandomFloatNeg1To1() - 0.25;
                            hkVector4 vNewVel(fNewX, fNewY, fNewZ, 0.0f);
                            vNewVel.fastNormalize3();
                            hkSimdReal kSpeed(fSpeed);
                            vNewVel.mul4(kSpeed);
                            pRigidBody->setLinearVelocity(vNewVel);
                        }
                        if (vAV.lengthSquared3() > 1.0) {
                            double dLength = vAV.length3();
                            int32_t iDamp = RandomInt(0, 20);
                            float fSpeed = (static_cast<double>(iDamp) / 100.0 + 90.0) * dLength;
                            float fNewX = RandomFloatNeg1To1();
                            float fNewY = RandomFloatNeg1To1();
                            float fNewZ = RandomFloatNeg1To1() - 0.25;
                            hkVector4 vNewVel(fNewX, fNewY, fNewZ, 0.0f);
                            vNewVel.fastNormalize3();
                            hkSimdReal kSpeed(fSpeed);
                            vNewVel.mul4(kSpeed);
                            pRigidBody->setAngularVelocity(vNewVel);
                        }
                        ::new (&csrigidData) NiBound;
                        bhkShape* pShape = bhkShape::Getbhk((*collArray)[i].m_rootCollidableB->m_shape);
                        csrigidData.cMaterialType1 = 4;
                        HavokMaterialType eMaterial = pShape ? pShape->GetMaterial() : BHK_MATERIAL_STONE;
                        csrigidData.cMaterialType2 = eMaterial;
                        csrigidData.fFriction = 1.0f;
                        float fLinearSpeed = vLV.length3();
                        csrigidData.fVelocity = HK2NI(fLinearSpeed);
                        // The source's sound ID is the sum of the two objects' (32-bit) addresses.
                        csrigidData.iID = static_cast<int32_t>(
                            reinterpret_cast<uintptr_t>(pRigidBody) + reinterpret_cast<uintptr_t>(pTree)
                        );
                        csrigidData.Location = HK2NI(dum, pRigidBody->getTransform().m_translation);
                        csrigidData.pBody1 = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pRigidBody));
                        csrigidData.pBody2 = nullptr;
                        ImpactMixer::PlayCollisionSound(csrigidData);
                    }
                }
            }
        }
    }
}

bool BSTreeNode::CheckForCollision(float afTime) {
    if (afTime == 0.0) {
        return true;
    }
    if (fNextCollisionCheck == 0.0) {
        double dNextCheck = afTime + fUpdateInterval.Float();
        float fInterval = fUpdateInterval.Float();
        fNextCollisionCheck = static_cast<float>(
            fInterval / static_cast<double>(iNumGroups.Int()) * BSTreeNode::ucNextGroup + dNextCheck
        );
        ++BSTreeNode::ucNextGroup;
        BSTreeNode::ucNextGroup = static_cast<uint8_t>(
            BSTreeNode::ucNextGroup % static_cast<int32_t>(iNumGroups.UInt())
        );
        return true;
    }

    if (fNextCollisionCheck < static_cast<double>(afTime)) {
        fNextCollisionCheck = afTime + fUpdateInterval.Float();
        return true;
    }
    return false;
}

// Gives the tree's leaf node a simple-shape phantom built from the tree's collision objects, converted to Havok units
// and scaled by fTreeSizeConversion times afScale: one sphere (placed by a transform shape), capsule or box (placed by
// a transform shape, its dimensions halved), or a multi-sphere shape of several spheres. The Havok infos are aligned to
// 16. The capsule's lower end is computed in double, then rounded to float.
void BSTreeNode::MakeHavokLeaves(CSpeedTreeRT* apST, float afScale) {
    if (!(bTreeTops == true)) {
        return;
    }
    uint32_t uiCount = apST->GetCollisionObjectCount();
    if (!uiCount) {
        return;
    }
    float fScale = BSTreeManager::GetSizeConversion() * afScale;
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
            bhkSphereShape* pSphere = new bhkSphereShape(afDimensions[0], false);
            pTransformInfo->m_childShape = **pSphere;
            bhkShape* pTransformShape = new bhkTransformShape(*pTransformInfo);
            pPhantomInfo->m_shape = **pTransformShape;
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
            bhkShape* pCapsule = new bhkCapsuleShape(*pCapsuleInfo);
            pPhantomInfo->m_shape = **pCapsule;
        }
        else if (eType == CSpeedTreeRT::CO_BOX) {
            alignas(16) uint8_t aTransformInfoStorage[sizeof(hkTransformShapeCinfo)];
            hkTransformShapeCinfo* pTransformInfo = ::new (aTransformInfoStorage) hkTransformShapeCinfo;
            for (float& fDimension : afDimensions) {
                fDimension = fDimension * 0.5;
            }
            float fCenterZ = afPosition[2] + afDimensions[2];
            alignas(16) hkVector4 kTranslation(afPosition[0], afPosition[1], fCenterZ, 0.0f);
            alignas(16) hkVector4 kHalfExtents(afDimensions[0], afDimensions[1], afDimensions[2], 0.0f);
            pTransformInfo->m_transform.Trans = kTranslation;
            bhkShape* pBox = new bhkBoxShape(kHalfExtents);
            pTransformInfo->m_childShape = **pBox;
            bhkShape* pTransformShape = new bhkTransformShape(*pTransformInfo);
            pPhantomInfo->m_shape = **pTransformShape;
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
        bhkShape* pMulti = new bhkMultiSphereShape(*pMultiInfo);
        pPhantomInfo->m_shape = **pMulti;
        pMultiInfo->~hkMultiSphereShapeCinfo();
    }
    CFilter kFilter(L_TREES, 4, P_OTHER);
    pPhantomInfo->m_collisionFilterInfo = kFilter;
    bhkSimpleShapePhantom* pPhantom = new bhkSimpleShapePhantom(*pPhantomInfo);
    bhkSPCollisionObject* pCollisionObject = new bhkSPCollisionObject(pLeafNode);
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
                NiObject::DynamicCastStatic(&bhkSimpleShapePhantom::ms_RTTI, pnico->GetWorldObject())
            );
        }
    }
    return pp;
}
