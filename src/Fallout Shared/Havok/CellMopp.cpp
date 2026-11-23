#include "Fallout Shared/Havok/CellMopp.h"

#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkTriSampledHeightFieldBvTreeShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSHavok/hkBSHeightFieldShape.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

#include <cstdint>
#include <xmmintrin.h>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fLandFriction("fLandFriction:Landscape", 2.5f);

CellMopp::CellMopp() : spRigidBodyA(0, 1) {
    ++iCount;
}

CellMopp::~CellMopp() {
    --iCount;
    Kill();
}

void CellMopp::Remove() {
    bhkWorldM* pWorld = GetParent();
    if (pWorld) {
        for (uint32_t i = 0; i < spRigidBodyA.m_usESize; ++i) {
            bhkRigidBody* prb = spRigidBodyA[i].m_pObject;
            if (prb) {
                pWorld->RemoveBody(prb);
            }
        }
    }
}

bhkWorldM* CellMopp::GetParent() {
    if (!spRigidBodyA.m_usESize) {
        return nullptr;
    }
    if (!spRigidBodyA[0]) {
        return nullptr;
    }
    bhkRigidBody* prb = spRigidBodyA[0];
    const hkpWorld* phkWorld = prb->GethkWorld();
    return static_cast<bhkWorldM*>(bhkWorld::Getbhk(phkWorld));
}

bool CellMopp::Make(bhkWorld* apWorld, const NiPoint3& arPosition, const NiQuaternion& arRotation) {
    bool bRes = false;
    if (apWorld && QCreated()) {
        NiQuaternion rot;
        rot.FromAngleAxisX(1.5707964f);
        float fY = 0.0f;
        uint32_t K = 0;
        for (uint32_t I = 0; I < 2; ++I) {
            float fX = -2048.0f;
            for (uint32_t J = 0; J < 2; ++J) {
                bhkRigidBody* prb = spRigidBodyA[K++].m_pObject;
                NiPoint3 pos = arPosition;
                pos.x = pos.x + fX;
                pos.y = pos.y + fY;
                prb->SetPosition(pos);
                prb->SetRotation(rot);
                fX = fX + 2048.0;
            }
            fY = fY + 2048.0;
        }
        if (!GetParent()) {
            for (uint32_t I = 0; I < spRigidBodyA.m_usESize; ++I) {
                bhkRigidBody* prb = spRigidBodyA[I].m_pObject;
                apWorld->Add(prb);
                uint32_t uiColor = hkColor::rgbFromChars(220, 150, 40, 255);
                apWorld->SetDebugObjectColor(prb, uiColor);
            }
        }
    }

    return bRes;
}

bool CellMopp::QCreated() {
    return (&spRigidBodyA)
               ->NiTArray<NiPointer<bhkRigidBody>, NiTNewInterface<NiPointer<bhkRigidBody>>>::GetEffectiveSize() != 0;
}

void CellMopp::Kill() {
    Remove();
    spRigidBodyA.RemoveAll();
}

bool CellMopp::Create(NiTriStrips** appStripArray, uint32_t aiCount, float afOffset) {
    bool bRes = false;
    if (!QCreated()) {
        if (appStripArray) {
            bhkRigidBodyCinfo rbinfo;
            rbinfo.SetMass(0.0f);
            rbinfo.SetMotionType(hkpMotion::MOTION_FIXED);
            rbinfo.SetFriction(fLandFriction.Float());
            CFilter icf(L_GROUND, 1u, P_OTHER);
            rbinfo.SetCollisionFilter(icf.iFilter);
            for (uint32_t i = 0; i < aiCount; ++i) {
                NiTriStrips* pstrips = appStripArray[i];
                if (pstrips) {
                    hkTriSampledHeightFieldBvTreeShapeCinfo ci;
                    ci.m_zRes = 17;
                    ci.m_xRes = 17;
                    float fScaleZ = NI2HK(128.0f);
                    float fScaleY = NI2HK(1.0f);
                    float fScaleX = NI2HK(128.0f);
                    ci.m_scale.set(fScaleX, fScaleY, fScaleZ, 0.0f);
                    ci.pData = pstrips->GetVertices();
                    ci.fOffset = afOffset;
                    ci.CalcMinMax();
                    bhkTriSampledHeightFieldBvTreeShape* pShape = new bhkTriSampledHeightFieldBvTreeShape(ci);
                    alignas(16) NiPointer<bhkTriSampledHeightFieldBvTreeShape> spShape(pShape);
                    rbinfo.SetShape(spShape->Gethk());
                    bhkRigidBody* prb = new bhkRigidBody(rbinfo);
                    // The property key is the RTTI object's address, as SetAVObject's own default key.
                    prb->SetAVObject(pstrips, bhkPropertyKey(NiTriStrips::ms_RTTI));
                    NiPointer<bhkRigidBody> sprb(prb);
                    spRigidBodyA.Add(sprb);
                }
            }
        }
        bRes = true;
    }
    return bRes;
}

hkTriSampledHeightFieldBvTreeShapeCinfo::hkTriSampledHeightFieldBvTreeShapeCinfo() {}

bhkBvTreeShape::bhkBvTreeShape() {
    ++iCount;
}

void bhkRigidBodyCinfo::SetShape(const hkpShape* apShape) {
    m_shape = apShape;
    hk.m_shape = apShape;
}

hkBSHeightFieldShapeCinfo::hkBSHeightFieldShapeCinfo() : pData(nullptr), fOffset(0.0f) {}

void NiQuaternion::FromAngleAxisX(float fAngle) {
    float fHalfAngle = static_cast<float>(fAngle * 0.5);
    NiSinCos(fHalfAngle, m_fX, m_fW);
    m_fZ = 0.0f;
    m_fY = 0.0f;
}

hkpSampledHeightFieldBaseCinfo::hkpSampledHeightFieldBaseCinfo()
  : m_xRes(2), m_zRes(2), m_minHeight(0.0f), m_maxHeight(-1.0f), m_useProjectionBasedHeight(false) {
    m_scale.set(1.0f, 1.0f, 1.0f, 0.0f);
}

void* hkpSampledHeightFieldBaseCinfo::operator new(size_t aSize) {
    return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
}

void hkpSampledHeightFieldBaseCinfo::operator delete(void* apMemory) {
    hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, sizeof(hkpSampledHeightFieldBaseCinfo));
}
