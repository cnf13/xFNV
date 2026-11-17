#include "BSHavok/bhkPackedNiTriStripsShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/hkPackedNiTriStripsShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkPackedNiTriStripsShape::ms_RTTI("bhkPackedNiTriStripsShape", &bhkShapeCollection::ms_RTTI);

bhkPackedNiTriStripsShape::bhkPackedNiTriStripsShape() {
	++iCount;
}

bhkPackedNiTriStripsShape::~bhkPackedNiTriStripsShape() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkPackedNiTriStripsShape::GetRTTI() const {
	return &bhkPackedNiTriStripsShape::ms_RTTI;
}

// The exe's cleanup-only EH frame is left out; the object it unwinds is the clone's.
NiObject* bhkPackedNiTriStripsShape::CreateClone(NiCloningProcess& arCloneData) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	bhkSerializable::CloneCritLock();
	bhkPackedNiTriStripsShape* pClone =
		static_cast<bhkPackedNiTriStripsShape*>(NiMemObject::operator new(sizeof(bhkPackedNiTriStripsShape)));
	if (pClone) {
		::new (pClone) bhkPackedNiTriStripsShape;
	}
	// Through the base: the exe reaches the class's CopyMembers through the vtable (slot 49).
	static_cast<bhkShape*>(this)->CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	GetThreadMemContext() = iOldMemContext;
	return pClone;
}

// The cinfo through its slot, the shape's base data, then the radius and the scale read into the cinfo, then the
// link. The two reads are the exe's inlined NiBinaryStream::BinaryRead (the read callback and the position
// update), each with its own component size.
void bhkPackedNiTriStripsShape::LoadBinary(NiStream& arStream) {
	// The exe passes an uninitialised byte, which the builder writes and nothing reads.
	bool bCreated;
	// Slot 45 (+0xB4), through the vtable.
	hkSerializableCinfo* pRawCinfo = &static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated);
	bhkShape::LoadBinary(arStream);
	hkPackedNiTriStripsShapeCinfo* pCinfo = static_cast<hkPackedNiTriStripsShapeCinfo*>(pRawCinfo);
	uint32_t uiRadiusComponentSize = 4;
	arStream.m_pkIstr->BinaryRead(&pCinfo->fRadius, 4, &uiRadiusComponentSize, 1);
	uint32_t uiScaleComponentSize = 4;
	arStream.m_pkIstr->BinaryRead(&pCinfo->vScale, 0x10, &uiScaleComponentSize, 1);
	arStream.ReadLinkID();
}

// Links the data object the stream named into the creation info, then the base's links. The exe loads the cinfo's
// data pointer once: it releases the old object, stores the new one and takes a reference on it.
void bhkPackedNiTriStripsShape::LinkObject(NiStream& arStream) {
	bool bCreated;
	hkPackedNiTriStripsShapeCinfo& rCinfo = static_cast<hkPackedNiTriStripsShapeCinfo&>(CreateCinfo(bCreated));
	hkPackedNiTriStripsData* pData = static_cast<hkPackedNiTriStripsData*>(arStream.GetObjectFromLinkID());
	rCinfo.spData = pData;
	bhkShapeCollection::LinkObject(arStream);
}

// Registers the wrapped shape's strips data (through its vtable), then the base's streamables.
bool bhkPackedNiTriStripsShape::RegisterStreamables(NiStream& arStream) {
	hkReferencedObject* phkObject;
	if (this) {
		phkObject = this->phkObject;
	} else {
		phkObject = 0;
	}
	hkPackedNiTriStripsData* pData = static_cast<hkPackedNiTriStripsShape*>(phkObject)->spTriStripsData.m_pObject;
	pData->RegisterStreamables(arStream);
	return bhkShape::RegisterStreamables(arStream);
}

// checkbody DIFF at the BinaryWrite calls: the exe reuses its own arStream argument slot as BinaryWrite's size
// variable; src passes a local holding the same 4 and 0x10. At the KillCinfo call the exe pushes the whole dword of
// the bool slot CreateCinfo fills one byte of; the checker's stub of CreateCinfo writes nothing, so the upper bytes
// are frame garbage on both sides.
void bhkPackedNiTriStripsShape::SaveBinary(NiStream& arStream) {
	// The exe makes the creation info, streams the shape, then drops the info again.
	bool bCreated;
	static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated);
	hkPackedNiTriStripsShape* pShape = static_cast<hkPackedNiTriStripsShape*>(phkObject);
	bhkShape::SaveBinary(arStream);
	NiBinaryStream* pOstr = arStream.m_pkOstr;
	float fRadius = pShape->fRadius;
	uint32_t uiSize = 4;
	pOstr->BinaryWrite(&fRadius, 4, &uiSize, 1);
	uiSize = 0x10;
	pOstr->BinaryWrite(&pShape->vScale, 0x10, &uiSize, 1);
	arStream.SaveLinkID(pShape->spTriStripsData.m_pObject);
	static_cast<bhkSerializable*>(this)->KillCinfo(bCreated);
}

