#include "Gamebryo2.2/CoreLibs/NiMain/NiOldScreenSpace.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"

#include <cstdint>

NiRTTI NiScreenGeometryData::ms_RTTI("NiScreenGeometryData", &NiTriShapeData::ms_RTTI);
NiRTTI NiScreenGeometry::ms_RTTI("NiScreenGeometry", &NiTriShape::ms_RTTI);
NiRTTI NiScreenPolygon::ms_RTTI("NiScreenPolygon", &NiObject::ms_RTTI);
NiRTTI NiScreenSpaceCamera::ms_RTTI("NiScreenSpaceCamera", &NiCamera::ms_RTTI);

namespace {

    // The engine's new[] of a type without a constructor: the byte count saturates on overflow so the allocation fails
    // instead of coming back short.
    template <class T>
    T* NewArray(uint32_t auiCount) {
        uint64_t ulSize = static_cast<uint64_t>(sizeof(T)) * auiCount;
        return static_cast<T*>(
            MemoryManager::Instance().Allocate(ulSize > SIZE_MAX ? SIZE_MAX : static_cast<size_t>(ulSize))
        );
    }

} // namespace

// An empty triangle shape: a stream load sets its data.
NiScreenGeometry::NiScreenGeometry() {}

NiScreenGeometry::~NiScreenGeometry() {}

// NiScreenGeometry
const NiRTTI* NiScreenGeometry::GetRTTI() const {
    return &ms_RTTI;
}

// A new empty shape, then the members copied.
NiObject* NiScreenGeometry::CreateClone(NiCloningProcess& kCloning) {
    NiScreenGeometry* pkClone = new NiScreenGeometry;
    // Calls BSResizableTriShape::CopyMembers on the same object, a class this one does not derive from;
    // its body only calls NiTriShape::CopyMembers.
    static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(this))
        ->CopyMembers(static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(pkClone)), kCloning);
    return pkClone;
}

void NiScreenGeometry::LoadBinary(NiStream& kStream) {
    NiTriShape::LoadBinary(kStream);
}

void NiScreenGeometry::LinkObject(NiStream& kStream) {
    NiTriShape::LinkObject(kStream);
}

bool NiScreenGeometry::RegisterStreamables(NiStream& kStream) {
    return NiTriShape::RegisterStreamables(kStream);
}

void NiScreenGeometry::SaveBinary(NiStream& kStream) {
    NiTriShape::SaveBinary(kStream);
}

bool NiScreenGeometry::IsEqual(NiObject* pkObject) {
    return NiTriShape::IsEqual(pkObject);
}

void NiScreenGeometry::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriShape::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// An empty triangle shape.
NiObject* NiScreenGeometry::CreateObject() {
    return new NiScreenGeometry;
}

// An empty element.
NiScreenGeometryData::ScreenElement::ScreenElement()
  : m_usNumVerts(0), m_usNumTexCoordSets(0), m_pkVerts(nullptr), m_pkColors(nullptr), m_pkTexCoords(nullptr) {}

// The element's three arrays are freed through MemoryManager.
NiScreenGeometryData::ScreenElement::~ScreenElement() {
    MemoryManager::Instance().Deallocate(m_pkVerts);
    MemoryManager::Instance().Deallocate(m_pkColors);
    MemoryManager::Instance().Deallocate(m_pkTexCoords);
}

void NiScreenGeometryData::ScreenElement::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

