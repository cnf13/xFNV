#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherUpdateTask.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMorphData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <bit>
#include <cstring>
#include <limits>

NiRTTI NiGeomMorpherController::ms_RTTI("NiGeomMorpherController", &NiInterpController::ms_RTTI);

NiGeomMorpherController::InterpItem::InterpItem() : m_fWeight(0.0f) {}

NiGeomMorpherController::NiGeomMorpherController()
  : m_uFlags(0), m_spMorphData(nullptr), m_pkInterpItems(nullptr), m_bNeedPreDisplay(false), m_bIgnoreBaseTarget(false),
    m_bAlwaysUpdate(false), m_bBoundCalculated(false) {}

NiGeomMorpherController::~NiGeomMorpherController() {
    m_spMorphData = nullptr;
    delete[] m_pkInterpItems;
}

const NiRTTI* NiGeomMorpherController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiGeomMorpherController::CreateClone(NiCloningProcess& kCloning) {
    NiGeomMorpherController* v4 = new NiGeomMorpherController;
    CopyMembers(v4, kCloning);
    return v4;
}

// Before 10.0.1.2 the flags come from the stream's time controller flags. Before 20.1.0.3 the interpolators are
// links LinkObject reads (with the weights from the morph data); later files store a link and a weight per target.
void NiGeomMorpherController::LoadBinary(NiStream& kStream) {
    NiInterpController::LoadBinary(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 2)) {
        m_uFlags = (kStream.m_usNiTimeControllerFlags >> 5) & 0x3F;
    }
    else {
        NiStreamLoadBinary(kStream, m_uFlags);
    }

    // m_spMorphData
    kStream.ReadLinkID();

    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 0, 0, 2)) {
        uint8_t ucAlwaysUpdate;
        NiStreamLoadBinary(kStream, ucAlwaysUpdate);
        m_bAlwaysUpdate = ucAlwaysUpdate != 0;
    }

    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        return;
    }
    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 3)) {
        kStream.ReadMultipleLinkIDs();
        return;
    }

    uint32_t uiNumInterps;
    NiStreamLoadBinary(kStream, uiNumInterps);
    m_pkInterpItems = NiTNewInterface<InterpItem>::Allocate(uiNumInterps);
    for (uint16_t i = 0; i < uiNumInterps; i++) {
        m_pkInterpItems[i].m_spInterpolator = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
        NiStreamLoadBinary(kStream, m_pkInterpItems[i].m_fWeight);
    }
}

// Files before 20.1.0.3 keep the weights in the morph data, and the interpolators in the morph targets (before
// 10.1.0.104) or as a link list. Files up to 4.1.0.12 have keys that must cover the controller's key range.
void NiGeomMorpherController::LinkObject(NiStream& kStream) {
    NiInterpController::LinkObject(kStream);
    m_spMorphData = static_cast<NiMorphData*>(kStream.GetObjectFromLinkID());

    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 3)) {
        AllocateInterpArray();
        for (uint32_t i = 0; i < GetNumTargets(); i++) {
            m_pkInterpItems[i].m_fWeight = m_spMorphData->GetTarget(i)->m_fLegacyWeight;
        }
    }

    // A file before 10.1.0.104 with no morph data reads the interpolator link IDs as a newer file does (the version and
    // the data are tested together).
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104) && m_spMorphData) {
        uint32_t uiNumTargets = m_spMorphData->m_uiNumTargets;
        for (uint32_t i = 0; i < uiNumTargets; i++) {
            SetInterpolator(m_spMorphData->GetTarget(i)->GetLegacyInterpolator(), static_cast<uint16_t>(i));
        }
    }
    else if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 3)) {
        uint32_t uiNumInterps = kStream.GetNumberOfLinkIDs();
        for (uint32_t i = 0; i < uiNumInterps; i++) {
            SetInterpolator(static_cast<NiInterpolator*>(kStream.GetObjectFromLinkID()), static_cast<uint16_t>(i));
        }
    }

    if (m_spMorphData && kStream.GetFileVersion() <= NiStream::GetVersion(4, 1, 0, 12)) {
        GuaranteeTimeRange(m_fLoKeyTime, m_fHiKeyTime);
    }
}

bool NiGeomMorpherController::RegisterStreamables(NiStream& kStream) {
    if (!NiInterpController::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spMorphData) {
        m_spMorphData->RegisterStreamables(kStream);
    }
    for (uint16_t i = 0; i < GetInterpolatorCount(); i++) {
        NiInterpolator* pkInterp = GetInterpolator(i);
        if (pkInterp) {
            pkInterp->RegisterStreamables(kStream);
        }
    }
    return true;
}

