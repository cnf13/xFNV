#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/DebugUtil/TraceStream/hkTraceStream.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroup.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxEnum.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedEnum.h"
#include "Havok/Source/Common/SceneData/Environment/hkxEnvironment.h"
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexBuffer.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/CompressedMesh/hkpCompressedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/SimpleMesh/hkpSimpleMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/MultiRay/hkpMultiRayShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/BallSocket/hkpBallSocketChainData.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxAttributeGroupTypeInfo = {
    "hkxAttributeGroup",
    finishLoadedObjecthkxAttributeGroup,
    cleanupLoadedObjecthkxAttributeGroup,
    nullptr,
    &typeid(hkxAttributeGroup),
    sizeof(hkxAttributeGroup)
};

// The class record's member table.
using hkxAttributeGroupClassMemberArray = const hkClassMember[2];
static hkxAttributeGroupClassMemberArray hkxAttributeGroupClass_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"attributes",
     &hkxAttributeClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x4,
     nullptr}
};
hkClass hkxAttributeGroupClass(
    "hkxAttributeGroup",
    nullptr,
    16,
    nullptr,
    0,
    nullptr,
    0,
    hkxAttributeGroupClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

void finishLoadedObjecthkpCompressedMeshShapeChunk(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpBallAndSocketConstraintDataAtoms(void* p) {
    ;
}

void finishLoadedObjecthkpBallAndSocketConstraintDataAtoms(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpSimpleMeshShapeTriangle(void* p) {
    ;
}

void finishLoadedObjecthkpAgent1nSector(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkxVertexDescriptionElementDecl(void* p) {
    ;
}

void finishLoadedObjecthkxMaterialTextureStage(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkPackfileSectionHeader(void* p) {
    ;
}

void cleanupLoadedObjecthkSphere(void* p) {
    ;
}

void finishLoadedObjecthkpTypedBroadPhaseHandle(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpBroadPhaseHandle(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkSweptTransform(void* p) {
    ;
}

void cleanupLoadedObjecthkpMultiRayShapeRay(void* p) {
    ;
}

void cleanupLoadedObjecthkpListShapeChildInfo(void* p) {
    ;
}

void cleanupLoadedObjecthkTraceStreamTitle(void* p) {
    ;
}

void cleanupLoadedObjecthkMotionState(void* p) {
    ;
}

// The type info's finish-loaded-object callback for hkpBallAndSocketConstraintData: its finish constructor, when
// the object is not null.
void finishLoadedObjecthkpBallAndSocketConstraintData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpBallAndSocketConstraintData(flag);
    }
}

const void* getVtablehkpBallAndSocketConstraintData() {
    return hkGetVtableOf<hkpBallAndSocketConstraintData>();
}

void cleanupLoadedObjecthkpTypedBroadPhaseHandle(void* p) {
    ;
}

// The type info's vtable getter for hkpSimpleMeshShape: finish-constructs one on the stack, not finishing, and returns
// its vtable.
const void* getVtablehkpSimpleMeshShape() {
    // The class has no native default constructor.
    return hkGetVtableOf<hkpSimpleMeshShape>();
}

// The type info's finish-loaded-object callback for hkpMultiRayShape: its finish constructor, when the object is
// not null.
void finishLoadedObjecthkpMultiRayShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpMultiRayShape(flag);
    }
}

const void* getVtablehkpMultiRayShape() {
    return hkGetVtableOf<hkpMultiRayShape>();
}

void finishLoadedObjecthkpListShape(void* p, int32_t finishing) {
    hkpListShape* a1 = static_cast<hkpListShape*>(p);
    int32_t a2 = finishing;
    if (a1) {
        ::new (a1) hkpListShape(static_cast<hkFinishLoadedObjectFlag>(a2));
    }
}

void cleanupLoadedObjecthkpConvexVerticesShapeFourVectors(void* p) {
    ;
}

const void* getVtablehkpConvexVerticesShape() {
    // The class has no native default constructor.
    return hkGetVtableOf<hkpConvexVerticesShape>();
}

void cleanupLoadedObjecthkpCompressedMeshShapeBigTriangle(void* p) {
    ;
}

// The type info's vtable getter for hkpCompressedMeshShape: finish-constructs one on the stack, not finishing, and
// returns its vtable.
const void* getCompressedMeshShapeVtable() {
    return hkGetVtableOf<hkpCompressedMeshShape>();
}

void finishLoadedObjecthkpCompressedMeshShapeConvexPiece(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpCompressedMeshShapeChunk(void* a1) {
    static_cast<hkpCompressedMeshShape::Chunk*>(a1)->~Chunk();
}

void cleanupLoadedObjecthkpCompressedMeshShapeConvexPiece(void* a1) {
    static_cast<hkpCompressedMeshShape::ConvexPiece*>(a1)->~ConvexPiece();
}

void cleanupLoadedObjecthkpCdBody(void* p) {
    ;
}

void cleanupLoadedObjecthkpBroadPhaseHandle(void* p) {
    ;
}

void cleanupLoadedObjecthkpAgent1nSector(void* p) {
    ;
}

// The type info's cleanup callback for hkxVertexDescription: runs its destructor, which frees m_decls.
void cleanupLoadedObjecthkxVertexDescription(void* a1) {
    hkxVertexDescription* pDesc = static_cast<hkxVertexDescription*>(a1);
    // EXE-INLINED none: hkArray<hkxVertexDescription::ElementDecl>::~hkArray
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t capacityAndFlags = pDesc->m_decls.m_capacityAndFlags;
    pDesc->m_decls.m_size = 0;
    if (capacityAndFlags >= 0) {
        pHeap->bufFree(
            pDesc->m_decls.m_data,
            (capacityAndFlags & hkArrayBase<hkxVertexDescription::ElementDecl>::CAPACITY_MASK) *
                sizeof(hkxVertexDescription::ElementDecl)
        );
    }
    pDesc->m_decls.m_capacityAndFlags = hkArrayBase<hkxVertexDescription::ElementDecl>::DONT_DEALLOCATE_FLAG;
    pDesc->m_decls.m_data = nullptr;
}

// The type info's vtable getter for hkxVertexBuffer: finish-constructs one on the stack, not finishing, and returns its
// vtable.
const void* getVtablehkxVertexBuffer() {
    return hkGetVtableOf<hkxVertexBuffer>();
}

// The finish function the reflection registers for hkxVertexBuffer: finishes the loaded object
// with its finish constructor, when the object is not null.
void finishLoadedObjecthkxVertexBuffer(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxVertexBuffer(flag);
    }
}

void finishLoadedObjecthkxVertexBufferVertexData(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkxVertexBufferVertexData(void* a1) {
    static_cast<hkxVertexBuffer::VertexData*>(a1)->~VertexData();
}

void finishLoadedObjecthkxNodeAnnotationData(void* p, int32_t finishing) {
    if (p) {
        ::new (static_cast<hkStringPtr*>(p) + 1) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
    }
}

void cleanupLoadedObjecthkxNodeAnnotationData(void* p) {
    static_cast<hkxNode::AnnotationData*>(p)->m_description.~hkStringPtr();
}

// The type info's finish-loaded-object callback for hkxNode: its finish constructor, when the object is not null.
void finishLoadedObjecthkxNode(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxNode(flag);
    }
}

void cleanupLoadedObjecthkxMaterialTextureStage(void* p) {
    hkReferencedObject** a1 = static_cast<hkReferencedObject**>(p);
    if (*a1) {
        (*a1)->removeReference();
    }
    *a1 = nullptr;
}

// The type info's finish-loaded-object callback for hkxEnvironment::Variable: its finish constructor finishes the name,
// then the value.
void finishLoadedObjecthkxEnvironmentVariable(void* p, int32_t finishing) {
    if (p) {
        hkxEnvironment::Variable* pVariable = static_cast<hkxEnvironment::Variable*>(p);
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (&pVariable->m_name) hkStringPtr(flag);
        ::new (&pVariable->m_value) hkStringPtr(flag);
    }
}

// The finish function the reflection registers for hkxEnvironment: finishes the loaded object
// with its finish constructor, when the object is not null.
void finishLoadedObjecthkxEnvironment(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxEnvironment(flag);
    }
}

// The type info's vtable getter for hkxEnvironment: finish-constructs one on the stack, not finishing, and returns its
// vtable (`*(void**)&o`).
const void* getVtablehkxEnvironment() {
    return hkGetVtableOf<hkxEnvironment>();
}

// The type info's cleanup callback for hkxEnvironment::Variable: destroys the variable's value, then its name.
void cleanupLoadedObjecthkxEnvironmentVariable(void* p) {
    hkxEnvironment::Variable* pVariable = static_cast<hkxEnvironment::Variable*>(p);
    pVariable->m_value.~hkStringPtr();
    pVariable->m_name.~hkStringPtr();
}

void finishLoadedObjecthkxEnumItem(void* p, int32_t finishing) {
    if (p) {
        ::new (static_cast<hkStringPtr*>(p) + 1) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
    }
}

// The type info's cleanup callback for hkxEnum::Item: runs the item's destructor, which destroys its name.
void cleanupLoadedObjecthkxEnumItem(void* p) {
    static_cast<hkxEnum::Item*>(p)->m_name.~hkStringPtr();
}

const void* getVtablehkxSparselyAnimatedEnum() {
    return hkGetVtableOf<hkxSparselyAnimatedEnum>();
}

// The type info's finish-loaded-object callback for hkxEnum: its finish constructor, when the object is not null.
void finishLoadedObjecthkxEnum(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxEnum(flag);
    }
}

const void* getVtablehkxEnum() {
    return hkGetVtableOf<hkxEnum>();
}

// The type info's finish-loaded-object callback for hkxAttributeGroup: its finish constructor finishes the name (the
// attribute array's finish constructor is empty).
void finishLoadedObjecthkxAttributeGroup(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (&static_cast<hkxAttributeGroup*>(p)->m_name) hkStringPtr(flag);
    }
}