void NiScreenGeometryData::ScreenElement::operator delete[](void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

// NiTriShapeData's default constructor, then no flags, no active vertices and an empty element array of 4 slots
// growing by 4.
NiScreenGeometryData::NiScreenGeometryData()
  : m_bPixelAccurate(false), m_bUnorderedElements(false), m_usActiveVerts(0), m_kScreenElements(4, 4) {}

// Each element is taken out of the array, its arrays freed and the element returned to the pool (its destructor is
// not run); then the array is emptied, and the compiler frees its buffer.
NiScreenGeometryData::~NiScreenGeometryData() {
    for (uint32_t i = 0; i < m_kScreenElements.m_usSize; ++i) {
        ScreenElement* pkElement = m_kScreenElements[i];
        m_kScreenElements.SetAt(i, nullptr);
        if (pkElement) {
            MemoryManager::Instance().Deallocate(pkElement->m_pkVerts);
            MemoryManager::Instance().Deallocate(pkElement->m_pkColors);
            MemoryManager::Instance().Deallocate(pkElement->m_pkTexCoords);
            ms_pkScreenElementPool->ReleaseObject(pkElement);
        }
    }
    m_kScreenElements.SetSize(0);
}

// NiScreenGeometryData
const NiRTTI* NiScreenGeometryData::GetRTTI() const {
    return &ms_RTTI;
}

// Files before 10.0.1.15 go through NiGeometryData's loader plus the old triangle fields. Each element comes from the
// class's element pool and takes the loaded arrays, its old ones freed.
// KEEP: niread: the vertex and color flags are read from the stream into stack bytes nothing initialised (the
// pixel-accuracy flag into the stream pointer's argument slot); a short read leaves them unwritten and the body
// branches on them.
void NiScreenGeometryData::LoadBinary(NiStream& kStream) {
    if (kStream.GetFileVersion() >= 0x0A00010F) {
        NiTriShapeData::LoadBinary(kStream);
    }
    else {
        NiGeometryData::LoadBinary(kStream);
    }

    uint8_t ucPixelAccurate;
    NiStreamLoadBinary(kStream, ucPixelAccurate);
    m_bPixelAccurate = ucPixelAccurate != 0;

    if (kStream.GetFileVersion() < 0x0A00010F) {
        uint16_t usVertices;
        NiStreamLoadBinary(kStream, usVertices);
        m_usVertices = usVertices;
        NiStreamLoadBinary(kStream, m_usTriangles);
    }
    if (kStream.GetFileVersion() < 0x0A00010F) {
        uint16_t usTriListLength;
        NiStreamLoadBinary(kStream, usTriListLength);
        m_uiTriListLength = usTriListLength;
        if (usTriListLength) {
            m_pusTriList = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usTriListLength));
            NiStreamLoadBinary(kStream, m_pusTriList, usTriListLength);
        }
    }

    uint32_t uiNumElements = 0;
    NiStreamLoadBinary(kStream, uiNumElements);
    if (!uiNumElements) {
        return;
    }

    m_kScreenElements.SetSize(uiNumElements);
    for (uint32_t i = 0; i < uiNumElements; i++) {
        uint16_t usNumVerts = 0;
        NiStreamLoadBinary(kStream, usNumVerts);
        if (!usNumVerts) {
            continue;
        }

        uint16_t usNumTexCoordSets = 0;
        NiPoint2* pkVerts = nullptr;
        NiColorA* pkColors = nullptr;
        NiPoint2* pkTexCoords = nullptr;

        uint8_t ucVerts;
        NiStreamLoadBinary(kStream, ucVerts);
        if (ucVerts) {
            pkVerts = NewArray<NiPoint2>(usNumVerts);
            NiStreamLoadBinary(kStream, pkVerts, usNumVerts);
        }

        uint8_t ucColors;
        NiStreamLoadBinary(kStream, ucColors);
        if (ucColors) {
            NiColorA* pkNewColors = NewArray<NiColorA>(usNumVerts);
            if (pkNewColors) {
                ::new (static_cast<void*>(pkNewColors)) NiColorA[usNumVerts];
                pkColors = pkNewColors;
            }
            else {
                pkColors = nullptr;
            }
            NiStreamLoadBinary(kStream, pkColors, usNumVerts);
        }

        NiStreamLoadBinary(kStream, usNumTexCoordSets);
        if (usNumTexCoordSets) {
            uint32_t uiNumTexCoords = static_cast<uint32_t>(usNumVerts) * usNumTexCoordSets;
            pkTexCoords = NewArray<NiPoint2>(uiNumTexCoords);
            NiStreamLoadBinary(kStream, pkTexCoords, uiNumTexCoords);
        }

        ScreenElement* pkElement = ms_pkScreenElementPool->GetFreeObject();

        MemoryManager::Instance().Deallocate(pkElement->m_pkVerts);
        pkElement->m_usNumVerts = usNumVerts;
        pkElement->m_pkVerts = pkVerts;
        MemoryManager::Instance().Deallocate(pkElement->m_pkColors);
        pkElement->m_pkColors = pkColors;
        MemoryManager::Instance().Deallocate(pkElement->m_pkTexCoords);
        pkElement->m_pkTexCoords = pkTexCoords;
        pkElement->m_usNumTexCoordSets = usNumTexCoordSets;
        m_kScreenElements.SetAt(i, pkElement);
    }
}