// Always the current format: a link and a weight per morph target.
void NiGeomMorpherController::SaveBinary(NiStream& kStream) {
    NiInterpController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
    kStream.SaveLinkID(m_spMorphData);
    uint8_t ucAlwaysUpdate = m_bAlwaysUpdate;
    NiStreamSaveBinary(kStream, ucAlwaysUpdate);

    uint32_t uiNumTargets = GetNumTargets();

    NiStreamSaveBinary(kStream, uiNumTargets);
    for (uint32_t i = 0; i < uiNumTargets; i++) {
        kStream.SaveLinkID(m_pkInterpItems[i].m_spInterpolator);
        NiStreamSaveBinary(kStream, m_pkInterpItems[i].m_fWeight);
    }
}

// The morph data must be set on both. Interpolators are compared from the other controller's side.
bool NiGeomMorpherController::IsEqual(NiObject* pkObject) {
    if (!NiInterpController::IsEqual(pkObject)) {
        return false;
    }

    NiGeomMorpherController* pkOther = static_cast<NiGeomMorpherController*>(pkObject);
    if (!m_spMorphData->IsEqual(pkOther->m_spMorphData)) {
        return false;
    }

    for (uint32_t i = 0; i < GetNumTargets(); i++) {
        NiInterpolator* pkInterp = m_pkInterpItems[i].m_spInterpolator;
        NiInterpolator* pkOtherInterp = pkOther->m_pkInterpItems[i].m_spInterpolator;
        if ((pkOtherInterp && !pkInterp) || (!pkOtherInterp && pkInterp)) {
            return false;
        }
        if (pkOtherInterp && !pkOtherInterp->IsEqual(pkInterp)) {
            return false;
        }
        if (pkOther->m_pkInterpItems[i].m_fWeight != m_pkInterpItems[i].m_fWeight) {
            return false;
        }
    }
    return true;
}

// No base class strings. Per target: "<index> <name>", with " = <value>" for a (blend) float interpolator. The target
// count is read directly, not through GetNumTargets.
void NiGeomMorpherController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    pkStrings->Add(pcString);

    pcString = NiGetViewerString("NumTargets", m_spMorphData ? m_spMorphData->m_uiNumTargets : 0u);
    pkStrings->Add(pcString);

    pcString = NiGetViewerString("NumVertsPerTarget", m_spMorphData ? m_spMorphData->m_uiNumVertsPerTarget : 0u);
    pkStrings->Add(pcString);

    uint32_t uiNumTargets = m_spMorphData ? m_spMorphData->m_uiNumTargets : 0u;
    for (uint32_t i = 0; i < uiNumTargets; i++) {
        NiMorphData::MorphTarget* pkTarget = m_spMorphData->GetTarget(i);
        NiInterpolator* pkInterp = GetInterpolator(static_cast<uint16_t>(i));
        char acString[256];
        if (pkInterp) {
            NiBlendFloatInterpolator* pkBlend = static_cast<NiBlendFloatInterpolator*>(
                NiObject::DynamicCast(&NiBlendFloatInterpolator::ms_RTTI, pkInterp)
            );
            NiFloatInterpolator* pkFloat = static_cast<NiFloatInterpolator*>(
                NiObject::DynamicCast(&NiFloatInterpolator::ms_RTTI, pkInterp)
            );
            if (pkBlend) {
                float fValue = pkBlend->m_fFloatValue;
                std::sprintf(
                    acString,
                    "%d %s = %0.2f",
                    i,
                    static_cast<const char*>(pkTarget->GetName()),
                    static_cast<double>(fValue)
                );
            }
            else if (pkFloat) {
                float fValue = pkFloat->m_fFloatValue;
                std::sprintf(
                    acString,
                    "%d %s = %0.2f",
                    i,
                    static_cast<const char*>(pkTarget->GetName()),
                    static_cast<double>(fValue)
                );
            }
            else {
                std::sprintf(acString, "%d %s", i, static_cast<const char*>(pkTarget->GetName()));
            }
        }
        else {
            std::sprintf(acString, "%d %s", i, static_cast<const char*>(pkTarget->GetName()));
        }
        size_t stLength = std::strlen(acString) + 3;
        pcString = static_cast<char*>(_NiMalloc(stLength));
        strcpy_s(pcString, stLength, acString);
        pkStrings->Add(pcString);
    }
}

