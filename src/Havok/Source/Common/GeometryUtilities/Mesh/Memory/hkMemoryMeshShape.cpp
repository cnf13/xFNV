#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshShape.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshVertexBuffer.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedBool.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedInt.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedString.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialShader.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialShaderSet.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureFile.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureInplace.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxTriangleSelectionChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexFloatDataChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexIntDataChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexSelectionChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexVectorDataChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMeshSection.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/SceneData/Scene/hkxScene.h"
#include "Havok/Source/Common/SceneData/Selection/hkxNodeSelectionSet.h"
#include "Havok/Source/Common/SceneData/Skin/hkxSkinBinding.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

static void finishLoadedObjecthkMemoryMeshShape(void* p, int32_t finishing);
static void cleanupLoadedObjecthkMemoryMeshShape(void* p);
// Havok SDK name; not in the PDB.
static int32_t calculateNumIndices(int32_t a1, int32_t a2);

// The class record's member table (the SDK's file-local hkMemoryMeshShapeClass_Members of
// hkMemoryMeshShapeClass.cpp). Not named in the PDB or IDA; name chosen here, after the SDK's.
using hkMemoryMeshShapeClassMemberTable = const hkClassMember[4];
static hkMemoryMeshShapeClassMemberTable hkMemoryMeshShapeClass_Members{
	{"sections", &hkMeshSectionCinfoClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, 0,
		0x8, nullptr},
	{"indices16", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x14, nullptr},
	{"indices32", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT32}, 0, 0, 0x20, nullptr},
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x2C, nullptr}};
hkClass hkMemoryMeshShapeClass("hkMemoryMeshShape", &hkMeshShapeClass, 48, nullptr, 0, nullptr, 0,
	hkMemoryMeshShapeClass_Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkMemoryMeshShapeTypeInfo{"hkMemoryMeshShape", finishLoadedObjecthkMemoryMeshShape,
	cleanupLoadedObjecthkMemoryMeshShape, getVtablehkMemoryMeshShape(), &typeid(hkMemoryMeshShape),
	sizeof(hkMemoryMeshShape)};

static void cleanupLoadedObjecthkMemoryMeshShape(void* p) {
	static_cast<hkMemoryMeshShape*>(p)->~hkMemoryMeshShape();
}

// The SDK's finish constructor hkMemoryMeshShape(flag) in place, which the exe inlines.
static void finishLoadedObjecthkMemoryMeshShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkMemoryMeshShape(kFlag);
	}
}

static int32_t calculateNumIndices(int32_t a1, int32_t a2) {
	int32_t result;
	switch (a1) {
	case 1: {
		result = a2;
		break;
	}
	case 2: {
		result = 2 * a2;
		break;
	}
	case 3: {
		result = 3 * a2;
		break;
	}
	case 4: {
		result = a2 + 2;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return result;
}

// The type info's finish-loaded-object callback for hkxVertexVectorDataChannel: the null check, then its finish
// constructor, which the exe inlines.
void finishLoadedObjecthkxVertexVectorDataChannel(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxVertexVectorDataChannel(kFlag);
	}
}

const void* getVtablehkxVertexVectorDataChannel() {
	return hkGetVtableOf<hkxVertexVectorDataChannel>();
}

// As above, for hkxVertexSelectionChannel.
void finishLoadedObjecthkxVertexSelectionChannel(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxVertexSelectionChannel(kFlag);
	}
}

const void* getVtablehkxVertexSelectionChannel() {
	return hkGetVtableOf<hkxVertexSelectionChannel>();
}

// As above, for hkxVertexIntDataChannel.
void finishLoadedObjecthkxVertexIntDataChannel(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxVertexIntDataChannel(kFlag);
	}
}

const void* getVtablehkxVertexIntDataChannel() {
	return hkGetVtableOf<hkxVertexIntDataChannel>();
}

// As above, for hkxVertexFloatDataChannel.
void finishLoadedObjecthkxVertexFloatDataChannel(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxVertexFloatDataChannel(kFlag);
	}
}