void NiScreenGeometryData::LinkObject(NiStream& kStream) {
    NiTriShapeData::LinkObject(kStream);
}

bool NiScreenGeometryData::RegisterStreamables(NiStream& kStream) {
    return NiTriShapeData::RegisterStreamables(kStream);
}

// Writes the pixel-accuracy flag and the elements; an empty slot is written as a zero vertex count.
void NiScreenGeometryData::SaveBinary(NiStream& kStream) {
    NiTriShapeData::SaveBinary(kStream);
    // The flag goes to the stream from a stack copy.
    uint8_t ucPixelAccurate = m_bPixelAccurate;
    NiStreamSaveBinary(kStream, ucPixelAccurate);
    uint32_t uiNumElements = m_kScreenElements.m_usSize;
    NiStreamSaveBinary(kStream, uiNumElements);
    for (uint32_t i = 0; i < uiNumElements; i++) {
        ScreenElement* pkElement = m_kScreenElements.m_pBase[i];
        uint16_t usNumVerts = pkElement ? pkElement->m_usNumVerts : 0;
        NiStreamSaveBinary(kStream, usNumVerts);
        if (!usNumVerts) {
            continue;
        }

        NiPoint2* pkVerts = pkElement->m_pkVerts;
        NiColorA* pkColors = pkElement->m_pkColors;
        NiPoint2* pkTexCoords = pkElement->m_pkTexCoords;
        uint16_t usNumTexCoordSets = pkElement->m_usNumTexCoordSets;

        bool bVerts = pkVerts != nullptr;
        NiStreamSaveBinary(kStream, bVerts);
        if (bVerts) {
            for (uint32_t j = 0; j < usNumVerts; j++) {
                pkVerts[j].SaveBinary(kStream);
            }
        }

        bool bColors = pkColors != nullptr;
        NiStreamSaveBinary(kStream, bColors);
        if (bColors) {
            for (uint32_t j = 0; j < usNumVerts; j++) {
                pkColors[j].SaveBinary(kStream);
            }
        }

        NiStreamSaveBinary(kStream, usNumTexCoordSets);
        if (usNumTexCoordSets) {
            uint32_t uiNumTexCoords = static_cast<uint32_t>(usNumVerts) * usNumTexCoordSets;
            for (uint32_t j = 0; j < uiNumTexCoords; j++) {
                pkTexCoords[j].SaveBinary(kStream);
            }
        }
    }
}

// Walks the effective size (not the slot count) of the element arrays; vertex, color and texture coordinate arrays
// are compared without null checks once the counts match.
bool NiScreenGeometryData::IsEqual(NiObject* pkObject) {
    if (!NiTriShapeData::IsEqual(pkObject)) {
        return false;
    }

    NiScreenGeometryData* pkOther = static_cast<NiScreenGeometryData*>(pkObject);
    if (pkOther->m_bPixelAccurate != m_bPixelAccurate) {
        return false;
    }

    uint32_t uiNumElements = pkOther->m_kScreenElements.m_usESize;
    if (uiNumElements != m_kScreenElements.m_usESize ||
        pkOther->m_kScreenElements.m_usSize != m_kScreenElements.m_usSize) {
        return false;
    }

    for (uint32_t i = 0; i < uiNumElements; i++) {
        ScreenElement* pkOtherElement = pkOther->m_kScreenElements.m_pBase[i];
        ScreenElement* pkElement = m_kScreenElements.m_pBase[i];
        if (!pkOtherElement) {
            if (pkElement) {
                return false;
            }
            continue;
        }
        if (!pkElement) {
            return false;
        }

        uint32_t uiNumVerts = pkOtherElement->m_usNumVerts;
        uint32_t uiNumTexCoordSets = pkOtherElement->m_usNumTexCoordSets;
        if (uiNumVerts != pkElement->m_usNumVerts || uiNumTexCoordSets != pkElement->m_usNumTexCoordSets) {
            return false;
        }

        for (uint32_t j = 0; j < uiNumVerts; j++) {
            if (pkOtherElement->m_pkVerts[j] != pkElement->m_pkVerts[j]) {
                return false;
            }
            if (pkOtherElement->m_pkColors[j] != pkElement->m_pkColors[j]) {
                return false;
            }
            for (uint32_t k = 0; k < uiNumTexCoordSets; k++) {
                uint32_t uiIndex = j * uiNumTexCoordSets + k;
                if (pkOtherElement->m_pkTexCoords[uiIndex] != pkElement->m_pkTexCoords[uiIndex]) {
                    return false;
                }
            }
        }
    }
    return true;
}