// The base class part is NiTimeController's (NiInterpController has none).
void NiGeomMorpherController::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);
    for (uint16_t i = 0; i < GetNumTargets(); i++) {
        if (m_pkInterpItems[i].m_spInterpolator) {
            m_pkInterpItems[i].m_spInterpolator->ProcessClone(kCloning);
        }
    }
}

// A parallel update pass only notes that a morph controller was found. Otherwise the weights are updated when the
// time moved (or always, when manager-controlled or m_bAlwaysUpdate) and, with parallel updates on and the target
// data not shared, a task is queued.
void NiGeomMorpherController::Update(NiUpdateData& UpdateData) {
    if (!m_spMorphData) {
        return;
    }

    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundMorphController = true;
        return;
    }

    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_bNeedPreDisplay = true;
        m_fScaledTime = NiInterpController::INVALID_TIME;
    }
    else if (!DontDoUpdate(UpdateData.fTime) || m_bAlwaysUpdate) {
        m_bNeedPreDisplay = true;
    }

    if (!m_pkTarget || !m_bNeedPreDisplay) {
        return;
    }

    GenMorphInterp(m_fScaledTime);
    if (ms_bParallelUpdateEnabled &&
        NiParallelUpdateTaskManager::IsActive() &&
        static_cast<NiGeometry*>(m_pkTarget)->m_spModelData->m_uiRefCount == 1) {
        NiGeomMorpherUpdateTask* pkTask = NiGeomMorpherUpdateTask::GetFreeObject();
        if (pkTask) {
            pkTask->SetController(this, static_cast<NiGeometry*>(m_pkTarget));
            // A task the manager did not take goes back to the pool.
            if (!NiParallelUpdateTaskManager::ms_pkManager
                     ->NiParallelUpdateTaskManager::AddTask(pkTask, NiTaskManager::HIGH)) {
                pkTask->Clear();
            }
        }
    }
}

// A non-geometry target is dropped. Morph data present: the bound is computed for the new target.
void NiGeomMorpherController::SetTarget(NiObjectNET* pkTarget) {
    if (pkTarget->IsGeometry()) {
        NiTimeController::SetTarget(pkTarget);
        if (m_spMorphData) {
            CalculateMorphBound();
            m_bBoundCalculated = true;
        }
    }
    else {
        NiTimeController::SetTarget(nullptr);
    }
}

// Marks the morphed vertices changed and recomputes the normals when asked to (and the data has normals).
void NiGeomMorpherController::OnPreDisplay() {
    if (m_pkTarget && m_bNeedPreDisplay) {
        NiGeometryData* pkData = static_cast<NiGeometry*>(m_pkTarget)->m_spModelData;
        pkData->MarkAsChanged(1);
        if ((m_uFlags & UPDATE_NORMALS_MASK) && pkData->m_pkNormal) {
            pkData->CalculateNormals();
        }
        m_bNeedPreDisplay = false;
    }
}

uint16_t NiGeomMorpherController::GetInterpolatorCount() const {
    return static_cast<uint16_t>(GetNumTargets());
}

// The target's name. An index past the last target reads through a null target.
const char* NiGeomMorpherController::GetInterpolatorID(uint16_t usIndex) {
    return m_spMorphData->GetTarget(usIndex)->GetName();
}

// The first target whose name matches apcID, ignoring case.
uint16_t NiGeomMorpherController::GetInterpolatorIndex(const char* pcID) const {
    if (!pcID) {
        return NiInterpController::INVALID_INDEX;
    }
    for (uint16_t i = 0; i < GetNumTargets(); i++) {
        if (NiStricmp(m_spMorphData->GetTarget(i)->GetName(), pcID) == 0) {
            return i;
        }
    }
    return NiInterpController::INVALID_INDEX;
}

// The first target whose name is akID (same string handle).
uint16_t NiGeomMorpherController::GetInterpolatorIndexFx(const NiFixedString& arName) const {
    for (uint16_t i = 0; i < GetNumTargets(); i++) {
        if (m_spMorphData->GetTarget(i)->GetName() == arName) {
            return i;
        }
    }
    return NiInterpController::INVALID_INDEX;
}

NiInterpolator* NiGeomMorpherController::GetInterpolator(uint16_t usIndex) const {
    return m_pkInterpItems[usIndex].m_spInterpolator;
}

