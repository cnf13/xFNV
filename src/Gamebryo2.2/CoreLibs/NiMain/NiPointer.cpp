#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/IOManager.h"
#include "BSShader/GrassLoadTask.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BSCloneReserver.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

void NiPointerDefaultConstructor(NiPointer<NiRefObject>* apThis) {
    ::new (apThis) NiPointer<NiRefObject>(static_cast<NiRefObject*>(nullptr));
}

void NiPointer_DefaultConstructorClosure_B54620(NiPointer<NiRefObject>* apThis) {
    apThis->m_pObject = nullptr;
}

void NiPointer_Destruct_AE9290(NiPointer<NiRefObject>* apThis) {
    NiRefObject* m_pObject = apThis->m_pObject;
    if (apThis->m_pObject) {
        NiDecRefCount(m_pObject);
    }
}

void NiPointer_Destruct_B66F20(NiPointer<NiRefObject>* apThis) {
    NiRefObject* m_pObject = apThis->m_pObject;
    if (apThis->m_pObject) {
        NiDecRefCount(m_pObject);
    }
}

NiPointer<ActorPathingMessageQueue>*
NiPointer_Construct_6EBF60(NiPointer<ActorPathingMessageQueue>* apThis, ActorPathingMessageQueue* apObject) {
    apThis->m_pObject = apObject;
    if (apThis->m_pObject) {
        apThis->m_pObject->IncRefCount();
    }
    return apThis;
}