// The title and the flag come after NiGeometryData's strings: NiTriShapeData's own are skipped.
void NiScreenGeometryData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiGeometryData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bPixelAccurate", m_bPixelAccurate));
    pkStrings->Add(NiGetViewerString("Num Elements", static_cast<uint32_t>(m_kScreenElements.m_usESize)));
}

// An empty screen geometry data, built by the class's constructor.
NiObject* NiScreenGeometryData::CreateObject() {
    return new NiScreenGeometryData;
}

// The element pool, made in place: an empty free list, no containers yet, and 8 as both the current and the initial
// container size.
void NiScreenGeometryData::_SDMInit() {
    ScreenElementPool* pkPool = static_cast<ScreenElementPool*>(
        MemoryManager::Instance().Allocate(sizeof(ScreenElementPool))
    );
    if (pkPool) {
        pkPool->m_kFreeObjects.m_pBase = nullptr;
        pkPool->m_kFreeObjects.m_uiAlloced = 0;
        pkPool->m_kFreeObjects.m_uiUsed = 0;
        pkPool->m_pkContainers = nullptr;
        pkPool->m_uiCurrentSize = 8;
        pkPool->m_uiInitialSize = 8;
    }
    ms_pkScreenElementPool = pkPool;
}

// Frees the element pool; the static pointer is deliberately left set.
void NiScreenGeometryData::_SDMShutdown() {
    ScreenElementPool* pkPool = ms_pkScreenElementPool;
    if (pkPool) {
        pkPool->~NiTObjectPool<NiScreenGeometryData::ScreenElement>();
        MemoryManager::Instance().Deallocate(pkPool);
    }
}

// Allocates uiCount elements as new[] does: the byte count saturates at UINT32_MAX on overflow, first of the elements'
// size, then with the count cookie added.
NiScreenGeometryData::ScreenElement* AllocateScreenElements(uint32_t uiCount) {
    uint64_t ulBytes = static_cast<uint64_t>(uiCount) * sizeof(NiScreenGeometryData::ScreenElement);
    uint32_t uiBytes = (ulBytes >> 32) != 0 ? UINT32_MAX : static_cast<uint32_t>(ulBytes);
    uint32_t uiTotal = uiBytes + sizeof(size_t) < uiBytes ? UINT32_MAX : uiBytes + sizeof(size_t);
    // The element count new[] stores just before the array (its cookie).
    size_t* puiCookie = static_cast<size_t*>(MemoryManager::Instance().Allocate(uiTotal));
    if (!puiCookie) {
        return nullptr;
    }
    *puiCookie = uiCount;
    // The untyped block after the cookie, where the elements are built.
    NiScreenGeometryData::ScreenElement* pkArray = reinterpret_cast<NiScreenGeometryData::ScreenElement*>(
        puiCookie + 1
    );
    for (int32_t i = 0; i < static_cast<int32_t>(uiCount); ++i) {
        ::new (static_cast<void*>(pkArray + i)) NiScreenGeometryData::ScreenElement;
    }
    return pkArray;
}

