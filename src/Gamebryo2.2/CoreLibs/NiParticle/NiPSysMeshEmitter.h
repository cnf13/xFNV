#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"

#include <stdint.h>

class NiCloningProcess;
class NiGeometry;
class NiSkinInstance;
class NiStream;

// Maps the unit random of a face-surface emission.
float FastSqrt(float afValue);
// The NiPointer<NiGeometry> arrays' new[].
NiPointer<NiGeometry>* NewGeometryPointerArray(uint32_t numObjs);
// The other NiPointer arrays' shared new[]: the same
// code as NewGeometryPointerArray with another default constructor copy; unsigned count, as there.
NiPointer<NiObject>* NewObjectPointerArray(uint32_t numObjs);

// emits particles from the surface of meshes.
class NiPSysMeshEmitter : public NiPSysEmitter {
public:
    // where on the mesh a particle starts.
    enum MeshEmissionType {
        NI_EMIT_FROM_VERTICES = 0x0,
        NI_EMIT_FROM_FACE_CENTER = 0x1,
        NI_EMIT_FROM_EDGE_CENTER = 0x2,
        NI_EMIT_FROM_FACE_SURFACE = 0x3,
        NI_EMIT_FROM_EDGE_SURFACE = 0x4,
        NI_EMIT_MAX = 0x5,
    };

    // how the starting velocity is chosen.
    enum InitialVelocityType {
        NI_VELOCITY_USE_NORMALS = 0x0,
        NI_VELOCITY_USE_RANDOM = 0x1,
        NI_VELOCITY_USE_DIRECTION = 0x2,
        NI_VELOCITY_MAX = 0x3,
    };

    // the skin partition of one skinned emitter mesh.
    class NiSkinnedEmitterData : public NiRefObject {
    public:
        NiSkinnedEmitterData();
        ~NiSkinnedEmitterData() override;

        // Builds the skin partition used to emit from the skinned mesh; false when it could not be built.
        bool CreatePartition(NiGeometry* pkGeom);

        NiPointer<NiSkinPartition> m_spSkinPartition;
    };

    NiPSysMeshEmitter();
    ~NiPSysMeshEmitter() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void ComputeInitialPositionAndVelocity(NiPoint3& kPosition, NiPoint3& kVelocity) override;

    // Appends the mesh (ignoring null) with a skinned emitter data entry that shares the mesh's skin partition, and
    // keeps the mesh data's CPU copies.
    void AddGeometryEmitter(NiGeometry* pkGeomEmitter);
    // The number of mesh slots in use.
    uint32_t GetGeometryEmitterCount();
    // The mesh in slot uiWhichEmitter, or null past the count.
    NiGeometry* GetGeometryEmitter(uint32_t uiWhichEmitter);

protected:
    // Each picks a point on the mesh and the matching velocity; the skinned versions deform it with the skin first.
    // The result says whether a point was found and is ignored by the caller.
    bool EmitFromVertex(NiGeometry* pkGeomEmitter, NiPoint3& kPosition, NiPoint3& kVelocity);
    bool EmitFromFace(NiGeometry* pkGeomEmitter, NiPoint3& kPosition, NiPoint3& kVelocity);
    bool EmitFromEdge(NiGeometry* pkGeomEmitter, NiPoint3& kPosition, NiPoint3& kVelocity);
    bool EmitFromSkinnedVertex(
        NiSkinnedEmitterData* pkSkinEmitterData,
        NiGeometry* pkGeomEmitter,
        NiPoint3& kPosition,
        NiPoint3& kVelocity
    );
    bool EmitFromSkinnedFace(
        NiSkinnedEmitterData* pkSkinEmitterData,
        NiGeometry* pkGeomEmitter,
        NiPoint3& kPosition,
        NiPoint3& kVelocity
    );
    bool EmitFromSkinnedEdge(
        NiSkinnedEmitterData* pkSkinEmitterData,
        NiGeometry* pkGeomEmitter,
        NiPoint3& kPosition,
        NiPoint3& kVelocity
    );
    // The skin-deformed vertex and normal of index uiWhichIndex of pkPart.
    void DeformBySkin(
        NiPoint3& kVert,
        NiPoint3& kNormal,
        NiTriBasedGeom* pkGeom,
        NiSkinInstance* pkSkin,
        NiSkinPartition::Partition* pkPart,
        uint32_t uiWhichIndex
    );
    // Moves kPosition and kVelocity from pkObj's space into the emitter's.
    void TransformIntoCoordinateSpace(NiAVObject* pkObj, NiPoint3& kPosition, NiPoint3& kVelocity);

public:
    static NiObject* CreateObject();

    NiTObjectArray<NiPointer<NiGeometry>> m_kGeomEmitterArray;
    NiTObjectArray<NiPointer<NiSkinnedEmitterData>> m_kSkinnedEmitterData;
    InitialVelocityType m_eInitVelocityType;
    MeshEmissionType m_eEmissionType;
    NiPoint3 m_kEmitAxis;

    static NiRTTI ms_RTTI;
};