// The type info's cleanup callback for hkxAttribute: does the work of the attribute's destructor, releasing its value,
// then destroying its name.
void cleanupLoadedObjecthkxAttribute(void* a1) {
    hkxAttribute* pAttribute = static_cast<hkxAttribute*>(a1);
    // EXE-INLINED 0xC6CCA0
    if (pAttribute->m_value.m_pntr) {
        pAttribute->m_value.m_pntr->removeReference();
    }
    pAttribute->m_value.m_pntr = nullptr;
    pAttribute->m_name.~hkStringPtr();
}

// The cleanup function the reflection uses for hkxAttributeGroup: destroys the loaded group in place.
void cleanupLoadedObjecthkxAttributeGroup(void* p) {
    static_cast<hkxAttributeGroup*>(p)->~hkxAttributeGroup();
}

void cleanupLoadedObjecthkPackfileHeader(void* p) {
    ;
}

// The type info's finish-loaded-object callback for hkTraceStream: its finish constructor, when the object is not
// null.
void finishLoadedObjecthkTraceStream(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkTraceStream(flag);
    }
}

const void* getVtablehkTraceStream() {
    return hkGetVtableOf<hkTraceStream>();
}

void cleanupLoadedObjecthkpBallAndSocketConstraintData(void* p) {
    static_cast<hkpBallAndSocketConstraintData*>(p)->~hkpBallAndSocketConstraintData();
}