// Any interpolator is accepted (InterpolatorIsCorrectType is not asked).
void NiGeomMorpherController::SetInterpolator(NiInterpolator* pkInterpolator, uint16_t usIndex) {
    m_pkInterpItems[usIndex].m_spInterpolator = pkInterpolator;
}

// The key range becomes the union of the interpolators' active ranges; a manager-controlled controller keeps its own.
void NiGeomMorpherController::ResetTimeExtrema() {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        return;
    }

    uint32_t uiNumTargets = GetNumTargets();

    m_fLoKeyTime = std::numeric_limits<float>::max();
    m_fHiKeyTime = -std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < uiNumTargets; i++) {
        NiInterpolator* pkInterp = GetInterpolator(static_cast<uint16_t>(i));
        if (pkInterp) {
            float fHi = -std::numeric_limits<float>::max();
            float fLo = std::numeric_limits<float>::max();
            pkInterp->GetActiveTimeRange(fLo, fHi);
            if (m_fLoKeyTime > fLo) {
                m_fLoKeyTime = fLo;
            }
            if (m_fHiKeyTime < fHi) {
                m_fHiKeyTime = fHi;
            }
        }
    }
}

NiInterpolator* NiGeomMorpherController::CreatePoseInterpolator(uint16_t usIndex) {
    NiGeomMorpherController::InterpItem* m_pkInterpItems;
    m_pkInterpItems = this->m_pkInterpItems;
    float m_fWeight = 0.0f;
    if (m_pkInterpItems) {
        m_fWeight = m_pkInterpItems[usIndex].m_fWeight;
    }
    return new NiFloatInterpolator(m_fWeight);
}

// Sets a float interpolator's pose value to the target's current weight. Any other interpolator (or null) is passed
// on as null.
void NiGeomMorpherController::SynchronizePoseInterpolator(NiInterpolator* pkInterp, uint16_t usIndex) {
    float fWeight = GetWeight(usIndex);
    NiFloatInterpolator* pkFloat = pkInterp
        ? static_cast<NiFloatInterpolator*>(pkInterp->DynamicCastMember(&NiFloatInterpolator::ms_RTTI))
        : nullptr;
    pkFloat->SetPoseValue(fWeight);
}

NiBlendInterpolator* NiGeomMorpherController::CreateBlendInterpolator(
    uint16_t,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    return new NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

void NiGeomMorpherController::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    for (uint32_t i = 0; i < m_spMorphData->m_uiNumTargets; i++) {
        NiInterpolator* pkInterp = GetInterpolator(static_cast<uint16_t>(i));
        if (pkInterp) {
            pkInterp->GuaranteeTimeRange(fStartTime, fEndTime);
        }
    }
}

// A target data without vertices gets a new, uninitialized array of its vertex count (other arrays kept).
NiPoint3* NiGeomMorpherController::GetVertexData() {
    if (!m_pkTarget) {
        return nullptr;
    }

    NiGeometryData* pkData = static_cast<NiGeometry*>(m_pkTarget)->m_spModelData;
    if (pkData->m_pkVertex) {
        return pkData->m_pkVertex;
    }

    uint16_t usVertices = pkData->m_usVertices;
    NiPoint3* pkVertex = new NiPoint3[usVertices];
    pkData->ReplaceData(
        usVertices,
        pkVertex,
        pkData->m_pkNormal,
        pkData->m_pkColor,
        pkData->m_pkTexture,
        pkData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK,
        static_cast<NiGeometryData::DataFlags>(pkData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK)
    );
    return pkVertex;
}

bool NiGeomMorpherController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiGeometry::ms_RTTI, m_pkTarget);
}

bool NiGeomMorpherController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsFloatValueSupported();
}

// The bound of the first target merged with every other target's; no targets: a zero sphere at the origin. The
// morph data must be set.
void NiGeomMorpherController::CalculateMorphBound() {
    NiBound kBound;
    uint32_t uiNumTargets = m_spMorphData->m_uiNumTargets;
    if (uiNumTargets == 0) {
        kBound.m_kCenter = NiPoint3::ZERO;
        kBound.m_fRadius = 0.0f;
    }
    else {
        kBound.ComputeFromData(m_spMorphData->m_uiNumVertsPerTarget, m_spMorphData->GetTarget(0)->m_aTargetVerts);
        for (uint32_t i = 1; i < uiNumTargets; i++) {
            NiBound kTargetBound;
            kTargetBound.ComputeFromData(
                m_spMorphData->m_uiNumVertsPerTarget,
                m_spMorphData->GetTarget(i)->m_aTargetVerts
            );
            kBound.Merge(&kTargetBound);
        }
    }
    static_cast<NiGeometry*>(m_pkTarget)->m_spModelData->m_kBound = kBound;
}