const void* getVtablehkxVertexFloatDataChannel() {
	return hkGetVtableOf<hkxVertexFloatDataChannel>();
}

// As above, for hkxTriangleSelectionChannel.
void finishLoadedObjecthkxTriangleSelectionChannel(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxTriangleSelectionChannel(kFlag);
	}
}

const void* getVtablehkxTriangleSelectionChannel() {
	return hkGetVtableOf<hkxTriangleSelectionChannel>();
}

const void* getVtablehkxTextureInplace() {
	return hkGetVtableOf<hkxTextureInplace>();
}

const void* getVtablehkxTextureFile() {
	return hkGetVtableOf<hkxTextureFile>();
}

// The class registry's finish-loaded-object callback for hkxTextureInplace: the null check, then the class's finish
// constructor, which the exe inlines (cdecl, 2 stack arguments, 0xE016E0).
void finishLoadedObjecthkxTextureInplace(void* p, int32_t finishing) {
	if (!p) {
		return;
	}
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = finishing;
	::new (p) hkxTextureInplace(flag);
}

// As above, for hkxTextureFile (cdecl, 2 stack arguments, 0xE017F0).
void finishLoadedObjecthkxTextureFile(void* p, int32_t finishing) {
	if (!p) {
		return;
	}
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = finishing;
	::new (p) hkxTextureFile(flag);
}

// The type info's finish-loaded-object callback for hkxSparselyAnimatedString: the null check, then its finish
// constructor, which the exe inlines.
void finishLoadedObjecthkxSparselyAnimatedString(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxSparselyAnimatedString(kFlag);
	}
}

const void* getVtablehkxSparselyAnimatedString() {
	return hkGetVtableOf<hkxSparselyAnimatedString>();
}

// As above, for hkxSparselyAnimatedInt.
void finishLoadedObjecthkxSparselyAnimatedInt(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxSparselyAnimatedInt(kFlag);
	}
}

const void* getVtablehkxSparselyAnimatedInt() {
	return hkGetVtableOf<hkxSparselyAnimatedInt>();
}

// As above, for hkxSparselyAnimatedBool.
void finishLoadedObjecthkxSparselyAnimatedBool(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxSparselyAnimatedBool(kFlag);
	}
}

const void* getVtablehkxSparselyAnimatedBool() {
	return hkGetVtableOf<hkxSparselyAnimatedBool>();
}

// As above, for hkxSkinBinding.
void finishLoadedObjecthkxSkinBinding(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxSkinBinding(kFlag);
	}
}

const void* getVtablehkxSkinBinding() {
	return hkGetVtableOf<hkxSkinBinding>();
}

// The type info's finish-loaded-object callback for hkxNodeSelectionSet: the null check, then its finish
// constructor, which the exe inlines.
void finishLoadedObjecthkxNodeSelectionSet(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxNodeSelectionSet(kFlag);
	}
}

const void* getVtablehkxNodeSelectionSet() {
	return hkGetVtableOf<hkxNodeSelectionSet>();
}

void cleanupLoadedObjecthkxMesh(void* p) {
	static_cast<hkxMesh*>(p)->~hkxMesh();
}

// The SDK's getVtable: hkxMesh's finish constructor (flag 0), out of line, on a 16-aligned stack object; returns the
// object's first word, the vptr that constructor stores: the exe's vtable address, raw memory to src.
const void* getVtablehkxMesh() {
	return hkGetVtableOf<hkxMesh>();
}

// As above, for hkxMesh: the null check, then its out-of-line finish constructor (cdecl, 2 stack arguments, 0xE01E20).
void finishLoadedObjecthkxMesh(void* p, int32_t finishing) {
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (!p) {
		return;
	}
	::new (p) hkxMesh(aFlag);
}

// As above, for hkxMeshUserChannelInfo (cdecl, 2 stack arguments, 0xE01E80).
void finishLoadedObjecthkxMeshUserChannelInfo(void* p, int32_t finishing) {
	if (!p) {
		return;
	}
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = finishing;
	::new (p) hkxMesh::UserChannelInfo(flag);
}

