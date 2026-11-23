#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"

class NiAVObject;
class NiCloningProcess;
class NiPoint3;
class NiStream;
class NiTransform;

// Tags an object as a point of a specific-objects particle array.
class PArrayPoint : public NiExtraData {
public:
    PArrayPoint();
    ~PArrayPoint() override;

    const NiRTTI* GetRTTI() const override {
        return &PArrayPoint::ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;

    // Creates the tag: interns "PAP" into a 4-byte MemoryManager block.
    static void InitSDM();
    // Allocates the PArrayPoint object itself, the way `NiNew PArrayPoint` does, so it takes no memory argument and is
    // written as a factory.
    static PArrayPoint* CreateInstance();
    static void KillSDM();

    static inline NiFixedString* pTag;
    static NiRTTI ms_RTTI;
};

// emits particles from the vertices, objects or nodes of an array.
class BSPSysArrayEmitter : public NiPSysVolumeEmitter {
public:
    // what the array holds.
    enum PARTICLEARRAYTYPE {
        PAT_VERTICES = 0x0,
        PAT_SPECIFICOBJS = 0x1,
        PAT_NODES = 0x2,
    };

    BSPSysArrayEmitter();
    BSPSysArrayEmitter(
        char* pcName,
        NiAVObject* uiOrder,
        int32_t aeType,
        float fSpeed,
        float fSpeedVar,
        float fDeclination,
        float fDeclinationVar,
        float fPlanarAngle,
        float fPlanarAngleVar,
        NiColorA aInitialColor,
        float fInitialRadius,
        float fLifeSpan,
        float fLifeSpanVar,
        float fRadiusVar
    );
    ~BSPSysArrayEmitter() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void ProcessClone(NiCloningProcess& arCloneData) override;
    void PostLinkObject(NiStream& arStream) override;
    void SetEmitterObj(NiAVObject* apEmitter) override;

protected:
    void ComputeVolumeInitialPositionAndVelocity(
        NiTransform& arEmitterToPSys,
        NiPoint3& arPosition,
        NiPoint3& arVelocity
    ) override;

public:
    // Refills the array from the emitter object's subtree (after updating it) with what the array type selects.
    void SetupArray();
    void CopyMembers(BSPSysArrayEmitter* apDest, NiCloningProcess& arCloneData);

protected:
    // Adds apObject when the array type selects it (unless it is the emitter object), then walks its children.
    void SetObjectPointers(NiAVObject* pObject);
    // A position (and, for some types, velocity) from a random array entry.
    void GetAPoint(NiPoint3& arPos, NiPoint3& arVel);
    // The object's position, or for nodes a random point between it and its parent; specific objects also rotate
    // the velocity into the object's frame.
    void GetPointFromObject(NiAVObject* pObject, NiPoint3& arPos, NiPoint3& arVel);
    // A random vertex of the geometry; with normals, the velocity points along the vertex's world normal.
    void GetPointFromGeometry(NiAVObject* pObject, NiPoint3& arPos, NiPoint3& arVel);
    // "<prefix> = AT_..." for the array type, in a MemoryManager block the caller owns.
    char* GetViewerString(const char* apPrefix, PARTICLEARRAYTYPE aeType);

public:
    static NiObject* CreateObject();

    PARTICLEARRAYTYPE eType;
    NiTPrimitiveArray<NiPointer<NiAVObject>> aObjects;

    static NiRTTI ms_RTTI;
};
