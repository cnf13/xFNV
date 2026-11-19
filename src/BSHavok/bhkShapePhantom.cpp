#include "BSHavok/bhkShapePhantom.h"

#include "BSCore/TLSData.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// Held around a phantom shape's move.
static BSCriticalSection ShapeLock;

NiRTTI bhkShapePhantom::ms_RTTI("bhkShapePhantom", &bhkPhantom::ms_RTTI);

hkShapePhantomCinfo::hkShapePhantomCinfo() {
    m_transform.setIdentity();
}

// Adds the phantom's viewer lines to apStrings: the base phantom's lines, the class heading, then the transform's
// rotation and translation in game units.
void bhkShapePhantom::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    MEM_CONTEXT& iMemContext = GetThreadTLSData()->eMemContext;
    MEM_CONTEXT iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;

    bhkPhantom::GetViewerStrings(apStrings);

    char* pcName = NiGetViewerString(bhkShapePhantom::ms_RTTI.m_pcName);
    uint32_t uiSize = apStrings->m_usSize;
    uint32_t uiMaxSize = apStrings->m_usMaxSize;
    if (uiSize >= uiMaxSize) {
        apStrings->SetSize(uiSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiSize, pcName);

    hkTransform kTransform;
    NiMatrix3 kRot;
    alignas(16) NiPoint3 kTrans;
    GetTransform(kTransform);
    HK2NI(kRot, kTransform.m_rotation);
    HK2NI(kTrans, kTransform.m_translation);

    alignas(16) char* pcString = kRot.GetViewerString("Rot");
    uiSize = apStrings->m_usSize;
    uiMaxSize = apStrings->m_usMaxSize;
    if (uiSize >= uiMaxSize) {
        apStrings->SetSize(uiSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiSize, pcString);

    pcString = kTrans.GetViewerString("Trans");
    uiSize = apStrings->m_usSize;
    uiMaxSize = apStrings->m_usMaxSize;
    if (uiSize >= uiMaxSize) {
        apStrings->SetSize(uiSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiSize, pcString);

    iMemContext = iSavedMemContext;
}

// The phantom's world transform, or nothing without a Havok phantom. The world's read marks around the copy are empty
// in this build. Tests this for null, as the game does.
void bhkShapePhantom::GetTransform(hkTransform& arTrans) {
    if (this) {
        hkpShapePhantom* pPhantom = static_cast<hkpShapePhantom*>(this->phkObject);
        if (pPhantom) {
            MarkRead();
            arTrans = pPhantom->m_motionState.m_transform;
            UnMarkRead();
        }
    }
}

void bhkShapePhantom::GetTransform(NiTransform& arTrans) {
    hkTransform hkt;
    GetTransform(hkt);
    HK2NI(arTrans.m_Rotate, hkt.m_rotation);
    HK2NI(arTrans.m_Translate, hkt.m_translation);
}

// Moves the phantom to arPos, converted to Havok units.
void bhkShapePhantom::SetPosition(const NiPoint3& arPos) {
    hkVector4 kPosition;
    SetPosition(NI2HK(kPosition, arPos));
}

// Runs the wrapped phantom's linear cast under the shape's critical section and the world's write marks. Tests `this`,
// then reads phkObject.
void bhkShapePhantom::SetPositionAndLinearCast(
    const hkVector4& arPos,
    const hkpLinearCastInput& arIn,
    hkpCdPointCollector& arPnts,
    hkpCdPointCollector* apStart
) {
    if (!this) {
        return;
    }
    hkpShapePhantom* pPhantom = static_cast<hkpShapePhantom*>(this->phkObject);
    if (!pPhantom) {
        return;
    }
    EnterCriticalSection(&ShapeLock.CriticalSection);
    MarkWrite();
    pPhantom->setPositionAndLinearCast(arPos, arIn, arPnts, apStart);
    UnMarkWrite();
    LeaveCriticalSection(&ShapeLock.CriticalSection);
}

// Fills apInfo: the world object's part, then the wrapped phantom's transform, column by column through
// bhkVector4::operator=.
void bhkShapePhantom::CopyCinfo(hkShapePhantomCinfo& arInfo) {
    hkTransform kTransform;
    bhkWorldObject::CopyCinfo(arInfo);
    GetTransform(kTransform);
    arInfo.m_transform.Rot.Col0 = kTransform.m_rotation.m_col0;
    arInfo.m_transform.Rot.Col1 = kTransform.m_rotation.m_col1;
    arInfo.m_transform.Rot.Col2 = kTransform.m_rotation.m_col2;
    arInfo.m_transform.Trans = kTransform.m_translation;
}

void bhkShapePhantom::setTransform(hkTransform* a2) {
    hkpShapePhantom* v3 = static_cast<hkpShapePhantom*>(**this);
    if (v3) {
        v3->setTransform(*a2);
    }
}
