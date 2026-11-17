#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiRTTI NiScreenElementsData::ms_RTTI("NiScreenElementsData", &NiTriShapeData::ms_RTTI);

NiScreenElementsData::NiScreenElementsData()
	: m_akPolygon(nullptr), m_ausPIndexer(nullptr), m_usMaxPQuantity(0), m_usPGrowBy(0), m_usPQuantity(0),
	  m_usMaxVQuantity(0), m_usVGrowBy(0), m_usMaxIQuantity(0), m_usIGrowBy(0), m_bBoundNeedsUpdate(false) {}

// Each normal of the normal array is initialised to -UNIT_Z and each colour of the colour array is zeroed, and the
// texture set count is kept in the low bit of m_usDataFlags.
NiScreenElementsData::NiScreenElementsData(bool bWantNormals, bool bWantColors, uint16_t usNumTextureSets,
	int32_t iMaxPQuantity, int32_t iPGrowBy, int32_t iMaxVQuantity, int32_t iVGrowBy, int32_t iMaxTQuantity,
	int32_t iTGrowBy)
	: m_usMaxPQuantity(static_cast<uint16_t>(iMaxPQuantity > 0 ? iMaxPQuantity : 1)),
	  m_usPGrowBy(static_cast<uint16_t>(iPGrowBy > 0 ? iPGrowBy : 1)), m_usPQuantity(0),
	  m_usMaxVQuantity(static_cast<uint16_t>(iMaxVQuantity > 0 ? iMaxVQuantity : 1)),
	  m_usVGrowBy(static_cast<uint16_t>(iVGrowBy > 0 ? iVGrowBy : 1)),
	  m_usMaxIQuantity(static_cast<uint16_t>(iMaxTQuantity > 0 ? 3 * iMaxTQuantity : 3)),
	  m_usIGrowBy(static_cast<uint16_t>(iTGrowBy > 0 ? 3 * iTGrowBy : 3)), m_bBoundNeedsUpdate(false) {
	m_akPolygon = static_cast<Polygon*>(_NiMalloc(sizeof(Polygon) * m_usMaxPQuantity));
	m_ausPIndexer = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_usMaxPQuantity));
	std::memset(m_ausPIndexer, 0xFF, sizeof(uint16_t) * m_usMaxPQuantity);

	m_usVertices = 0;
	m_pkVertex = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(sizeof(NiPoint3) * m_usMaxVQuantity));
	if (bWantNormals) {
		m_pkNormal = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(sizeof(NiPoint3) * m_usMaxVQuantity));
		for (uint16_t i = 0; i < m_usMaxVQuantity; i++) {
			m_pkNormal[i] = -NiPoint3_UNIT_Z;
		}
	} else {
		m_pkNormal = nullptr;
	}

	NiColorA* pkColor = nullptr;
	if (bWantColors) {
		pkColor = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(sizeof(NiColorA) * m_usMaxVQuantity));
		if (pkColor) {
			for (uint32_t i = 0; i < m_usMaxVQuantity; i++) {
				pkColor[i].r = 0.0f;
				pkColor[i].g = 0.0f;
				pkColor[i].b = 0.0f;
				pkColor[i].a = 0.0f;
			}
		}
	}
	m_pkColor = pkColor;

	if (usNumTextureSets) {
		uint32_t uiNumTexCoords = usNumTextureSets * static_cast<uint32_t>(m_usMaxVQuantity);
		m_pkTexture = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(sizeof(NiPoint2) * uiNumTexCoords));
		// The low bit of the data flags holds the texture set count.
		m_usDataFlags ^= static_cast<uint16_t>((usNumTextureSets ^ m_usDataFlags) & 1);
	} else {
		m_pkTexture = nullptr;
	}

	m_usTriangles = 0;
	m_uiTriListLength = 0;
	m_pusTriList = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_usMaxIQuantity));
}

NiScreenElementsData::~NiScreenElementsData() {
	_NiFree(m_akPolygon);
	_NiFree(m_ausPIndexer);
}

const NiRTTI* NiScreenElementsData::GetRTTI() const {
	return &ms_RTTI;
}