// An empty polygon. The property state is deliberately left unset: CreateClone and the stream load
// set it.
NiScreenPolygon::NiScreenPolygon() : m_usVertices(0), m_pkVertex(nullptr), m_pkTexture(nullptr), m_pkColor(nullptr) {}

// The polygon's arrays are freed through MemoryManager.
NiScreenPolygon::~NiScreenPolygon() {
    MemoryManager::Instance().Deallocate(m_pkVertex);
    MemoryManager::Instance().Deallocate(m_pkTexture);
    MemoryManager::Instance().Deallocate(m_pkColor);
}

// NiScreenPolygon
const NiRTTI* NiScreenPolygon::GetRTTI() const {
    return &ms_RTTI;
}

// A new empty polygon (its property state left unset), the members copied, then a copy of the property state of its
// own.
NiObject* NiScreenPolygon::CreateClone(NiCloningProcess& kCloning) {
    NiScreenPolygon* pkClone = new NiScreenPolygon;
    NiObject::CopyMembers(pkClone, kCloning);
    pkClone->m_spPropertyState = new NiPropertyState(*m_spPropertyState);
    return pkClone;
}

// Files before 4.1.0.0 store the texture and color arrays' old pointers (written into the members, then replaced) where
// newer ones store a flag. The property links are queued for LinkObject. The texture and color flags are not
// initialised before they are read; a short read leaves them unwritten and the body branches on them.
void NiScreenPolygon::LoadBinary(NiStream& stream) {
    NiObject::LoadBinary(stream);
    NiStreamLoadBinary(stream, m_usVertices);
    m_pkVertex = NewArray<NiPoint3>(m_usVertices);
    for (uint16_t i = 0; i < m_usVertices; i++) {
        m_pkVertex[i].LoadBinary(stream);
    }

    bool bTexture;
    if (stream.GetFileVersion() >= 0x4010000) {
        NiStreamLoadBinary(stream, bTexture);
    }
    else {
        NiStreamLoadBinary(stream, m_pkTexture);
        bTexture = m_pkTexture != nullptr;
    }
    if (bTexture) {
        m_pkTexture = NewArray<NiPoint2>(m_usVertices);
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pkTexture[i].LoadBinary(stream);
        }
    }

    bool bColor;
    if (stream.GetFileVersion() >= 0x4010000) {
        NiStreamLoadBinary(stream, bColor);
    }
    else {
        NiStreamLoadBinary(stream, m_pkColor);
        bColor = m_pkColor != nullptr;
    }
    if (bColor) {
        uint32_t uiCount = m_usVertices;
        NiColorA* pkColor = NewArray<NiColorA>(uiCount);
        if (pkColor) {
            for (uint32_t i = 0; i < uiCount; i++) {
                new (&pkColor[i]) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        m_pkColor = pkColor;
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pkColor[i].LoadBinary(stream);
        }
    }

    stream.ReadMultipleLinkIDs();
}

// Each linked property goes into its type's slot of the property state; other objects are ignored.
void NiScreenPolygon::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
    uint32_t uiCount = stream.GetNumberOfLinkIDs();
    while (uiCount) {
        uiCount--;
        NiProperty* pkProperty = static_cast<NiProperty*>(stream.GetObjectFromLinkID());
        if (pkProperty && pkProperty->Type() <= 7) {
            m_spPropertyState->m_aspProps[pkProperty->Type()] = pkProperty;
        }
    }
}

// The property slots are registered without a null check.
bool NiScreenPolygon::RegisterStreamables(NiStream& stream) {
    if (!NiObject::RegisterStreamables(stream)) {
        return false;
    }

    for (uint32_t i = 0; i < 7; i++) {
        m_spPropertyState->m_aspProps[i]->RegisterStreamables(stream);
    }
    return true;
}

