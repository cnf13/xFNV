#include "BSHavok/bhkConvexSweepShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

NiRTTI bhkConvexSweepShape::ms_RTTI("bhkConvexSweepShape", &bhkConvexShape::ms_RTTI);

bhkConvexSweepShape::bhkConvexSweepShape() {}

bhkConvexSweepShape::~bhkConvexSweepShape() {
    bhkSerializable::Kill();
}

const NiRTTI* bhkConvexSweepShape::GetRTTI() const {
    return &bhkConvexSweepShape::ms_RTTI;
}

// Resolves the shape's link before the base load, then hands it to the stream's ChangeObject.
void bhkConvexSweepShape::LoadBinary(NiStream& arStream) {
    NiObject* pShape = arStream.ResolveLinkID();
    bhkConvexShape::LoadBinary(arStream);
    arStream.ChangeObject(pShape);
}

void bhkConvexSweepShape::LinkObject(NiStream& arStream) {
    bhkShape::LinkObject(arStream);
}

// The shape registers nothing of its own.
bool bhkConvexSweepShape::RegisterStreamables(NiStream&) {
    return true;
}

// Any two sweep shapes compare equal here.
bool bhkConvexSweepShape::IsEqual(NiObject*) {
    return true;
}

uint32_t bhkConvexSweepShape::QCinfoSize() const {
    return sizeof(hkConvexSweepShapeCinfo);
}

void bhkConvexSweepShape::Init(const hkSerializableCinfo& arInfo) {
    PostInit(arInfo);
}

// Builds the shape's creation info; arCreated tells whether it was allocated here.
hkSerializableCinfo& bhkConvexSweepShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        pInfo = new hkConvexSweepShapeCinfo;
    }
    return *pInfo;
}

// Builds an empty convex sweep shape, or null when the allocation fails.
NiObject* bhkConvexSweepShape::CreateObject() {
    return new bhkConvexSweepShape;
}

hkConvexSweepShapeCinfo::hkConvexSweepShapeCinfo()
  : m_childShape(nullptr), m_maxLinearVelocity(20.0f), m_linearVelocityClipDiff(5.0f) {}