// The polygon and indexer arrays are sized by the stored maximum (any previous arrays are not freed); each polygon
// is one 8-byte transfer.
void NiScreenElementsData::LoadBinary(NiStream& kStream) {
	NiTriShapeData::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_usMaxPQuantity);
	m_akPolygon = static_cast<Polygon*>(_NiMalloc(sizeof(Polygon) * m_usMaxPQuantity));
	m_ausPIndexer = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_usMaxPQuantity));
	uint32_t uiMaxPQuantity = m_usMaxPQuantity;
	NiStreamLoadBinary(kStream, m_akPolygon, uiMaxPQuantity);
	NiStreamLoadBinary(kStream, m_ausPIndexer, uiMaxPQuantity);
	NiStreamLoadBinary(kStream, m_usPGrowBy);
	NiStreamLoadBinary(kStream, m_usPQuantity);
	NiStreamLoadBinary(kStream, m_usMaxVQuantity);
	NiStreamLoadBinary(kStream, m_usVGrowBy);
	NiStreamLoadBinary(kStream, m_usMaxIQuantity);
	NiStreamLoadBinary(kStream, m_usIGrowBy);
}

void NiScreenElementsData::LinkObject(NiStream& kStream) {
	NiTriShapeData::LinkObject(kStream);
}

bool NiScreenElementsData::RegisterStreamables(NiStream& kStream) {
	return NiTriShapeData::RegisterStreamables(kStream);
}

void NiScreenElementsData::SaveBinary(NiStream& kStream) {
	NiTriShapeData::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_usMaxPQuantity);
	uint32_t uiMaxPQuantity = m_usMaxPQuantity;
	NiStreamSaveBinary(kStream, m_akPolygon, uiMaxPQuantity);
	NiStreamSaveBinary(kStream, m_ausPIndexer, uiMaxPQuantity);
	NiStreamSaveBinary(kStream, m_usPGrowBy);
	NiStreamSaveBinary(kStream, m_usPQuantity);
	NiStreamSaveBinary(kStream, m_usMaxVQuantity);
	NiStreamSaveBinary(kStream, m_usVGrowBy);
	NiStreamSaveBinary(kStream, m_usMaxIQuantity);
	NiStreamSaveBinary(kStream, m_usIGrowBy);
}

// The counts, then the polygon and indexer arrays byte by byte up to the maximum polygon count.
bool NiScreenElementsData::IsEqual(NiObject* pkObject) {
	if (!NiTriShapeData::IsEqual(pkObject))
		return false;

	NiScreenElementsData* pkOther = static_cast<NiScreenElementsData*>(pkObject);
	if (m_usMaxPQuantity != pkOther->m_usMaxPQuantity || m_usPGrowBy != pkOther->m_usPGrowBy ||
		m_usPQuantity != pkOther->m_usPQuantity || m_usMaxVQuantity != pkOther->m_usMaxVQuantity ||
		m_usVGrowBy != pkOther->m_usVGrowBy || m_usMaxIQuantity != pkOther->m_usMaxIQuantity ||
		m_usIGrowBy != pkOther->m_usIGrowBy)
		return false;

	if (std::memcmp(m_akPolygon, pkOther->m_akPolygon, sizeof(Polygon) * m_usMaxPQuantity))
		return false;
	return std::memcmp(m_ausPIndexer, pkOther->m_ausPIndexer, sizeof(uint16_t) * m_usMaxPQuantity) == 0;
}

void NiScreenElementsData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiTriShapeData::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_akPolygon", static_cast<const void*>(m_akPolygon)));
	pkStrings->Add(NiGetViewerString("m_ausPIndexer", static_cast<const void*>(m_ausPIndexer)));
	pkStrings->Add(NiGetViewerString("m_usMaxPQuantity", m_usMaxPQuantity));
	pkStrings->Add(NiGetViewerString("m_usPGrowBy", m_usPGrowBy));
	pkStrings->Add(NiGetViewerString("m_usPQuantity", m_usPQuantity));
	pkStrings->Add(NiGetViewerString("m_usMaxVQuantity", m_usMaxVQuantity));
	pkStrings->Add(NiGetViewerString("m_usVGrowBy", m_usVGrowBy));
	pkStrings->Add(NiGetViewerString("m_usMaxIQuantity", m_usMaxIQuantity));
	pkStrings->Add(NiGetViewerString("m_usIGrowBy", m_usIGrowBy));
}