const void* getVtablehkxMeshUserChannelInfo() {
	return hkGetVtableOf<hkxMesh::UserChannelInfo>();
}

// The type info's finish-loaded-object callback for hkxMeshSection: the null check, then its finish constructor,
// which the exe inlines.
void finishLoadedObjecthkxMeshSection(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkxMeshSection(kFlag);
	}
}

const void* getVtablehkxMeshSection() {
	return hkGetVtableOf<hkxMeshSection>();
}

// The SDK's getVtable: hkxMaterialShaderSet's finish constructor (flag 0), out of line, on a 16-aligned stack
// object; returns the object's first word, the vptr that constructor stores: the exe's vtable address, raw memory
// to src.
const void* getVtablehkxMaterialShaderSet() {
	return hkGetVtableOf<hkxMaterialShaderSet>();
}

// The class registry's finish-loaded-object callback of the class at 0xE020C0: the null check, then the class's
// out-of-line finish constructor (cdecl, 2 stack arguments, 0xE020C0). IDA name; the PDB's finishLoadedObject<Class>
// name of this address is not resolved here.
void finishLoadedObjecthkxMaterialShaderSet(void* p, int32_t finishing) {
	void* apMemory = static_cast<void*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (!apMemory) {
		return;
	}
	::new (apMemory) hkxMaterialShaderSet(aFlag);
}

// As above, for hkxMaterialShader (cdecl, 2 stack arguments, 0xE02140).
void finishLoadedObjecthkxMaterialShader(void* p, int32_t finishing) {
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (!p) {
		return;
	}
	::new (p) hkxMaterialShader(aFlag);
}

void cleanupLoadedObjecthkxVertexVectorDataChannel(void* p) {
	static_cast<hkxVertexVectorDataChannel*>(p)->~hkxVertexVectorDataChannel();
}

void cleanupLoadedObjecthkxVertexSelectionChannel(void* p) {
	static_cast<hkxVertexSelectionChannel*>(p)->~hkxVertexSelectionChannel();
}

void cleanupLoadedObjecthkxVertexIntDataChannel(void* p) {
	static_cast<hkxVertexIntDataChannel*>(p)->~hkxVertexIntDataChannel();
}

void cleanupLoadedObjecthkxVertexFloatDataChannel(void* p) {
	static_cast<hkxVertexFloatDataChannel*>(p)->~hkxVertexFloatDataChannel();
}

void cleanupLoadedObjecthkxTriangleSelectionChannel(void* p) {
	static_cast<hkxTriangleSelectionChannel*>(p)->~hkxTriangleSelectionChannel();
}

void cleanupLoadedObjecthkxSparselyAnimatedBool(void* p) {
	static_cast<hkxSparselyAnimatedBool*>(p)->~hkxSparselyAnimatedBool();
}

void cleanupLoadedObjecthkxMaterialShaderSet(void* p) {
	static_cast<hkxMaterialShaderSet*>(p)->~hkxMaterialShaderSet();
}

const void* getVtablehkMemoryMeshShape() {
	return hkGetVtableOf<hkMemoryMeshShape>();
}

int32_t calculateNumPrimitives(int32_t primitiveType, int32_t numIndices) {
	switch (primitiveType) {
	case hkMeshSection::PRIMITIVE_TYPE_POINT_LIST:
		return numIndices;
	case hkMeshSection::PRIMITIVE_TYPE_LINE_LIST:
		return numIndices / 2;
	case hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST:
		return numIndices / 3;
	case hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP:
		return numIndices - 2;
	default:
		return 0;
	}
}