// The base's comparison, then whether both wrapped shapes hold the same mesh.
bool bhkPackedNiTriStripsShape::IsEqual(NiObject* apObject) {
	bool bEqual = bhkShape::IsEqual(apObject);
	if (!bEqual) {
		return bEqual;
	}
	hkPackedNiTriStripsShape* phkOther = apObject
		? static_cast<hkPackedNiTriStripsShape*>(static_cast<bhkPackedNiTriStripsShape*>(apObject)->phkObject)
		: nullptr;
	hkPackedNiTriStripsShape* phkShape = this ? static_cast<hkPackedNiTriStripsShape*>(phkObject) : nullptr;
	return phkShape->spTriStripsData.m_pObject == phkOther->spTriStripsData.m_pObject;
}

// The base's strings, this class's RTTI name and the mesh's sub part count, then for each of the first five sub parts
// its index, its collision filter's strings and its material. The appends' allocations are charged to the Havok heap
// context. The exe's cleanup-only C++ EH frame is left out.
void bhkPackedNiTriStripsShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	bhkShapeCollection::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(bhkPackedNiTriStripsShape::ms_RTTI.GetName());
	// EXE-INLINED none: NiTArray<char*>::Add
	uint32_t uiIndex = apStrings->m_usSize;
	if (uiIndex >= apStrings->m_usMaxSize) {
		apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(uiIndex, pcString);
	hkPackedNiTriStripsShape* phkShape = this ? static_cast<hkPackedNiTriStripsShape*>(phkObject) : nullptr;
	uint32_t uiParts = phkShape->spTriStripsData.m_pObject->usPartCount;
	pcString = NiGetViewerString("Subparts", uiParts);
	apStrings->Add(pcString);
	if (uiParts > 5) {
		uiParts = 5;
	}
	for (uint32_t i = 0; i < uiParts; ++i) {
		pcString = NiGetViewerString("Part", i);
		// EXE-INLINED none: NiTArray<char*>::Add
		uiIndex = apStrings->m_usSize;
		if (uiIndex >= apStrings->m_usMaxSize) {
			apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
		}
		// EXE-INLINED 0x96AE90
		if (uiIndex < apStrings->m_usSize) {
			if (pcString) {
				if (!apStrings->m_pBase[uiIndex]) {
					++apStrings->m_usESize;
				}
			} else if (apStrings->m_pBase[uiIndex]) {
				--apStrings->m_usESize;
			}
		} else {
			apStrings->m_usSize = static_cast<uint16_t>(uiIndex + 1);
			if (pcString) {
				++apStrings->m_usESize;
			}
		}
		apStrings->m_pBase[uiIndex] = pcString;
		CFilter kFilter;
		kFilter.iFilter = phkShape->spTriStripsData.m_pObject->paSubPartData[i].uiFilter;
		kFilter.GetViewerStrings(apStrings);
		pcString = NiGetViewerString("MATERIAL",
			static_cast<int32_t>(phkShape->spTriStripsData.m_pObject->paSubPartData[i].uiMaterial));
		// EXE-INLINED none: NiTArray<char*>::Add
		uiIndex = apStrings->m_usSize;
		if (uiIndex >= apStrings->m_usMaxSize) {
			apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
		}
		// EXE-INLINED 0x96AE90
		if (uiIndex < apStrings->m_usSize) {
			if (pcString) {
				if (!apStrings->m_pBase[uiIndex]) {
					++apStrings->m_usESize;
				}
			} else if (apStrings->m_pBase[uiIndex]) {
				--apStrings->m_usESize;
			}
		} else {
			apStrings->m_usSize = static_cast<uint16_t>(uiIndex + 1);
			if (pcString) {
				++apStrings->m_usESize;
			}
		}
		apStrings->m_pBase[uiIndex] = pcString;
	}
	pTLSData->eMemContext = eMemContext;
}

