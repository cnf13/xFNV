#include "BSHavok/bhkBvTreeShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"

NiRTTI bhkBvTreeShape::ms_RTTI("bhkBvTreeShape", &bhkShape::ms_RTTI);

// Reads a link ID, the shape's data, and clears the creation info's shape collection: the link pass fills it in.
void bhkBvTreeShape::LoadBinary(NiStream& arStream) {
    arStream.ReadLinkID();
    bhkShape::LoadBinary(arStream);
    bool bCreated;
    hkBvTreeShapeCinfo* pCinfo = &static_cast<hkBvTreeShapeCinfo&>(CreateCinfo(bCreated));
    pCinfo->m_shapeCollection = nullptr;
}

void bhkBvTreeShape::LinkObject(NiStream& arStream) {
    bool bCreated;
    hkBvTreeShapeCinfo* pCinfo = &static_cast<hkBvTreeShapeCinfo&>(CreateCinfo(bCreated));
    bhkShapeCollection* psc = static_cast<bhkShapeCollection*>(arStream.GetObjectFromLinkID());
    if (psc) {
        pCinfo->m_shapeCollection = static_cast<hkpShapeCollection*>(psc->phkObject);
    }
    bhkShape::LinkObject(arStream);
}

// The collection's wrapper registers its streamables first, then the base's. The shape collection is found through the
// Havok tree shape's container, cast to the collection, and its user data.
bool bhkBvTreeShape::RegisterStreamables(NiStream& arStream) {
    bhkShapeCollection* psc = GetShapeCollection();
    if (psc) {
        psc->RegisterStreamables(arStream);
    }
    return bhkShape::RegisterStreamables(arStream);
}

// Writes the link ID of the collection's wrapper (null when there is no collection), then the base's data.
void bhkBvTreeShape::SaveBinary(NiStream& arStream) {
    bhkShapeCollection* psc = GetShapeCollection();
    arStream.SaveLinkID(psc);
    bhkShape::SaveBinary(arStream);
}

bool bhkBvTreeShape::IsEqual(NiObject* apObject) {
    return bhkShape::IsEqual(apObject);
}

// The base's strings, the class name, then the collection's wrapper's strings, under the Havok memory context.
void bhkBvTreeShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShape::GetViewerStrings(apStrings);
    char* pViewerString = NiGetViewerString(bhkBvTreeShape::ms_RTTI.GetName());
    apStrings->Add(pViewerString);
    bhkShapeCollection* psc = GetShapeCollection();
    if (psc) {
        psc->GetViewerStrings(apStrings);
    }
}

// Attaching takes a reference on the collection's wrapper (the shape the tree is built over), detaching drops it, then
// the base links.
void bhkBvTreeShape::Link(bool abAttach) {
    if (abAttach) {
        bhkShapeCollection* psc = GetShapeCollection();
        if (psc) {
            psc->IncRefCount();
        }
    }
    else {
        RembhkShapeRef();
    }
    bhkShape::Link(abAttach);
}

// The destination's creation info gets a clone of the collection's shape unless the shape says the clone may share it,
// then the base's members are copied.
void bhkBvTreeShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkBvTreeShapeCinfo* pCinfo = &static_cast<hkBvTreeShapeCinfo&>(CreateCinfo(bCreated));
    hkpShapeCollection* pCollection = pCinfo->m_shapeCollection;
    if (pCollection) {
        bhkShape* pShape = pCollection->m_userData;
        if (pShape && (!pShape->CanShare(arCloneData))) {
            bhkShape* pClone = static_cast<bhkShape*>(pShape->CreateClone(arCloneData));
            pCinfo->m_shapeCollection = pClone ? static_cast<hkpShapeCollection*>(pClone->phkObject) : nullptr;
        }
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

bhkShapeCollection* bhkBvTreeShape::FindShapeCollection() {
    return static_cast<bhkShapeCollection*>(GetShapeCollection());
}

// The collection's wrapper answers when there is one, else the base.
bool bhkBvTreeShape::CanShare(NiCloningProcess& arCloneData) {
    bhkShapeCollection* psc = GetShapeCollection();
    if (psc) {
        return psc->CanShare(arCloneData);
    }
    return bhkShape::CanShare(arCloneData);
}

// The collection's wrapper builds the display geometry.
void bhkBvTreeShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    bhkShapeCollection* psc = GetShapeCollection();
    if (psc) {
        psc->BuildDisplayGeometry(apNode, arDefColor);
    }
}

// The collection's wrapper drops its display geometry.
void bhkBvTreeShape::DestroyDisplayGeometry() {
    bhkShapeCollection* psc = GetShapeCollection();
    if (psc) {
        psc->DestroyDisplayGeometry();
    }
}

bhkBvTreeShape::~bhkBvTreeShape() {
    bhkSerializable::Kill();
    --iCount;
}

// Copies the base's creation info, then the Havok tree's collection (its container, cast back to the collection).
void bhkBvTreeShape::CopyCinfo(hkBvTreeShapeCinfo& arInfo) {
    bhkShape::CopyCinfo(arInfo);
    arInfo.m_shapeCollection = GethkShapeCollection();
}

// Drops the tree's reference to its collection's wrapper under the Havok heap context.
void bhkBvTreeShape::RembhkShapeRef() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShapeCollection* psc = GetShapeCollection();
    if (psc) {
        psc->DecRefCount();
    }
}

hkBvTreeShapeCinfo::hkBvTreeShapeCinfo() : m_shapeCollection(nullptr) {}
