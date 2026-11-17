#include "BSHavok/bhkShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkMigrationUtilFunctions.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "SDK/Windows/winbase.h"

// The name of each HavokMaterialType.
using MaterialNameArray = const char* [BHK_MATERIAL_MAX];
static MaterialNameArray pMaterialNameA{
    "STONE",
    "CLOTH",
    "DIRT",
    "GLASS",
    "GRASS",
    "METAL",
    "ORGANIC",
    "SKIN",
    "WATER",
    "WOOD",
    "HEAVY STONE",
    "HEAVY METAL",
    "HEAVY WOOD",
    "CHAIN",
    "BOTTLECAP",
    "ELEVATOR",
    "HOLLOW METAL",
    "SHEET METAL",
    "SAND",
    "BROKEN CONCRETE",
    "VEHICLE BODY",
    "VEHICLE PART SOLID",
    "VEHICLE PART HOLLOW",
    "BARREL",
    "BOTTLE",
    "SODA CAN",
    "PISTOL",
    "RIFLE",
    "SHOPPING CART",
    "LUNCHBOX",
    "BABY RATTLE",
    "RUBBER BALL",
    "CHAINLINK",
    "TILE",
    "CARPET",
    "TUMBLEWEED"
};
using MaterialNameTail = uint32_t[2];
static MaterialNameTail data_11B09D0{0x40DFF8D6u, 0u};

// The debug display color of each HavokMaterialType.
using kMaterialColorsArray = NiColor[BHK_MATERIAL_MAX];
static kMaterialColorsArray kMaterialColors{
    {0.65f, 0.65f, 0.55f}, {1.0f, 1.0f, 0.5f},    {0.81f, 0.73f, 0.47f}, {0.5f, 0.86f, 0.86f},  {0.0f, 1.0f},
    {0.43f, 0.55f, 0.56f}, {1.0f, 0.5f},          {1.0f, 0.74f, 0.65f},  {0.0f, 0.67f, 0.98f},  {0.81f, 0.6f, 0.4f},
    {0.45f, 0.45f, 0.35f}, {0.23f, 0.35f, 0.36f}, {0.61f, 0.4f, 0.2f},   {0.0f, 1.0f},          {1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 1.0f},    {0.43f, 0.55f, 0.56f}, {0.43f, 0.55f, 0.56f}, {0.81f, 0.73f, 0.47f}, {0.45f, 0.45f, 0.35f},
    {0.2f, 0.2f, 0.2f},    {0.2f, 0.3f, 0.3f},    {0.3f, 0.3f, 0.2f},    {0.1f, 0.1f, 0.2f},    {0.2f, 0.1f, 0.1f},
    {0.1f, 0.3f, 0.1f},    {0.1f, 0.2f, 0.2f},    {0.2f, 0.2f, 0.2f},    {0.25f, 0.15f, 0.25f}, {0.25f, 0.25f, 0.15f},
    {0.95f, 1.0f, 0.95f},  {1.0f, 0.62f, 0.47f}
};
NiRTTI bhkShape::ms_RTTI("bhkShape", &bhkSerializable::ms_RTTI);

bhkShape::bhkShape() : eMaterial(BHK_MATERIAL_STONE) {
    ++iCount;
}

bhkShape::~bhkShape() {
    bhkSerializable::Kill();
    --iCount;
}

// The base's data, then the material type, carried through the stream in the creation info's user data word (swapped
// when the source is big-endian; a type past the last material reads as stone) and cleared from it afterwards. The info
// CreateCinfo may make is deliberately not killed, as the game does.
void bhkShape::LoadBinary(NiStream& arStream) {
    bhkSerializable::LoadBinary(arStream);
    bool bCreated;
    bhkSerializable* pThis = this;
    hkShapeCinfo& rInfo = static_cast<hkShapeCinfo&>(pThis->CreateCinfo(bCreated));
    // Compares the flag's raw byte with 1: any other byte swaps.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        rInfo.m_userData = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(rInfo.m_userData)));
    }
    if (rInfo.m_userData >= BHK_MATERIAL_MAX) {
        rInfo.m_userData = 0;
    }
    eMaterial = static_cast<HavokMaterialType>(rInfo.m_userData);
    rInfo.m_userData = 0;
}

void bhkShape::LinkObject(NiStream& arStream) {
    bhkSerializable::LinkObject(arStream);
}

bool bhkShape::RegisterStreamables(NiStream& arStream) {
    return bhkSerializable::RegisterStreamables(arStream);
}