// The PDB's bhkPackedNiTriStripsShape::KillCinfo(abCreated); bhkExtendedMeshShape's slot 41 holds the same address
// (one body). The exe's cleanup-only EH frame is left out.
void bhkPackedNiTriStripsShape::KillCinfo(bool abCreated) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	if (abCreated) {
		hkSerializableCinfo* pInfo = this->pInfo;
		if (pInfo) {
			hkPackedNiTriStripsShapeCinfo* pCinfo = static_cast<hkPackedNiTriStripsShapeCinfo*>(pInfo);
			pCinfo->~hkPackedNiTriStripsShapeCinfo();
			// CreateCinfo aligned the block to 16 bytes; the byte before it holds the padding it was shifted by.
			// The allocation is raw memory: its padding byte is not part of the class.
			uint8_t* puiBlock = static_cast<uint8_t*>(static_cast<void*>(pCinfo));
			uint8_t uiPadding = puiBlock[-1];
			MemoryManager_Singleton.Deallocate(puiBlock - uiPadding);
		}
		this->pInfo = nullptr;
	}
	GetThreadMemContext() = iOldMemContext;
}

uint32_t bhkPackedNiTriStripsShape::QCinfoSize() const {
	return 32;
}

int32_t bhkPackedNiTriStripsShape::LoadCInfo(NiStream& arStream) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	// The exe reads the creation info into a scratch block and drops it: nothing is kept.
	uint32_t uiCinfoSize = static_cast<const bhkSerializable*>(this)->QCinfoSize();
	void* pCinfo = MemoryManager_Singleton.Allocate(uiCinfoSize);
	NiBinaryStream* pIstr = arStream.m_pkIstr;
	pIstr->BinaryRead(pCinfo, uiCinfoSize, nullptr, 0);
	MemoryManager_Singleton.Deallocate(pCinfo);
	GetThreadMemContext() = iOldMemContext;
	return static_cast<int32_t>(uiCinfoSize);
}

void bhkPackedNiTriStripsShape::Init(const hkSerializableCinfo& arInfo) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	hkPackedNiTriStripsShape* pShape = static_cast<hkPackedNiTriStripsShape*>(kRouter.heap()
			.blockAlloc(static_cast<int32_t>(sizeof(hkPackedNiTriStripsShape))));
	// The shape's constructor records the size it was allocated with.
	pShape->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkPackedNiTriStripsShape));
	hkReferencedObject* pObject =
		::new (pShape) hkPackedNiTriStripsShape(static_cast<const hkPackedNiTriStripsShapeCinfo&>(arInfo));
	// Both are called through the vtable, as the exe does (slots 35 and 47).
	static_cast<bhkSerializable*>(this)->SethkObject(pObject);
	pObject->removeReference();
	static_cast<bhkSerializable*>(this)->PostInit(arInfo);
	GetThreadMemContext() = iOldMemContext;
}

// The exe's cleanup-only EH frame is left out; it unwinds the creation info's block.
hkSerializableCinfo& bhkPackedNiTriStripsShape::CreateCinfo(bool& arCreated) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	arCreated = this->pInfo == nullptr;
	if (arCreated) {
		char* pMemory =
			static_cast<char*>(MemoryManager_Singleton.Allocate(sizeof(hkPackedNiTriStripsShapeCinfo) + 0x10));
		// The block is aligned to 16 bytes; the byte before the result holds the padding it was shifted by. The
		// address is raw memory here, so the exe's integer arithmetic on it is written the same way.
		uint8_t uiPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
		char* pAligned = pMemory + uiPadding;
		pAligned[-1] = static_cast<char>(uiPadding);
		hkPackedNiTriStripsShapeCinfo* pCinfo = ::new (pAligned) hkPackedNiTriStripsShapeCinfo;
		this->pInfo = pCinfo;
		// A shape that already exists is copied into the new creation info.
		if (this->phkObject) {
			CopyCinfo(*pCinfo);
		}
	}
	hkSerializableCinfo* pResult = this->pInfo;
	GetThreadMemContext() = iOldMemContext;
	return *pResult;
}

// The creation info's scale (all four components) multiplied by the cloning process's scale, then the base's members.
// The exe calls CreateCinfo through the vtable (+0xB4) and does not kill the info it may make; it copies the scale
// with fld/fstp before broadcasting it.
void bhkPackedNiTriStripsShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
	bool bCreated;
	hkPackedNiTriStripsShapeCinfo& rCinfo = static_cast<hkPackedNiTriStripsShapeCinfo&>(CreateCinfo(bCreated));
	// The exe's clone process is always a BSCloningProcess; it reads Scale.x.
	float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
	rCinfo.vScale.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0), rCinfo.vScale.m_quad);
	bhkShape::CopyMembers(apDest, arCloneData);
}