// Copies the sections, taking a reference on each vertex buffer and material, and copies each section's indices into
// the shape's own 16- or 32-bit index array, pointing the section at the copy.
hkMemoryMeshShape::hkMemoryMeshShape(const hkMeshSectionCinfo* sections, int32_t numSections) {
	m_name = nullptr;
	int32_t iNumIndices16 = 0;
	int32_t iNumIndices32 = 0;
	for (int32_t i = 0; i < numSections; ++i) {
		int32_t iNumIndices = calculateNumIndices(sections[i].m_primitiveType.m_storage, sections[i].m_numPrimitives);
		if (sections[i].m_indexType.m_storage == hkMeshSection::INDEX_TYPE_UINT16) {
			iNumIndices16 += iNumIndices;
		} else if (sections[i].m_indexType.m_storage == hkMeshSection::INDEX_TYPE_UINT32) {
			iNumIndices32 += iNumIndices;
		}
	}
	m_indices16.reserve(iNumIndices16);
	m_indices32.reserve(iNumIndices32);
	m_sections.setSize(numSections);
	for (int32_t i = 0; i < numSections; ++i) {
		hkMeshSectionCinfo& section = m_sections.m_data[i];
		section = sections[i];
		section.m_vertexBuffer->addReference();
		if (section.m_material) {
			section.m_material->addReference();
		}
		if (section.m_indexType.m_storage == hkMeshSection::INDEX_TYPE_UINT16) {
			int32_t iNumIndices = calculateNumIndices(section.m_primitiveType.m_storage, section.m_numPrimitives);
			uint16_t* pIndices = m_indices16.expandBy(iNumIndices);
			hkString::memCpy(pIndices, sections[i].m_indices, sizeof(uint16_t) * iNumIndices);
			section.m_indices = pIndices;
		} else if (section.m_indexType.m_storage == hkMeshSection::INDEX_TYPE_UINT32) {
			int32_t iNumIndices = calculateNumIndices(section.m_primitiveType.m_storage, section.m_numPrimitives);
			uint32_t* pIndices = m_indices32.expandBy(iNumIndices);
			hkString::memCpy(pIndices, sections[i].m_indices, sizeof(uint32_t) * iNumIndices);
			section.m_indices = pIndices;
		}
	}
}

// Drops the references the sections hold; the arrays and the name free themselves.
hkMemoryMeshShape::~hkMemoryMeshShape() {
	for (int32_t i = 0; i < m_sections.m_size; ++i) {
		hkMeshSectionCinfo& section = m_sections.m_data[i];
		section.m_vertexBuffer->removeReference();
		if (section.m_material) {
			section.m_material->removeReference();
		}
	}
}

const hkClass* hkMemoryMeshShape::getClassType() const {
	return &hkMemoryMeshShapeClass;
}

void hkMemoryMeshShape::calcContentStatistics(hkStatisticsCollector* apCollector, const hkClass* cls) const {
	int32_t m_capacityAndFlags = m_indices16.m_capacityAndFlags;
	if (m_capacityAndFlags >= 0 || apCollector->m_includeUnmanagedArrayMemory) {
		int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(m_indices16.m_data,
			static_cast<int32_t>(sizeof(*m_indices16.m_data)) * (m_capacityAndFlags & 0x3FFFFFFF));
		apCollector->addChunk(hkStatisticsCollector::MEM_ARRAY, "indices16", m_indices16.m_data,
			static_cast<int32_t>(sizeof(*m_indices16.m_data)) * m_indices16.m_size, AllocatedSize);
	}
	int32_t v7 = m_indices32.m_capacityAndFlags;
	if (v7 >= 0 || apCollector->m_includeUnmanagedArrayMemory) {
		int32_t v8 = hkStatisticsCollector::getAllocatedSize(m_indices32.m_data,
			static_cast<int32_t>(sizeof(*m_indices32.m_data)) * v7);
		apCollector->addChunk(hkStatisticsCollector::MEM_ARRAY, "indices32", m_indices32.m_data,
			static_cast<int32_t>(sizeof(*m_indices32.m_data)) * m_indices32.m_size, v8);
	}
	int32_t v9 = m_sections.m_capacityAndFlags;
	if (v9 >= 0 || apCollector->m_includeUnmanagedArrayMemory) {
		int32_t v10 = hkStatisticsCollector::getAllocatedSize(m_sections.m_data,
			static_cast<int32_t>(sizeof(*m_sections.m_data)) * v9);
		apCollector->addChunk(hkStatisticsCollector::MEM_ARRAY, "sections", m_sections.m_data,
			static_cast<int32_t>(sizeof(*m_sections.m_data)) * m_sections.m_size, v10);
	}
	if (m_sections.m_size > 0) {
		int32_t v11 = 0;
		int32_t collector = m_sections.m_size;
		do {
			hkMeshSectionCinfo* m_data = m_sections.m_data;
			apCollector->addReferencedObject("vertexBuffer", m_data[v11].m_vertexBuffer, 0);
			apCollector->addReferencedObject("material", m_data[v11++].m_material, 0);
			--collector;
		} while (collector);
	}
}

