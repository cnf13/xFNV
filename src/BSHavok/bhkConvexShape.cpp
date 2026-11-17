#include "BSHavok/bhkConvexShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCylinderShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkTriangleShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <bit>

NiRTTI bhkConvexShape::ms_RTTI("bhkConvexShape", &bhkSphereRepShape::ms_RTTI);

bhkConvexShape::bhkConvexShape() {
	++bhkConvexShape::iCount;
}

const NiRTTI* bhkConvexShape::GetRTTI() const {
	return &bhkConvexShape::ms_RTTI;
}

// A big-endian file's radius is swapped in the creation info, in place, after the base's data is read; the exe
// passes its own argument slot as the CreateCinfo flag's buffer.
void bhkConvexShape::LoadBinary(NiStream& arStream) {
	bhkShape::LoadBinary(arStream);
	if (!arStream.m_bSourceIsLittleEndian) {
		bool bCreated;
		hkConvexShapeCinfo& rInfo = static_cast<hkConvexShapeCinfo&>(CreateCinfo(bCreated));
		rInfo.m_radius = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_radius)));
	}
}

// For a big-endian file the radius is swapped in the creation info around the base's save, then swapped back.
void bhkConvexShape::SaveBinary(NiStream& arStream) {
	bool bCreated;
	hkConvexShapeCinfo& rInfo = static_cast<hkConvexShapeCinfo&>(CreateCinfo(bCreated));
	if (!arStream.m_bSaveLittleEndian) {
		rInfo.m_radius = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_radius)));
	}
	bhkShape::SaveBinary(arStream);
	if (!arStream.m_bSaveLittleEndian) {
		rInfo.m_radius = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_radius)));
	}
}

// Appends the class's heading and the radius in game units (the Havok radius, 0.1f without a Havok shape, times the
// float at 0x11B15C0) after the sphere rep shape's lines. The exe keeps the radius and the product in its argument
// slot as floats.
// The exe's cleanup-only C++ EH frame is left out.
void bhkConvexShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	hkpConvexShape* phkObject;
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	bhkSphereRepShape::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(bhkConvexShape::ms_RTTI.GetName());
	apStrings->Add(pcString);
	float fRadius;
	if (this && (phkObject = static_cast<hkpConvexShape*>(this->phkObject)) != 0) {
		fRadius = phkObject->m_radius;
	} else {
		fRadius = 0.1f;
	}
	float fScaled = fRadius * bhkConvexShape_fHk2BSScaleSC;
	pcString = NiGetViewerString("Radius", fScaled);
	apStrings->Add(pcString);
	v3->eMemContext = eMemContext;
}

// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
bhkConvexShape::~bhkConvexShape() {
	bhkSerializable::Kill();
	--bhkConvexShape::iCount;
}

void bhkConvexShape::CopyCinfo(hkConvexShapeCinfo& arInfo) {
	hkReferencedObject* phkObject;
	bhkShape::CopyCinfo(arInfo);
	if (this && (phkObject = this->phkObject) != 0) {
		arInfo.m_radius = static_cast<hkpConvexShape*>(phkObject)->m_radius;
	} else {
		arInfo.m_radius = 0.1f;
	}
}

// The default constructor: the sphere rep cinfo's constructor, then the default radius (the exe loads the float
// 0.1f at 0x10C72BC).
hkConvexShapeCinfo::hkConvexShapeCinfo() : m_radius(0.1f) {}