// The exe jumps to bhkShape's, past bhkShapeCollection's own.
bool bhkPackedNiTriStripsShape::CanShare(NiCloningProcess& arCloneData) {
	return bhkShape::CanShare(arCloneData);
}

// Adds to apNode one NiTriShape per sub part of the packed mesh, named for the class, scaled by the shape's |scale|,
// with a material property whose emittance is arDefColor, under the Havok heap context. The sub part's triangles
// are gathered into shapeArray as index triples relative to the part's first vertex, following a strip while each
// next triangle shares the last two indices. The exe's cleanup-only C++ EH frame is left out.
void bhkPackedNiTriStripsShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	hkPackedNiTriStripsShape* pShape = this ? static_cast<hkPackedNiTriStripsShape*>(this->phkObject) : nullptr;
	hkPackedNiTriStripsData* pdata = pShape->spTriStripsData.m_pObject;
	// The exe inlines the constructor (no out-of-line copy): no slots, a grow-by of one.
	FNVD_INLINED_LOCAL(NiTPrimitiveArray<uint16_t>, shapeArray, std::nullopt, std::nullopt);
	::new (&shapeArray) NiTPrimitiveArray<uint16_t>(0u, 1u);
	uint16_t usvertCount = 0;
	uint16_t uslastKey = 0;
	uint16_t uscurrentKey = 0;
	for (int32_t i = 0; i < pdata->usPartCount; ++i) {
		uint16_t length = 0;
		uint16_t us0 = 0;
		uint16_t us1 = 0;
		uint16_t us2 = 0;
		// The exe inlines Add, SetAtGrow and SetAt (0xA5B870) at each append, as src's inline template does.
		do {
			TriangleData* ptri = &pdata->paTriData[uscurrentKey];
			if (!length) {
				us0 = ptri->us1;
				us1 = ptri->us2;
				us2 = ptri->us3;
				if (us0 >= usvertCount + pdata->paSubPartData[i].uiNumVerts) {
					break;
				}
				shapeArray.Add(static_cast<uint16_t>(us0 - usvertCount));
				shapeArray.Add(static_cast<uint16_t>(us1 - usvertCount));
				shapeArray.Add(static_cast<uint16_t>(us2 - usvertCount));
				++uscurrentKey;
				length = 1;
			} else if (us1 == ptri->us1 && us2 == ptri->us2) {
				// The triangle continues the strip: it shares the last two indices.
				us0 = us1;
				us1 = us2;
				us2 = ptri->us3;
				shapeArray.Add(static_cast<uint16_t>(us0 - usvertCount));
				shapeArray.Add(static_cast<uint16_t>(us1 - usvertCount));
				shapeArray.Add(static_cast<uint16_t>(us2 - usvertCount));
				++length;
				++uscurrentKey;
			} else {
				length = 0;
			}
		} while (us0 < usvertCount + pdata->paSubPartData[i].uiNumVerts && uscurrentKey < pdata->uiKeyCount);

		void* pMemory = NiMemObject::operator new(sizeof(NiTriShapeData));
		NiTriShapeData* ptsd = pMemory ? ::new (pMemory) NiTriShapeData : nullptr;
		uint16_t uspartVerts = static_cast<uint16_t>(pdata->paSubPartData[i].uiNumVerts);
		uint64_t ullSize = static_cast<uint64_t>(uspartVerts) * sizeof(NiPoint3);
		NiPoint3* verts =
			static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF
																				  : static_cast<uint32_t>(ullSize)));
		// The exe copies each point into an hkVector4 (w zero) and multiplies its lanes on the x87 stack by the
		// Havok-to-game scale, loaded once before the loop.
		float fToGame = bhkConvert::fHk2BSScaleSC;
		for (uint16_t v = usvertCount; v < usvertCount + uspartVerts; ++v) {
			const NiPoint3& kVert = pdata->paVertData[v];
			NiPoint3& kOut = verts[v - usvertCount];
			kOut.x = kVert.x * fToGame;
			kOut.y = kVert.y * fToGame;
			kOut.z = kVert.z * fToGame;
		}
		ptsd->NiGeometryData::SetData(uspartVerts, verts, nullptr, nullptr, nullptr, 0,
			NiGeometryData::NBT_METHOD_NONE);
		uint16_t usnumTris = static_cast<uint16_t>(uscurrentKey - uslastKey);
		ullSize = static_cast<uint64_t>(static_cast<uint32_t>(usnumTris) * 3) * sizeof(uint16_t);
		uint16_t* ptriList =
			static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF
																				  : static_cast<uint32_t>(ullSize)));
		rt::memcpy(ptriList, shapeArray.m_pBase, shapeArray.m_usSize * sizeof(uint16_t));
		ptsd->SetData(usnumTris, ptriList);

		pMemory = NiMemObject::operator new(sizeof(NiTriShape));
		NiTriShape* ptriShape = pMemory ? ::new (pMemory) NiTriShape(ptsd) : nullptr;
		alignas(16) NiFixedString kName;
		// EXE-INLINED none: NiFixedString::NiFixedString(const char*)
		kName.SetHandle(NiGlobalStringTable::AddString("bhkPackedNiTriStripsShape"));
		ptriShape->SetName(kName);
		// The exe's fabs is the inlined x87 instruction.
		float fScale = GetScale();
		fScale = __builtin_fabsf(fScale);
		ptriShape->m_kLocal.m_fScale = fScale;
		shapeArray.RemoveAll();

		pMemory = NiMemObject::operator new(sizeof(NiMaterialProperty));
		NiMaterialProperty* pmat = pMemory ? ::new (pMemory) NiMaterialProperty : nullptr;
		// EXE-INLINED 0x4BC450
		pmat->m_emit = arDefColor;
		++pmat->m_uiRevID;
		ptriShape->AttachProperty(pmat);
		// Through the vtable (slot 55), as the exe does.
		apNode->AttachChild(ptriShape, 1);
		usvertCount += uspartVerts;
		uslastKey = uscurrentKey;
	}
	// The exe inlines the destructor (0xA5C760) before the memory context is restored: the free of the buffer (the
	// vtable store is dead).
	FNVD_DESTRUCT(NiTArray<uint16_t, NiTMallocInterface<uint16_t>>)(&shapeArray);
	GetThreadMemContext() = iOldMemContext;
}

