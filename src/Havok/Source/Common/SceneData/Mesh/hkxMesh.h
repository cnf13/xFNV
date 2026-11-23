#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"

#include <stdint.h>

class hkGeometry;
class hkxMaterial;
class hkxMeshSection;

// A scene mesh: its sections and the descriptions of its user channels.
class hkxMesh : public hkReferencedObject {
public:
    // The name and class of one user channel of the mesh's vertex buffers.
    class UserChannelInfo : public hkxAttributeHolder {
    public:
        // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
        UserChannelInfo(hkFinishLoadedObjectFlag aFlag)
          : hkxAttributeHolder(aFlag), m_name(aFlag), m_className(aFlag) {}

        ~UserChannelInfo() override;

        static const hkClass& staticClass();

        hkStringPtr m_name;
        hkStringPtr m_className;
    };

    // Implicit default constructor: the getVtable function declares the local, then constructs it again in place. Every
    // member's own default leaves it destructible.
    hkxMesh() = default;
    hkxMesh(hkFinishLoadedObjectFlag aFlag);
    ~hkxMesh() override;

    // Appends the position of every vertex of every section to arVerticesInOut.
    void collectVertexPositions(hkArray<hkVector4>& verticesInOut) const;
    // Appends the triangles of every section to arGeometryInOut, and the sections' materials to apMaterialsInOut
    // when it is given.
    void appendGeometry(hkGeometry& geometryInOut, hkArray<hkxMaterial*>* materialsInOut = nullptr) const;

    static const hkClass& staticClass();

    hkArray<hkRefPtr<hkxMeshSection>> m_sections;
    hkArray<hkRefPtr<UserChannelInfo>> m_userChannelInfos;
};

extern hkClass hkxMeshUserChannelInfoClass;
extern hkTypeInfo hkxMeshUserChannelInfoTypeInfo;
extern hkClass hkxMeshClass;
extern hkTypeInfo hkxMeshTypeInfo;
