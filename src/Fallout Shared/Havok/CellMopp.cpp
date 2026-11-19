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
    NiPointer<bhkRigidBody>* v1;
    bhkRigidBody* a1;
    uint32_t i;
    bhkWorldM* v5 = GetParent();
    if (v5) {
        for (i = 0; i < ((uint32_t)((int32_t)(&spRigidBodyA)->m_usESize)); ++i) {
            v1 = &(&spRigidBodyA)->operator[](i);
            a1 = v1->m_pObject;
            if (a1) {
                v5->RemoveBody(a1);
            }
        }
    }
}

bhkWorldM* CellMopp::GetParent() {
    NiPointer<bhkRigidBody>* v1;
    NiPointer<bhkRigidBody>* v2;
    bhkRigidBody* v5;
    if (!(&spRigidBodyA)->m_usESize) {
        return nullptr;
    }
    v1 = &(&spRigidBodyA)->operator[](0);
    bool vCondition1;
    vCondition1 = !v1->operator->();
    if (vCondition1) {
        return nullptr;
    }
    v2 = &(&spRigidBodyA)->operator[](0);
    v5 = v2->operator->();
    const hkpWorld* v3 = v5->GethkWorld();
    return static_cast<bhkWorldM*>(bhkWorld::Getbhk(v3));
}

bool CellMopp::Make(bhkWorld* apWorld, const NiPoint3& arPosition, const NiQuaternion& arRotation) {
    bool bres = false;
    if ((apWorld) && (QCreated())) {
        NiQuaternion rot;
        rot.FromAngleAxisX(1.5707964f);
        float fy = 0.0f;
        uint32_t K = 0;
        for (uint32_t I = 0; I < 2; ++I) {
            float fx = -2048.0f;
            for (uint32_t J = 0; J < 2; ++J) {
                NiPointer<bhkRigidBody>* v6 = &spRigidBodyA.operator[](K++);
                bhkRigidBody* prb = v6->m_pObject;
                NiPoint3 pos = arPosition;
                pos.x = pos.x + fx;
                pos.y = pos.y + fy;
                prb->SetPosition(pos);
                prb->SetRotation(rot);
                fx = fx + 2048.0;
            }
            fy = fy + 2048.0;
        }
        if (!GetParent()) {
            for (uint32_t I_2 = 0; I_2 < spRigidBodyA.m_usESize; ++I_2) {
                bhkRigidBody* prb_2 = (&spRigidBodyA.operator[](I_2))->m_pObject;
                apWorld->Add(prb_2);
                uint32_t v4 = hkColor::rgbFromChars(220, 150, 40, 255);
                apWorld->SetDebugObjectColor(prb_2, v4);
            }
        }
    }

    return bres;
}

bool CellMopp::QCreated() {
    return (
        ((int32_t)(&spRigidBodyA)
             ->NiTArray<NiPointer<bhkRigidBody>, NiTNewInterface<NiPointer<bhkRigidBody>>>::GetEffectiveSize()) != 0
    );
}

void CellMopp::Kill() {
    Remove();
    spRigidBodyA.RemoveAll();
}

bool CellMopp::Create(NiTriStrips** appStripArray, uint32_t aiCount, float afOffset) {
    hkTriSampledHeightFieldBvTreeShapeCinfo ci;
    bool bres = false;
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
                    ci.~hkTriSampledHeightFieldBvTreeShapeCinfo();
                    ::new (&ci) hkTriSampledHeightFieldBvTreeShapeCinfo;
                    ci.m_zRes = 17;
                    ci.m_xRes = 17;
                    float a4 = NI2HK(128.0f);
                    float a3 = NI2HK(1.0f);
                    float x = NI2HK(128.0f);
                    ci.m_scale.set(x, a3, a4, 0.0f);
                    ci.pData = pstrips->GetVertices();
                    float fOffset = afOffset;
                    ci.fOffset = fOffset;
                    ci.CalcMinMax();
                    bhkTriSampledHeightFieldBvTreeShape* pShape;
                    bhkTriSampledHeightFieldBvTreeShape* pShapeMemory =
                        static_cast<bhkTriSampledHeightFieldBvTreeShape*>(
                            NiMemObject::operator new(sizeof(bhkTriSampledHeightFieldBvTreeShape))
                        );
                    if (pShapeMemory) {
                        pShape = ::new (pShapeMemory) bhkTriSampledHeightFieldBvTreeShape(ci);
                    }
                    else {
                        pShape = nullptr;
                    }
                    alignas(16) NiPointer<bhkTriSampledHeightFieldBvTreeShape> spshape(pShape);
                    rbinfo.SetShape(spshape.m_pObject->Gethk());
                    bhkRigidBody* prb;
                    bhkRigidBody* pBodyMemory =
                        static_cast<bhkRigidBody*>(NiMemObject::operator new(sizeof(bhkRigidBody)));
                    if (pBodyMemory) {
                        prb = ::new (pBodyMemory) bhkRigidBody(rbinfo);
                    }
                    else {
                        prb = nullptr;
                    }
                    // The property key is the RTTI object's address, as SetAVObject's own default key.
                    prb->SetAVObject(pstrips, bhkPropertyKey(NiTriStrips::ms_RTTI));
                    NiPointer<bhkRigidBody> sprb(prb);
                    spRigidBodyA.Add(sprb);
                }
            }
        }
        bres = true;
    }
    return bres;
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
    float fHalfAngle = (float)(fAngle * 0.5);
    NiSinCos(fHalfAngle, m_fX, m_fW);
    m_fZ = (float)0.0;
    m_fY = (float)0.0;
}

hkpSampledHeightFieldBaseCinfo::hkpSampledHeightFieldBaseCinfo()
  : m_xRes(2), m_zRes(2), m_minHeight(0.0f), m_maxHeight(-1.0f), m_useProjectionBasedHeight(false) {
    m_scale.set(1.0f, 1.0f, 1.0f, 0.0f);
}