// Appends a polygon of usNumVertices vertices and usNumTriangles triangles (a fan when ausTriList is null) and returns
// its index: the first unused indexer slot takes it, if any. Each array grows by whole multiples of its grow-by count.
// The texture set count is kept in the low bit of m_usDataFlags, each texture set's copy is offset by m_usVertices
// (already the new count), and the triangle list is filled from ausTriList after copying it there, as the game does.
int32_t NiScreenElementsData::Insert(uint16_t usNumVertices, uint16_t usNumTriangles, const uint16_t* ausTriList) {
	if (usNumVertices < 3)
		return -1;
	if (usNumTriangles > 0 && !ausTriList)
		return -1;
	if (usNumTriangles == 0)
		usNumTriangles = static_cast<uint16_t>(usNumVertices - 2);

	uint16_t usPolygon = m_usPQuantity;
	uint16_t usMaxPQuantity = m_usMaxPQuantity;
	m_usPQuantity = static_cast<uint16_t>(usPolygon + 1);
	if (static_cast<uint16_t>(usPolygon + 1) > usMaxPQuantity) {
		uint16_t usNewMaxQuantity = static_cast<uint16_t>(usMaxPQuantity + m_usPGrowBy);
		Polygon* akNewPolygon = static_cast<Polygon*>(_NiMalloc(sizeof(Polygon) * usNewMaxQuantity));
		rt::memcpy(akNewPolygon, m_akPolygon, sizeof(Polygon) * m_usMaxPQuantity);
		_NiFree(m_akPolygon);
		m_akPolygon = akNewPolygon;
		uint16_t* ausNewPIndexer = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usNewMaxQuantity));
		rt::memcpy(ausNewPIndexer, m_ausPIndexer, sizeof(uint16_t) * m_usMaxPQuantity);
		std::memset(&ausNewPIndexer[m_usMaxPQuantity], 0xFF, sizeof(uint16_t) * (usNewMaxQuantity - m_usMaxPQuantity));
		_NiFree(m_ausPIndexer);
		m_ausPIndexer = ausNewPIndexer;
		m_usMaxPQuantity = usNewMaxQuantity;
	}

	Polygon& kPoly = m_akPolygon[usPolygon];
	kPoly.m_usNumVertices = usNumVertices;
	kPoly.m_usVOffset = m_usVertices;
	kPoly.m_usNumTriangles = usNumTriangles;
	kPoly.m_usIOffset = static_cast<uint16_t>(3 * m_usTriangles);

	for (uint16_t i = 0; i < m_usPQuantity; i++) {
		if (m_ausPIndexer[i] == 0xFFFF) {
			m_ausPIndexer[i] = usPolygon;
			usPolygon = i;
			break;
		}
	}

	m_usVertices += kPoly.m_usNumVertices;
	int32_t iExcess = m_usVertices - m_usMaxVQuantity;
	if (iExcess > 0) {
		float fRatio = static_cast<float>(iExcess) / static_cast<float>(m_usVGrowBy);
		uint16_t usNewMaxQuantity =
			static_cast<uint16_t>(m_usMaxVQuantity + m_usVGrowBy * (static_cast<int32_t>(fRatio + 0.5) + 1));
		uint32_t uiNewMaxQuantity = usNewMaxQuantity;

		NiPoint3* akNewVertex =
			static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(uiNewMaxQuantity <= SIZE_MAX / sizeof(NiPoint3)
					? sizeof(NiPoint3) * uiNewMaxQuantity
					: SIZE_MAX));
		size_t stPointSize = sizeof(NiPoint3) * m_usMaxVQuantity;
		rt::memcpy(akNewVertex, m_pkVertex, stPointSize);
		MemoryManager_Singleton.Deallocate(m_pkVertex);
		m_pkVertex = akNewVertex;

		if (m_pkNormal) {
			NiPoint3* akNewNormal =
				static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(uiNewMaxQuantity <= SIZE_MAX / sizeof(NiPoint3)
						? sizeof(NiPoint3) * uiNewMaxQuantity
						: SIZE_MAX));
			rt::memcpy(akNewNormal, m_pkNormal, stPointSize);
			MemoryManager_Singleton.Deallocate(m_pkNormal);
			m_pkNormal = akNewNormal;
			for (uint16_t i = m_usMaxVQuantity; i < usNewMaxQuantity; i++) {
				float fX = -NiPoint3_UNIT_Z.x;
				float fY = -NiPoint3_UNIT_Z.y;
				float fZ = -NiPoint3_UNIT_Z.z;
				NiPoint3& kNormal = m_pkNormal[i];
				kNormal.x = fX;
				kNormal.y = fY;
				kNormal.z = fZ;
			}
		}

		if (m_pkColor) {
			NiColorA* akNewColor =
				static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(uiNewMaxQuantity <= SIZE_MAX / sizeof(NiColorA)
						? sizeof(NiColorA) * uiNewMaxQuantity
						: SIZE_MAX));
			if (akNewColor) {
				for (uint32_t i = 0; i < uiNewMaxQuantity; i++) {
					akNewColor[i].r = 0.0f;
					akNewColor[i].g = 0.0f;
					akNewColor[i].b = 0.0f;
					akNewColor[i].a = 0.0f;
				}
			}
			rt::memcpy(akNewColor, m_pkColor, sizeof(NiColorA) * m_usMaxVQuantity);
			MemoryManager_Singleton.Deallocate(m_pkColor);
			m_pkColor = akNewColor;
		}

		uint16_t usNumTextureSets = static_cast<uint16_t>(m_usDataFlags & 1);
		if (usNumTextureSets) {
			uint32_t uiNumTexCoords = usNumTextureSets * uiNewMaxQuantity;
			NiPoint2* akNewTexture =
				static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(uiNumTexCoords <= SIZE_MAX / sizeof(NiPoint2)
						? sizeof(NiPoint2) * uiNumTexCoords
						: SIZE_MAX));
			size_t stTextureSize = sizeof(NiPoint2) * m_usMaxVQuantity;
			for (uint32_t uiSet = 0; uiSet < usNumTextureSets; uiSet++) {
				uint32_t uiOffset = uiSet * m_usVertices;
				rt::memcpy(&akNewTexture[uiOffset], &m_pkTexture[uiOffset], stTextureSize);
			}
			MemoryManager_Singleton.Deallocate(m_pkTexture);
			m_pkTexture = akNewTexture;
		}

		m_usMaxVQuantity = usNewMaxQuantity;
	}

	m_usTriangles += usNumTriangles;
	uint32_t uiNumIndices = 3 * static_cast<uint32_t>(usNumTriangles);
	m_uiTriListLength += uiNumIndices;
	iExcess = 3 * m_usTriangles - m_usMaxIQuantity;
	if (iExcess > 0) {
		float fRatio = static_cast<float>(iExcess) / static_cast<float>(m_usIGrowBy);
		uint16_t usNewMaxQuantity =
			static_cast<uint16_t>(m_usMaxIQuantity + m_usIGrowBy * (static_cast<int32_t>(fRatio + 0.5) + 1));
		uint16_t* ausNewTriList = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usNewMaxQuantity));
		rt::memcpy(ausNewTriList, m_pusTriList, sizeof(uint16_t) * m_usMaxIQuantity);
		_NiFree(m_pusTriList);
		m_pusTriList = ausNewTriList;
		m_usMaxIQuantity = usNewMaxQuantity;
	}

	uint16_t* pusTriList = &m_pusTriList[kPoly.m_usIOffset];
	if (ausTriList) {
		rt::memcpy(pusTriList, ausTriList, sizeof(uint16_t) * uiNumIndices);
		for (uint32_t i = 0; i < uiNumIndices; i++)
			pusTriList[i] = static_cast<uint16_t>(ausTriList[i] + kPoly.m_usVOffset);
	} else {
		for (uint16_t i = 0; i < usNumTriangles; i++) {
			pusTriList[0] = kPoly.m_usVOffset;
			pusTriList[1] = static_cast<uint16_t>(kPoly.m_usVOffset + i + 1);
			pusTriList[2] = static_cast<uint16_t>(kPoly.m_usVOffset + i + 2);
			pusTriList += 3;
		}
	}

	MarkAsChanged(NiGeometryData::VERTEX_MASK);
	return usPolygon;
}