// The material type goes into the creation info's user data word (swapped around the base's write when the stream
// saves big-endian), then the info is killed.
void bhkShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkShapeCinfo& rInfo = static_cast<hkShapeCinfo&>(pThis->CreateCinfo(bCreated));
    uint32_t uiMaterial = eMaterial & BHK_MATERIAL_RUBBERBALL;
    rInfo.m_userData = static_cast<int32_t>(uiMaterial);
    // Compares the flag's raw byte with 1: any other byte swaps.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rInfo.m_userData = static_cast<int32_t>(__builtin_bswap32(uiMaterial));
    }
    bhkSerializable::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rInfo.m_userData = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(rInfo.m_userData)));
    }
    pThis->KillCinfo(bCreated);
}

// The base's test, then the two shapes' material types (their flag bits left out).
bool bhkShape::IsEqual(NiObject* apObject) {
    bool bEqual = bhkSerializable::IsEqual(apObject);
    if (!bEqual) {
        return bEqual;
    }
    bhkShape* pOther = static_cast<bhkShape*>(apObject);
    return ((pOther->eMaterial ^ eMaterial) & BHK_MATERIAL_RUBBERBALL) == 0;
}

// The shape's viewer lines.
void bhkShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v12 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v12);
    char* ViewerString_5 = NiGetViewerString("MATERIAL", eMaterial & BHK_MATERIAL_RUBBERBALL);
    uint32_t v9 = apStrings->m_usSize;
    uint32_t v10 = apStrings->m_usMaxSize;
    v12 = ViewerString_5;
    if (v9 >= v10) {
        apStrings->SetSize(v9 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v9, v12);
    v3->eMemContext = eMemContext;
}

// The Havok shape's user data points back at this wrapper while it is linked, and is cleared when it is unlinked.
// Null-checks this before it reads the field, as the game does.
void bhkShape::Link(bool abAttach) {
    if (this) {
        hkpShape* pShape = static_cast<hkpShape*>(phkObject);
        if (pShape) {
            pShape->m_userData = abAttach ? this : nullptr;
        }
    }
    bhkRefObject::Link(abAttach);
}

// The base's members, then the material type (its flag bits left out) into the copy.
void bhkShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bhkSerializable::CopyMembers(apDest, arCloneData);
    static_cast<bhkShape*>(apDest)->eMaterial = static_cast<HavokMaterialType>(eMaterial & BHK_MATERIAL_RUBBERBALL);
}

// Adds the wrapped shape's mass properties to arMassProperty; returns HK_FAILURE when there is no wrapped shape and
// HK_SUCCESS when it did.
hkResult bhkShape::CalcMass(hkpMassProperties& arProp) {
    if (!this) {
        return HK_FAILURE;
    }
    hkpShape* pkShape = static_cast<hkpShape*>(phkObject);
    if (!pkShape) {
        return HK_FAILURE;
    }
    hkpInertiaTensorComputer::computeShapeVolumeMassProperties(pkShape, arProp.m_mass, arProp);
    return HK_SUCCESS;
}

// A clone may share the shape when it is not rescaled: the clone process is always a BSCloningProcess, and its Scale is
// compared with (1, 1, 1) component by component.
bool bhkShape::CanShare(NiCloningProcess& arCloneData) {
    const NiPoint3& kScale = static_cast<BSCloningProcess&>(arCloneData).Scale;
    return kScale.x == NiPoint3_UNIT_ALL.x && kScale.y == NiPoint3_UNIT_ALL.y && kScale.z == NiPoint3_UNIT_ALL.z;
}

