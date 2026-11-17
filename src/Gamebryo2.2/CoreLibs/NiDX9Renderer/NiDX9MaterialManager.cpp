#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9MaterialManager.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

bool NiDX9MaterialManager::MaterialEntry::Update(const NiMaterialProperty* apMaterial) {
    uint32_t m_uiRevID = apMaterial->m_uiRevID;
    if (this->m_uiRevID == m_uiRevID) {
        return false;
    }
    this->m_uiRevID = m_uiRevID;
    const NiColor* m_external_emit = apMaterial->m_external_emit;
    if (!m_external_emit) {
        m_external_emit = &apMaterial->m_emit;
    }
    float apMateriala = apMaterial->m_fAlpha;
    m_kD3DMat.Emissive.r = m_external_emit->r;
    m_kD3DMat.Emissive.g = m_external_emit->g;
    m_kD3DMat.Emissive.b = m_external_emit->b;
    m_kD3DMat.Emissive.a = apMateriala;
    m_kD3DMat.Ambient.r = NiColor_BLACK.r;
    m_kD3DMat.Ambient.g = NiColor_BLACK.g;
    m_kD3DMat.Ambient.b = NiColor_BLACK.b;
    m_kD3DMat.Ambient.a = apMateriala;
    m_kD3DMat.Diffuse.r = NiColor_BLACK.r;
    m_kD3DMat.Diffuse.g = NiColor_BLACK.g;
    m_kD3DMat.Diffuse.b = NiColor_BLACK.b;
    m_kD3DMat.Diffuse.a = apMateriala;
    m_kD3DMat.Specular.r = apMaterial->m_spec.r;
    m_kD3DMat.Specular.g = apMaterial->m_spec.g;
    m_kD3DMat.Specular.b = apMaterial->m_spec.b;
    m_kD3DMat.Specular.a = apMateriala;
    double m_fShine = apMaterial->m_fShine;
    m_kD3DMat.Power = m_fShine;
    return true;
}

// Frees the default material's entry, which the manager made.
NiDX9MaterialManager::~NiDX9MaterialManager() {
    if (NiMaterialProperty::ms_spDefault) {
        MaterialEntry* pkEntry = static_cast<MaterialEntry*>(NiMaterialProperty::ms_spDefault->m_pvRendererData);
        if (pkEntry) {
            NiMaterialProperty::ms_spDefault->m_pvRendererData = nullptr;
            if (pkEntry == m_pkCurrentMaterial) {
                m_pkCurrentMaterial = nullptr;
            }
            delete pkEntry;
        }
    }
}

void NiDX9MaterialManager::PurgeMaterial(NiMaterialProperty* apMaterial) {
    void* m_pvRendererData;
    if (apMaterial) {
        m_pvRendererData = apMaterial->m_pvRendererData;
        if (m_pvRendererData) {
            apMaterial->m_pvRendererData = nullptr;
            if (static_cast<const void*>(m_pvRendererData) == static_cast<const void*>(m_pkCurrentMaterial)) {
                m_pkCurrentMaterial = nullptr;
            }
            NiMemObject::operator delete(m_pvRendererData, 0x48u);
        }
    }
}

void NiDX9MaterialManager::SetCurrentMaterial(const NiMaterialProperty* apMaterial) {
    NiMaterialProperty* pkMaterial = const_cast<NiMaterialProperty*>(apMaterial);
    MaterialEntry* pkEntry = static_cast<MaterialEntry*>(pkMaterial->m_pvRendererData);
    if (!pkEntry) {
        pkEntry = static_cast<MaterialEntry*>(NiMemObject::operator new(sizeof(MaterialEntry)));
        if (pkEntry) {
            pkEntry->m_uiRevID = 0;
            pkEntry->Update(apMaterial);
        }
        pkMaterial->m_pvRendererData = pkEntry;
        m_bChanged = true;
    }
    if (pkEntry->Update(apMaterial)) {
        m_bChanged = true;
    }
    if (pkEntry != m_pkCurrentMaterial) {
        m_bChanged = true;
        m_pkCurrentMaterial = pkEntry;
    }
}

void NiDX9MaterialManager::UpdateToDevice(IDirect3DDevice9* apDevice) {
    if (m_bChanged) {
        m_bChanged = false;
        apDevice->SetMaterial(&m_pkCurrentMaterial->m_kD3DMat);
    }
}

void NiDX9MaterialManager::Initialize() {
    m_pkCurrentMaterial = nullptr;
    SetCurrentMaterial(NiMaterialProperty::ms_spDefault.m_pObject);
    m_bChanged = true;
}