int32_t hkMemoryMeshShape::getNumSections() const {
	return m_sections.m_size;
}

void hkMemoryMeshShape::lockSection(int32_t sectionIndex, uint8_t accessFlags, hkMeshSection& sectionOut) const {
	hkMeshVertexBuffer* m_vertexBuffer;
	hkMeshSectionCinfo* v4 = &m_sections.m_data[sectionIndex];
	sectionOut.m_primitiveType.m_storage = v4->m_primitiveType.m_storage;
	sectionOut.m_numPrimitives = v4->m_numPrimitives;
	sectionOut.m_numIndices = calculateNumIndices(v4->m_primitiveType.m_storage, v4->m_numPrimitives);
	sectionOut.m_vertexStartIndex = v4->m_vertexStartIndex;
	sectionOut.m_indexType.m_storage = v4->m_indexType.m_storage;
	sectionOut.m_transformIndex = v4->m_transformIndex;
	if ((accessFlags & 2) != 0) {
		m_vertexBuffer = v4->m_vertexBuffer;
	} else {
		m_vertexBuffer = 0;
	}
	sectionOut.m_vertexBuffer = m_vertexBuffer;
	sectionOut.m_indices = 0;
	if ((accessFlags & 1) != 0) {
		if (v4->m_indexType.m_storage) {
			sectionOut.m_indices = v4->m_indices;
		}
	}
	hkMeshMaterial* m_material = v4->m_material;
	sectionOut.m_sectionIndex = sectionIndex;
	sectionOut.m_material = m_material;
}

void hkMemoryMeshShape::unlockSection(const hkMeshSection& section) const {
	;
}

// The exe inlines hkStringPtr::cString.
const char* hkMemoryMeshShape::getName() const {
	return m_name.cString();
}

const hkClass& hkMemoryMeshShape::staticClass() {
	return hkMemoryMeshShapeClass;
}

// The loaded shader array is kept as it is.
hkxMaterialShaderSet::hkxMaterialShaderSet(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag), m_shaders(flag) {}

const hkClass& hkxMaterialShaderSet::staticClass() {
	return hkxMaterialShaderSetClass;
}

const hkClass& hkxTriangleSelectionChannel::staticClass() {
	return hkxTriangleSelectionChannelClass;
}

const hkClass& hkxVertexFloatDataChannel::staticClass() {
	return hkxVertexFloatDataChannelClass;
}

const hkClass& hkxVertexIntDataChannel::staticClass() {
	return hkxVertexIntDataChannelClass;
}

const hkClass& hkxVertexSelectionChannel::staticClass() {
	return hkxVertexSelectionChannelClass;
}

const hkClass& hkxVertexVectorDataChannel::staticClass() {
	return hkxVertexVectorDataChannelClass;
}

const hkClass& hkxMeshSection::staticClass() {
	return hkxMeshSectionClass;
}

// As above, for hkxScene (cdecl, 2 stack arguments, 0xE01C30).
void finishLoadedObjecthkxScene(void* p, int32_t finishing) {
	if (!p) {
		return;
	}
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = finishing;
	::new (p) hkxScene(flag);
}