uint32_t NiGeomMorpherController::GetNumTargets() const {
    return m_spMorphData ? m_spMorphData->m_uiNumTargets : 0;
}

void NiGeomMorpherController::SetMorphData(NiMorphData* pkData, bool bCalculateMorphBound) {
    m_spMorphData = pkData;
    AllocateInterpArray();
    if (bCalculateMorphBound) {
        CalculateMorphBound();
    }
}

// The clone gets the same morph data (its bound computed when the clone already has a target), shared clones of
// the interpolators and the weights.
void NiGeomMorpherController::CopyMembers(NiGeomMorpherController* pkDest, NiCloningProcess& kCloning) {
    NiInterpController::CopyMembers(pkDest, kCloning);
    pkDest->m_uFlags = m_uFlags;
    if (m_spMorphData) {
        pkDest->SetMorphData(m_spMorphData, pkDest->m_pkTarget != nullptr);
    }
    if (ms_bAlwaysUpdateClone) {
        m_bAlwaysUpdate = true;
        pkDest->m_bAlwaysUpdate = true;
    }
    for (uint16_t i = 0; i < GetNumTargets(); i++) {
        NiInterpolator* pkInterp = m_pkInterpItems[i].m_spInterpolator;
        if (pkInterp) {
            pkDest->m_pkInterpItems[i].m_spInterpolator = static_cast<NiInterpolator*>(
                pkInterp->CreateSharedClone(kCloning)
            );
        }
        pkDest->m_pkInterpItems[i].m_fWeight = m_pkInterpItems[i].m_fWeight;
    }
}

void NiGeomMorpherController::GenMorphInterp(float) {
    NiPoint3* pkVerts = GetVertexData();
    NiGeometryData* pkData = static_cast<NiGeometry*>(m_pkTarget)->m_spModelData;
    uint32_t uiNumVerts = pkData->m_usVertices;
    uint32_t uiNumTargets = m_spMorphData->m_uiNumTargets;

    for (uint32_t i = 0; i < uiNumTargets; i++) {
        NiInterpolator* pkInterp = GetInterpolator(static_cast<uint16_t>(i));
        float fWeight = GetWeight(i);
        if (i == 0 && m_spMorphData->m_bRelativeTargets) {
            // the base target of relative morph data always counts fully, unless it is ignored
            fWeight = m_bIgnoreBaseTarget ? 0.0f : 1.0f;
            SetWeight(i, fWeight);
        }
        else if (pkInterp && pkInterp->Update(m_fScaledTime, m_pkTarget, fWeight)) {
            SetWeight(i, fWeight);
        }
    }

    if (!m_bIgnoreBaseTarget) {
        std::memset(pkVerts, 0, uiNumVerts * sizeof(NiPoint3));
    }

    // targets whose weight is within 0.001 of zero are skipped
    bool bChanged = false;
    for (uint32_t i = 0; i < uiNumTargets; i++) {
        if (!m_pkInterpItems) {
            continue;
        }
        float fWeight = m_pkInterpItems[i].m_fWeight;
        if (fWeight >= 0.001f || fWeight <= -0.001f) {
            NiPoint3::PointsPlusEqualFloatTimesPoints(
                pkVerts,
                fWeight,
                m_spMorphData->m_aMorphTargets[i].m_aTargetVerts,
                uiNumVerts
            );
            bChanged = true;
        }
    }

    if (bChanged && m_pkTarget->IsGeometry()) {
        static_cast<NiGeometry*>(m_pkTarget)->m_spModelData->MarkAsChanged(1);
    }
}

void NiGeomMorpherController::AllocateInterpArray() {
    NiTNewInterface<InterpItem>::Deallocate(m_pkInterpItems);
    m_pkInterpItems = nullptr;
    if (m_spMorphData) {
        uint32_t uiNumTargets = m_spMorphData->m_uiNumTargets;
        if (uiNumTargets > 0) {
            m_pkInterpItems = NiTNewInterface<InterpItem>::Allocate(uiNumTargets);
        }
    }
}

NiObject* NiGeomMorpherController::CreateObject() {
    return new NiGeomMorpherController;
}
