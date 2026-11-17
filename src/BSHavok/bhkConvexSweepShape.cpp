#include "BSHavok/bhkConvexSweepShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
	int32_t& iContext = GetThreadMemContext();
	int32_t iSavedContext = iContext;
	iContext = MEM_CONTEXT_HAVOK;
	arCreated = (pInfo == nullptr);
	if (arCreated) {
		hkConvexSweepShapeCinfo* pCinfo =
			static_cast<hkConvexSweepShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkConvexSweepShapeCinfo)));
		if (pCinfo) {
			pCinfo->m_userData = 0;
			pCinfo->m_radius = 0.1f;
			pCinfo->m_childShape = nullptr;
			pCinfo->m_maxLinearVelocity = 20.0f;
			pCinfo->m_linearVelocityClipDiff = 5.0f;
		}
		pInfo = pCinfo;
	}
	hkSerializableCinfo* pResult = pInfo;
	iContext = iSavedContext;
	return *pResult;
}

// Builds an empty convex sweep shape, or null when the allocation fails.
NiObject* bhkConvexSweepShape::CreateObject() {
	bhkConvexSweepShape* pkShape =
		static_cast<bhkConvexSweepShape*>(NiMemObject::operator new(sizeof(bhkConvexSweepShape)));
	if (!pkShape) {
		return nullptr;
	}
	return ::new (pkShape) bhkConvexSweepShape;
}