// The right and bottom sums are rounded to float; the left and top coordinates are stored as passed.
bool NiScreenElementsData::SetRectangle(int32_t iPolygon, float fLeft, float fTop, float fWidth, float fHeight) {
	NiScreenElementsData::Polygon* m_akPolygon;
	int32_t m_usVOffset;
	if (iPolygon < 0) {
		return (bool)0;
	}
	if (iPolygon >= ((int32_t)m_usMaxPQuantity)) {
		return (bool)0;
	}
	uint16_t v7 = m_ausPIndexer[iPolygon];
	if (v7 == 0xffff) {
		return (bool)0;
	}
	int32_t v8 = (int32_t)v7;
	m_akPolygon = this->m_akPolygon;
	uint8_t v10 = (uint8_t)(m_akPolygon[v8].m_usNumVertices == 4);
	NiScreenElementsData::Polygon* v11 = &m_akPolygon[v8];
	if (!v10) {
		return (bool)0;
	}
	m_usVOffset = (int32_t)v11->m_usVOffset;
	double v13 = (double)fLeft;
	int32_t v14 = m_usVOffset;
	float fLefta = fLeft + fWidth;
	++m_usVOffset;
	float iPolygona = fTop + fHeight;
	m_pkVertex[v14].x = (float)v13;
	m_pkVertex[v14].y = fTop;
	m_pkVertex[v14].z = (float)0.0;
	int32_t v15 = m_usVOffset;
	m_pkVertex[v15].x = (float)v13;
	++m_usVOffset;
	m_pkVertex[v15].y = iPolygona;
	m_pkVertex[v15].z = (float)0.0;
	int32_t v16 = m_usVOffset;
	m_pkVertex[v16].x = fLefta;
	m_pkVertex[v16].y = iPolygona;
	m_pkVertex[v16].z = (float)0.0;
	int32_t v17 = m_usVOffset + 1;
	m_pkVertex[v17].x = fLefta;
	m_pkVertex[v17].y = fTop;
	m_pkVertex[v17].z = (float)0.0;
	NiGeometryData::MarkAsChanged(1u);
	m_bBoundNeedsUpdate = (bool)1;
	return (bool)1;
}

