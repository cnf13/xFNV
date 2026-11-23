#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class BSStream;
class NiAVObject;

// a streamed object that refers to another one by name
class BSReference : public NiObject {
public:
    BSReference() : pName(nullptr) {}
    explicit BSReference(const char* apName) : pName(nullptr) {
        SetName(apName);
    }
    ~BSReference() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream&) override {}
    bool RegisterStreamables(NiStream&) override {
        return true;
    }
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;

    void SetName(const char* apName);

    static NiObject* CreateObject();

    char* pName;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(BSReference);

// a streamed list of scene graph objects
class BSNodeReferences : public NiObject {
public:
    BSNodeReferences() : RefNodeA(0, 1) {}
    ~BSNodeReferences() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject*) override {
        return false;
    }

    // Adds the parent of apObject to the list when apObject is a scene graph object whose parent the stream's
    // reference map holds; true when it was added.
    bool AddParentRef(BSStream& arStream, NiObject* apObject);

    static NiObject* CreateObject();

    NiTPrimitiveArray<NiAVObject*> RefNodeA;

    static NiRTTI ms_RTTI;
};