HavokMaterialType bhkPackedNiTriStripsShape::QSubMaterial(uint32_t aiKey) {
	hkPackedNiTriStripsShape* phkObject;
	hkPackedNiTriStripsData* m_pObject;
	if (this) {
		phkObject = static_cast<hkPackedNiTriStripsShape*>(this->phkObject);
	} else {
		phkObject = nullptr;
	}
	int32_t v3 = (int32_t)((uint8_t)(aiKey >> 24));
	if (aiKey == ((-1))) {
		return (HavokMaterialType)(((int32_t)eMaterial) & 31);
	}
	m_pObject = phkObject->spTriStripsData.m_pObject;
	if (v3 >= ((int32_t)m_pObject->usPartCount)) {
		return (HavokMaterialType)(((int32_t)eMaterial) & 31);
	} else {
		return (HavokMaterialType)((int32_t)m_pObject->paSubPartData[v3].uiMaterial);
	}
}

// The base's data, then the wrapped shape's mesh, radius and scale; without a wrapped shape, a zero radius and a unit
// scale.
void bhkPackedNiTriStripsShape::CopyCinfo(hkPackedNiTriStripsShapeCinfo& arInfo) {
	bhkShape::CopyCinfo(arInfo);
	hkReferencedObject* phkShape;
	if (this && (phkShape = phkObject) != nullptr) {
		static_cast<hkPackedNiTriStripsShape*>(phkShape)->CopyCinfo(arInfo);
	} else {
		arInfo.fRadius = 0.0f;
		arInfo.vScale.setAll(1.0f);
	}
}

// The x component of the wrapped shape's scale, 1 without a shape.
float bhkPackedNiTriStripsShape::GetScale() {
	hkReferencedObject* phkObject;
	if (this && (phkObject = this->phkObject) != 0) {
		return static_cast<hkPackedNiTriStripsShape*>(phkObject)->vScale(0);
	} else {
		return 1.0f;
	}
}

// The exe's cleanup-only EH frame is left out: it only unwinds the object the allocation made.
NiObject* bhkPackedNiTriStripsShape::CreateObject() {
	bhkPackedNiTriStripsShape* pObject =
		static_cast<bhkPackedNiTriStripsShape*>(NiMemObject::operator new(sizeof(bhkPackedNiTriStripsShape)));
	if (!pObject)
		return nullptr;
	return ::new (pObject) bhkPackedNiTriStripsShape;
}

// hkShapeCinfo's constructor clears the user data; spData's own destructor releases the data.
hkPackedNiTriStripsShapeCinfo::hkPackedNiTriStripsShapeCinfo() : spData(nullptr), fRadius(0.0f) {
	vScale.setAll(1.0f);
}

hkPackedNiTriStripsShapeCinfo::~hkPackedNiTriStripsShapeCinfo() {}