// The PC texture-set check uses only bit zero.
bool NiScreenElementsData::SetTextures(int32_t iPolygon, uint16_t usSet, float fLeft, float fTop, float fRight,
	float fBottom) {
	if (!m_pkTexture || iPolygon < 0 || iPolygon >= m_usMaxPQuantity)
		return false;

	uint16_t usPolygon = m_ausPIndexer[iPolygon];
	if (usPolygon == 0xFFFF)
		return false;

	Polygon* pkPolygon = &m_akPolygon[usPolygon];
	if (pkPolygon->m_usNumVertices != 4 || usSet >= (m_usDataFlags & 1))
		return false;

	NiPoint2* pkTexture = GetTextureSet(usSet);
	uint32_t uiOffset = pkPolygon->m_usVOffset;
	pkTexture[uiOffset].x = fLeft;
	pkTexture[uiOffset].y = fTop;
	pkTexture[uiOffset + 1].x = fLeft;
	pkTexture[uiOffset + 1].y = fBottom;
	pkTexture[uiOffset + 2].x = fRight;
	pkTexture[uiOffset + 3].x = fRight;
	pkTexture[uiOffset + 2].y = fBottom;
	pkTexture[uiOffset + 3].y = fTop;
	MarkAsChanged(TEXTURE_MASK);
	return true;
}

// Clear the dirty bound even when there are no vertices.
void NiScreenElementsData::UpdateBound() {
	uint16_t usVertices = m_usVertices;
	m_bBoundNeedsUpdate = false;
	if (usVertices)
		m_kBound.ComputeFromData(usVertices, m_pkVertex);
}

NiObject* NiScreenElementsData::CreateObject() {
	void* pkObject = NiMemObject::operator new(sizeof(NiScreenElementsData));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiScreenElementsData;
}