void cleanupLoadedObjecthkxVertexBuffer(void* p) {
    static_cast<hkxVertexBuffer*>(p)->~hkxVertexBuffer();
}

void cleanupLoadedObjecthkxEnvironment(void* p) {
    static_cast<hkxEnvironment*>(p)->~hkxEnvironment();
}

// The type info's finish-loaded-object callback for hkxSparselyAnimatedEnum: its finish constructor, when the
// object is not null.
void finishLoadedObjecthkxSparselyAnimatedEnum(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxSparselyAnimatedEnum(flag);
    }
}

// The type info's vtable getter for hkpListShape: finish-constructs one on the stack, not finishing, and returns its
// vtable.
const void* getVtablehkpListShape() {
    return hkGetVtableOf<hkpListShape>();
}

// The finish function the reflection registers for hkxVertexDescription: finishes the loaded object
// with its finish constructor, when the object is not null.
void finishLoadedObjecthkxVertexDescription(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxVertexDescription(flag);
    }
}

// The finish function the reflection registers for hkxAttribute: finishes the loaded object with
// its finish constructor, when the object is not null.
void finishLoadedObjecthkxAttribute(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxAttribute(flag);
    }
}

// The finish function the reflection registers for hkMonitorStreamStringMap::StringMap: finishes
// the loaded object with its finish constructor, when the object is not null.
void finishLoadedObjecthkMonitorStreamStringMapStringMap(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkMonitorStreamStringMap::StringMap(flag);
    }
}