// Builds a debug display NiTriShape from the Havok geometry: its vertices converted to game units, its triangles'
// indices narrowed to 16 bits; names it apName when one is given and hangs it under apNode in the shape's material
// colour, all under the Havok heap context, restored after. A geometry with no vertices or no triangles builds nothing.
void bhkShape::BuildDisplayFromhkGeometry(
    NiNode* apNode,
    hkGeometry* apGeom,
    const NiColor& arDefColor,
    const char* apName
) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if (apGeom) {
        uint32_t uiVertices = apGeom->m_vertices.m_size;
        uint32_t uiTriangles = apGeom->m_triangles.m_size;
        if (uiVertices && uiTriangles) {
            // Array sizes are overflow-checked: a byte count that overflows asks for SIZE_MAX bytes. The index count
            // (three per triangle) wraps unchecked, as the game does; only its doubling is checked.
            NiPoint3* pVertices = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
                uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * uiVertices : SIZE_MAX
            ));
            uint32_t uiIndices = 3 * uiTriangles;
            uint16_t* pusTriList = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
                uiIndices <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiIndices : SIZE_MAX
            ));
            for (uint32_t i = 0; i < uiVertices; ++i) {
                HK2NI(pVertices[i], apGeom->m_vertices.m_data[i]);
            }
            for (uint32_t i = 0; i < uiTriangles; ++i) {
                const hkGeometry::Triangle& rTriangle = apGeom->m_triangles.m_data[i];
                pusTriList[3 * i] = static_cast<uint16_t>(rTriangle.m_a);
                pusTriList[3 * i + 1] = static_cast<uint16_t>(rTriangle.m_b);
                pusTriList[3 * i + 2] = static_cast<uint16_t>(rTriangle.m_c);
            }
            void* pMemory = NiMemObject::operator new(sizeof(NiTriShape));
            NiTriShape* pShape = pMemory ? ::new (pMemory) NiTriShape(
                                               uiVertices, pVertices, nullptr, nullptr, nullptr, 0,
                                               NiGeometryData::NBT_METHOD_NONE, uiTriangles, pusTriList
                                           )
                                         : nullptr;
            if (apName) {
                FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
                // EXE-INLINED 0x438170: NiFixedString::NiFixedString(const char*)
                kName.SetHandle(NiGlobalStringTable::AddString(apName));
                pShape->SetName(kName);
                // EXE-INLINED 0x4381B0: NiFixedString::~NiFixedString. The handle's reference count is the dword 8
                // bytes before its characters, raw memory the exe treats as untyped.
                if (kName.GetHandle() != NiGlobalStringTable::NULL_STRING) {
                    InterlockedDecrementUInt32(
                        reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName.GetHandle()) - 8)
                    );
                }
            }
            pShape->m_spModelData.m_pObject->CreateNormals(true);
            bhkMigrationUtilFunctions::PrepareDebugGeom(
                pShape, apNode, static_cast<HavokMaterialType>(eMaterial & BHK_MATERIAL_RUBBERBALL), &arDefColor
            );
        }
    }
    pTLSData->eMemContext = eMemContext;
}

void bhkShape::CopyCinfo(hkShapeCinfo& arInfo) {
    hkpShape* phkObject;
    if (this && (phkObject = static_cast<hkpShape*>(this->phkObject)) != 0) {
        // hkShapeCinfo::m_userData is an int; the shape's user data is its bhkShape.
        arInfo.m_userData = static_cast<int32_t>(reinterpret_cast<uintptr_t>(phkObject->m_userData));
    }
    else {
        arInfo.m_userData = 0;
    }
}

// The wrapped Havok shape, or null for a null this. The shared getter takes a non-const this.
hkpShape* bhkShape::operator*() const {
    if (this) {
        return const_cast<bhkShape*>(this)->Gethk();
    }
    else {
        return nullptr;
    }
}

HavokMaterialType bhkShape::GetSubshapeMaterial(uint32_t aiKey) {
    if (aiKey == ((-1))) {
        return static_cast<HavokMaterialType>(((int32_t)eMaterial) & 31);
    }
    bhkShapeCollection* v3 = FindShapeCollection();
    if (!v3) {
        return static_cast<HavokMaterialType>(((int32_t)eMaterial) & 31);
    }
    else {
        return v3->QSubMaterial(aiKey);
    }
}

HavokMaterialType bhkShape::GetMaterial() {
    return static_cast<HavokMaterialType>(eMaterial & BHK_MATERIAL_RUBBERBALL);
}

// Walks the shape's collection chain while the key is set (a -1 key keeps this shape), stopping at the first shape with
// no collection or whose collection is the shape itself, then reports whether that shape's material carries any bit of
// aeType.
bool bhkShape::GetSubMaterialVarType(uint32_t aiKey, MATERIAL_VARIANT_TYPE aeType) {
    bhkShape* pkShape = this;
    if (aiKey != 0xFFFFFFFFu) {
        for (;;) {
            bhkShapeCollection* pkCollection = pkShape->FindShapeCollection();
            if (!pkCollection || static_cast<bhkShape*>(pkCollection) == pkShape) {
                break;
            }
            pkShape = pkCollection;
        }
    }
    const bool bHasBits = (static_cast<int32_t>(pkShape->eMaterial) & aeType) != 0;
    return bHasBits == aeType;
}

bhkShape* bhkShape::Getbhk(const hkpShape* apShape) {
    if (apShape) {
        return apShape->m_userData;
    }
    else {
        return 0;
    }
}

void bhkShape::GetMaterialColor(HavokMaterialType aeType, NiColor& arColor) {
    arColor = kMaterialColors[aeType];
}

hkShapeCinfo::hkShapeCinfo() : m_userData(0) {}