void NiScreenPolygon::SaveBinary(NiStream& stream) {
    NiObject::SaveBinary(stream);
    NiStreamSaveBinary(stream, m_usVertices);
    for (uint16_t i = 0; i < m_usVertices; i++) {
        m_pkVertex[i].SaveBinary(stream);
    }

    bool bTexture = m_pkTexture != nullptr;
    NiStreamSaveBinary(stream, bTexture);
    if (bTexture) {
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pkTexture[i].SaveBinary(stream);
        }
    }

    bool bColor = m_pkColor != nullptr;
    NiStreamSaveBinary(stream, bColor);
    if (bColor) {
        for (uint32_t i = 0; i < m_usVertices; i++) {
            m_pkColor[i].SaveBinary(stream);
        }
    }

    uint32_t uiNumProps = 7;
    NiStreamSaveBinary(stream, uiNumProps);
    for (uint32_t i = 0; i < uiNumProps; i++) {
        stream.SaveLinkID(m_spPropertyState->m_aspProps[i]);
    }
}

// Vertices, texture coordinates and colors compare as floats (NaN is unequal); the properties by pointer.
bool NiScreenPolygon::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiScreenPolygon* pkOther = static_cast<NiScreenPolygon*>(pkObject);
    uint16_t usVertices = m_usVertices;
    if (usVertices != pkOther->m_usVertices) {
        return false;
    }

    for (uint16_t i = 0; i < usVertices; i++) {
        const NiPoint3& rkA = pkOther->m_pkVertex[i];
        const NiPoint3& rkB = m_pkVertex[i];
        if (rkA.x != rkB.x || rkA.y != rkB.y || rkA.z != rkB.z) {
            return false;
        }
    }

    if (m_pkTexture) {
        if (!pkOther->m_pkTexture) {
            return false;
        }
        for (uint16_t i = 0; i < usVertices; i++) {
            if (pkOther->m_pkTexture[i].x != m_pkTexture[i].x || pkOther->m_pkTexture[i].y != m_pkTexture[i].y) {
                return false;
            }
        }
    }
    else if (pkOther->m_pkTexture) {
        return false;
    }

    if (m_pkColor) {
        if (!pkOther->m_pkColor) {
            return false;
        }
        for (uint16_t i = 0; i < usVertices; i++) {
            const NiColorA& rkA = pkOther->m_pkColor[i];
            const NiColorA& rkB = m_pkColor[i];
            if (rkA.r != rkB.r || rkA.g != rkB.g || rkA.b != rkB.b || rkA.a != rkB.a) {
                return false;
            }
        }
    }
    else if (pkOther->m_pkColor) {
        return false;
    }

    for (uint16_t i = 0; i < 7; i++) {
        if (m_spPropertyState->m_aspProps[i] != pkOther->m_spPropertyState->m_aspProps[i]) {
            return false;
        }
    }
    return true;
}

// Each property is listed under its class name, without a null check.
void NiScreenPolygon::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_usVertices", m_usVertices));
    pkStrings->Add(NiGetViewerString("m_pkVertex", static_cast<const void*>(m_pkVertex)));
    pkStrings->Add(NiGetViewerString("m_pkTexture", static_cast<const void*>(m_pkTexture)));
    pkStrings->Add(NiGetViewerString("m_pkColor", static_cast<const void*>(m_pkColor)));
    for (uint32_t i = 0; i < 7; i++) {
        NiProperty* pkProperty = m_spPropertyState->m_aspProps[i];
        pkStrings->Add(NiGetViewerString(pkProperty->GetRTTI()->GetName(), static_cast<const void*>(pkProperty)));
    }
}

// An empty polygon.
NiObject* NiScreenPolygon::CreateObject() {
    return new NiScreenPolygon;
}

// Both screen arrays hold 5 slots and grow by 5; the camera is orthographic and its world data and bound are
// updated through NiCamera's own bodies.
NiScreenSpaceCamera::NiScreenSpaceCamera() : m_kScreenPolygons(5, 5), m_kScreenTextures(5, 5) {
    m_kViewFrustum.m_bOrtho = true;
    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    NiCamera::UpdateWorldData(kUpdateData);
    NiCamera::UpdateWorldBound();
}

// The object arrays release their elements and free their buffers themselves.
NiScreenSpaceCamera::~NiScreenSpaceCamera() {}