const hkClass& hkxAttributeGroup::staticClass() {
    return hkxAttributeGroupClass;
}

// The loaded variables are kept as they are.
hkxEnvironment::hkxEnvironment(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag), m_variables(flag) {}

const hkClass& hkPackfileHeader::staticClass() {
    return hkPackfileHeaderClass;
}

// The loaded members are kept as they are.
hkxVertexBuffer::hkxVertexBuffer(hkFinishLoadedObjectFlag flag)
  : hkReferencedObject(flag), m_data(flag), m_desc(flag) {}

// The type info's finish-loaded-object callback for hkpCompressedMeshShape: finishes the loaded object with its finish
// constructor, when the object is not null.
void finishLoadedObjecthkpCompressedMeshShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpCompressedMeshShape(flag);
    }
}

// The finish function the reflection uses for hkpBallSocketChainData: finishes the loaded object with its finish
// constructor, when the object is not null.
void finishLoadedObjecthkpBallSocketChainData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpBallSocketChainData(flag);
    }
}

hkpBallSocketChainData::hkpBallSocketChainData(hkFinishLoadedObjectFlag flag)
  : hkpConstraintChainData(flag), m_infos(flag) {
    if (flag.m_finishing) {
        m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
        m_atoms.m_bridgeAtom.init(this);
    }
}

// The finish function the reflection registers for hkMonitorStreamStringMap: finishes the loaded object with its
// finish constructor, when the object is not null.
void finishLoadedObjecthkMonitorStreamStringMap(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkMonitorStreamStringMap(flag);
    }
}

// The finish function for hkpSimpleMeshShape: finishes the loaded object with its finish constructor, when the object
// is not null.
void finishLoadedObjecthkpSimpleMeshShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpSimpleMeshShape(flag);
    }
}

// The loaded arrays stay as loaded; a finished object is a simple mesh again.
hkpSimpleMeshShape::hkpSimpleMeshShape(hkFinishLoadedObjectFlag flag)
  : hkpShapeCollection(flag), m_vertices(flag), m_triangles(flag), m_materialIndices(flag) {
    if (flag.m_finishing) {
        m_type.m_storage = HK_SHAPE_TRIANGLE_COLLECTION;
        m_collectionType.m_storage = hkpShapeCollection::COLLECTION_SIMPLE_MESH;
    }
}

hkpConvexVerticesShape::hkpConvexVerticesShape(hkFinishLoadedObjectFlag flag)
  : hkpConvexShape(flag), m_rotatedVertices(flag), m_planeEquations(flag) {
    if (flag.m_finishing) {
        m_type.m_storage = HK_SHAPE_CONVEX_VERTICES;
    }
}

// The finish function the reflection registers for hkpConvexVerticesShape: finishes the loaded object with its finish
// constructor, when the object is not null.
void finishLoadedObjecthkpConvexVerticesShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpConvexVerticesShape(flag);
    }
}

// The loaded array is kept as it is.
hkxVertexDescription::hkxVertexDescription(hkFinishLoadedObjectFlag flag) : m_decls(flag) {}