// NiScreenSpaceCamera
const NiRTTI* NiScreenSpaceCamera::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiScreenSpaceCamera::CreateClone(NiCloningProcess& kCloning) {
    NiScreenSpaceCamera* pkObject = new NiScreenSpaceCamera;
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

void NiScreenSpaceCamera::LoadBinary(NiStream& kStream) {
    NiCamera::LoadBinary(kStream);
}

// Appends a copy of every polygon and texture to its own array, then empties the array: the lists are never streamed,
// so after loading both end up empty. Add's element is a reference into the buffer its SetSize may reallocate.
void NiScreenSpaceCamera::LinkObject(NiStream& kStream) {
    NiCamera::LinkObject(kStream);

    uint32_t uiNumPolygons = m_kScreenPolygons.m_usSize;
    for (uint32_t i = 0; i < uiNumPolygons; i++) {
        m_kScreenPolygons.Add(m_kScreenPolygons.GetAt(i));
    }
    m_kScreenPolygons.RemoveAll();

    uint32_t uiNumTextures = m_kScreenTextures.m_usSize;
    for (uint32_t i = 0; i < uiNumTextures; i++) {
        m_kScreenTextures.Add(m_kScreenTextures.GetAt(i));
    }
    m_kScreenTextures.RemoveAll();
}

bool NiScreenSpaceCamera::RegisterStreamables(NiStream& kStream) {
    return NiCamera::RegisterStreamables(kStream);
}

void NiScreenSpaceCamera::SaveBinary(NiStream& kStream) {
    NiCamera::SaveBinary(kStream);
}

bool NiScreenSpaceCamera::IsEqual(NiObject* pkObject) {
    return NiCamera::IsEqual(pkObject);
}

void NiScreenSpaceCamera::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiCamera::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// NiAVObject's processing, then each polygon's and texture's.
void NiScreenSpaceCamera::ProcessClone(NiCloningProcess& kCloning) {
    NiCamera::ProcessClone(kCloning);
    for (NiScreenPolygon* pkPolygon : m_kScreenPolygons) {
        if (pkPolygon) {
            pkPolygon->ProcessClone(kCloning);
        }
    }
    for (NiScreenTexture* pkTexture : m_kScreenTextures) {
        if (pkTexture) {
            pkTexture->ProcessClone(kCloning);
        }
    }
}

// Copies NiCamera's members, then sizes each array as the source's and fills it with clones of its elements.
void NiScreenSpaceCamera::CopyMembers(NiScreenSpaceCamera* pkDest, NiCloningProcess& kCloning) {
    NiCamera::CopyMembers(pkDest, kCloning);

    pkDest->m_kScreenPolygons.SetSize(m_kScreenPolygons.m_usSize);
    pkDest->m_kScreenPolygons.m_usGrowBy = m_kScreenPolygons.m_usGrowBy;
    for (uint32_t i = 0; i < m_kScreenPolygons.m_usSize; i++) {
        NiPointer<NiScreenPolygon> spPolygon = m_kScreenPolygons.m_pBase[i];
        if (spPolygon) {
            NiPointer<NiScreenPolygon> spClone = static_cast<NiScreenPolygon*>(spPolygon->CreateClone(kCloning));
            pkDest->m_kScreenPolygons.SetAt(i, spClone);
        }
        else {
            NiPointer<NiScreenPolygon> spNull;
            pkDest->m_kScreenPolygons.SetAt(i, spNull);
        }
    }

    NiTObjectArray<NiPointer<NiScreenTexture>>* pkDestTextures = &pkDest->m_kScreenTextures;
    pkDestTextures->SetSize(m_kScreenTextures.m_usSize);
    pkDestTextures->m_usGrowBy = m_kScreenTextures.m_usGrowBy;
    for (uint32_t i = 0; i < m_kScreenTextures.m_usSize; i++) {
        NiPointer<NiScreenTexture> spTexture = m_kScreenTextures.m_pBase[i];
        if (spTexture) {
            NiPointer<NiScreenTexture> spClone = static_cast<NiScreenTexture*>(spTexture->CreateClone(kCloning));
            pkDestTextures->SetAt(i, spClone);
        }
        else {
            NiPointer<NiScreenTexture> spNull;
            pkDestTextures->SetAt(i, spNull);
        }
    }
}

NiObject* NiScreenSpaceCamera::CreateObject() {
    return new NiScreenSpaceCamera;
}
