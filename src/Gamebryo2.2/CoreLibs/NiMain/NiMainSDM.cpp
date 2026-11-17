#include "Gamebryo2.2/CoreLibs/NiMain/NiMainSDM.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAmbientLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBSPNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBinaryExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBooleanExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColorExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDitherProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFogProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegersExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMetricsLayer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiOldScreenSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleMeshes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleMeshesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProcessorSpecificCode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRangeLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSingleShaderMaterialLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSortAdjustNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSpecularProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSpotLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStringExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStringsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchStringExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVectorExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertWeightsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

// Frees the chain of blocks at apMem: each block's first pointer links the next.
void FreeMemoryBlocks() {
    void** v0 = static_cast<void**>(apMem);
    if (apMem) {
        void** v1;
        do {
            v1 = static_cast<void**>(*v0);
            MemoryManager_Singleton.Deallocate(v0);
            v0 = v1;
        }
        while (v1);
    }
    uiMemBlockCount = 0;
    apMem = 0;
}

// The first instance registers the library's Init and Shutdown with the static data manager.
NiMainSDM::NiMainSDM() {
    if (!siCounter++) {
        NiStaticDataManager::SetRootLibrary(NiMainSDM::Init, NiMainSDM::Shutdown);
    }
}

void NiMainSDM::Init() {
    if (!NiMainSDM::ms_bInitialized) {
        NiMainSDM::ms_bInitialized = 1;
        NiGlobalStringTable::_SDMInit();
        CreateSinTable();
        NiShaderConstantMap::_SDMInit();
        NiTextureEffect::_SDMInit();
        NiAlphaProperty::_SDMInit();
        NiFogProperty::_SDMInit();
        NiMaterialProperty::_SDMInit();
        NiShadeProperty::_SDMInit();
        NiShaderConstantMapEntry::_SDMInit();
        NiStencilProperty::_SDMInit();
        NiTexturingProperty::_SDMInit();
        NiCullingProperty::_SDMInit();
        NiAVObject::NullBoundS.m_fRadius = 0.0f;
        NiAVObject::NullBoundS.m_kCenter = NiPoint3_ZERO;
        NiImageConverter::_SDMInit();
        NiStream::_SDMInit();
        NiStream::RegisterLoader("BSPackedAdditionalGeometryData", &BSPackedAdditionalGeometryData::CreateObject);
        NiStream::RegisterLoader("NiAdditionalGeometryData", &NiAdditionalGeometryData::CreateObject);
        NiStream::RegisterLoader("NiAlphaAccumulator", &NiAlphaAccumulator::CreateObject);
        NiStream::RegisterLoader("NiAlphaProperty", &NiAlphaProperty::CreateObject);
        NiStream::RegisterLoader("NiAmbientLight", &NiAmbientLight::CreateObject);
        NiStream::RegisterLoader("NiCullingProperty", &NiCullingProperty::CreateObject);
        NiStream::RegisterLoader("NiAutoNormalParticles", &NiParticles::CreateObject);
        NiStream::RegisterLoader("NiAutoNormalParticlesData", &NiParticlesData::CreateObject);
        NiStream::RegisterLoader("NiBillboardNode", &NiBillboardNode::CreateObject);
        NiStream::RegisterLoader("NiBinaryExtraData", &NiBinaryExtraData::CreateObject);
        NiStream::RegisterLoader("NiBooleanExtraData", &NiBooleanExtraData::CreateObject);
        NiStream::RegisterLoader("NiBSPNode", &NiBSPNode::CreateObject);
        NiStream::RegisterLoader("NiCamera", &NiCamera::CreateObject);
        NiStream::RegisterLoader("NiClusterAccumulator", &NiAlphaAccumulator::CreateObject);
        NiStream::RegisterLoader("NiCollisionSwitch", &NiNode::CreateObject);
        NiStream::RegisterLoader("NiColorExtraData", &NiColorExtraData::CreateObject);
        NiStream::RegisterLoader("NiDefaultAVObjectPalette", &NiDefaultAVObjectPalette::CreateObject);
        NiStream::RegisterLoader("NiDirectionalLight", &NiDirectionalLight::CreateObject);
        NiStream::RegisterLoader("NiDitherProperty", &NiDitherProperty::CreateObject);
        NiStream::RegisterLoader("NiExtraData", &NiExtraData::CreateObject);
        NiStream::RegisterLoader("NiFloatExtraData", &NiFloatExtraData::CreateObject);
        NiStream::RegisterLoader("NiFloatsExtraData", &NiFloatsExtraData::CreateObject);
        NiStream::RegisterLoader("NiFogProperty", &NiFogProperty::CreateObject);
        NiStream::RegisterLoader("NiIntegerExtraData", &NiIntegerExtraData::CreateObject);
        NiStream::RegisterLoader("NiIntegersExtraData", &NiIntegersExtraData::CreateObject);
        NiStream::RegisterLoader("NiLODNode", &NiLODNode::CreateObject);
        NiStream::RegisterLoader("NiLines", &NiLines::CreateObject);
        NiStream::RegisterLoader("NiLinesData", &NiLinesData::CreateObject);
        NiStream::RegisterLoader("NiMaterialProperty", &NiMaterialProperty::CreateObject);
        NiStream::RegisterLoader("NiNode", &NiNode::CreateObject);
        NiStream::RegisterLoader("NiPalette", &NiPalette::CreateObject);
        NiStream::RegisterLoader("NiParticleMeshes", &NiParticleMeshes::CreateObject);
        NiStream::RegisterLoader("NiParticleMeshesData", &NiParticleMeshesData::CreateObject);
        NiStream::RegisterLoader("NiParticles", &NiParticles::CreateObject);
        NiStream::RegisterLoader("NiParticlesData", &NiParticlesData::CreateObject);
        NiStream::RegisterLoader(
            "NiPersistentSrcTextureRendererData", &NiPersistentSrcTextureRendererData::CreateObject
        );
        NiStream::RegisterLoader("NiPixelData", &NiPixelData::CreateObject);
        NiStream::RegisterLoader("NiPointLight", &NiPointLight::CreateObject);
        NiStream::RegisterLoader("NiRangeLODData", &NiRangeLODData::CreateObject);
        NiStream::RegisterLoader("NiRotatingParticles", &NiParticles::CreateObject);
        NiStream::RegisterLoader("NiRotatingParticlesData", &NiParticlesData::CreateObject_00A96660);
        NiStream::RegisterLoader("NiScreenLODData", &NiScreenLODData::CreateObject);
        NiStream::RegisterLoader("NiScreenElements", &NiScreenElements::CreateObject);
        NiStream::RegisterLoader("NiScreenElementsData", &NiScreenElementsData::CreateObject);
        NiStream::RegisterLoader("NiScreenTexture", &NiScreenTexture::CreateObject);
        NiStream::RegisterLoader("NiShadeProperty", &NiShadeProperty::CreateObject);
        NiStream::RegisterLoader("NiSkinData", &NiSkinData::CreateObject);
        NiStream::RegisterLoader("NiSkinInstance", &NiSkinInstance::CreateObject);
        NiStream::RegisterLoader("BSDismemberSkinInstance", &BSDismemberSkinInstance::CreateObject);
        NiStream::RegisterLoader("NiSkinPartition", &NiSkinPartition::CreateObject);
        NiStream::RegisterLoader("NiSortAdjustNode", &NiSortAdjustNode::CreateObject);
        NiStream::RegisterLoader("NiSourceTexture", &NiSourceTexture::CreateObject);
        NiStream::RegisterLoader("NiSourceCubeMap", &NiSourceCubeMap::CreateObject);
        NiStream::RegisterLoader("NiSpecularProperty", &NiSpecularProperty::CreateObject);
        NiStream::RegisterLoader("NiSpotLight", &NiSpotLight::CreateObject);
        NiStream::RegisterLoader("NiStencilProperty", &NiStencilProperty::CreateObject);
        NiStream::RegisterLoader("NiStringExtraData", &NiStringExtraData::CreateObject);
        NiStream::RegisterLoader("NiStringsExtraData", &NiStringsExtraData::CreateObject);
        NiStream::RegisterLoader("NiSwitchNode", &NiSwitchNode::CreateObject);
        NiStream::RegisterLoader("NiSwitchStringExtraData", &NiSwitchStringExtraData::CreateObject);
        NiStream::RegisterLoader("NiTextureEffect", &NiTextureEffect::CreateObject);
        NiStream::RegisterLoader("NiTexturingProperty", &NiTexturingProperty::CreateObject);
        NiStream::RegisterLoader("NiTriShape", &NiTriShape::CreateObject);
        NiStream::RegisterLoader("BSSegmentedTriShape", &BSSegmentedTriShape::CreateObject);
        NiStream::RegisterLoader("NiTriShapeData", &NiTriShapeData::CreateObject);
        NiStream::RegisterLoader("NiTriShapeDynamicData", &NiTriShapeDynamicData::CreateObject);
        NiStream::RegisterLoader("NiTriStrips", &NiTriStrips::CreateObject);
        NiStream::RegisterLoader("NiTriStripsData", &NiTriStripsData::CreateObject);
        NiStream::RegisterLoader("NiTriStripsDynamicData", &NiTriStripsDynamicData::CreateObject);
        NiStream::RegisterLoader("NiVectorExtraData", &NiVectorExtraData::CreateObject);
        NiStream::RegisterLoader("NiVertexColorProperty", &NiVertexColorProperty::CreateObject);
        NiStream::RegisterLoader("NiVertWeightsExtraData", &NiVertWeightsExtraData::CreateObject);
        NiStream::RegisterLoader("NiScreenSpaceCamera", &NiScreenSpaceCamera::CreateObject);
        NiStream::RegisterLoader("NiScreenGeometry", &NiScreenGeometry::CreateObject);
        NiStream::RegisterLoader("NiScreenGeometryData", &NiScreenGeometryData::CreateObject);
        NiStream::RegisterLoader("NiScreenPolygon", &NiScreenPolygon::CreateObject);
        NiScreenGeometryData::_SDMInit();
        NiTexture::RendererData::_SDMInit();
        NiPoint3::_SDMInit();
        NiSingleShaderMaterialLibrary::_SDMInit();
        NiMaterial::_SDMInit();
        NiProcessorSpecificCode::OptimizeMainEngineForProcessor();
    }
}

void NiMainSDM::Shutdown() {
    if (NiMainSDM::ms_bInitialized) {
        NiMainSDM::ms_bInitialized = 0;
        NiStream::UnregisterLoader("BSPackedAdditionalGeometryData");
        NiStream::UnregisterLoader("NiAdditionalGeometryData");
        NiStream::UnregisterLoader("NiAlphaAccumulator");
        NiStream::UnregisterLoader("NiAlphaProperty");
        NiStream::UnregisterLoader("NiAmbientLight");
        NiStream::UnregisterLoader("NiCullingProperty");
        NiStream::UnregisterLoader("NiAutoNormalParticles");
        NiStream::UnregisterLoader("NiAutoNormalParticlesData");
        NiStream::UnregisterLoader("NiBillboardNode");
        NiStream::UnregisterLoader("NiBinaryExtraData");
        NiStream::UnregisterLoader("NiBooleanExtraData");
        NiStream::UnregisterLoader("NiBSPNode");
        NiStream::UnregisterLoader("NiCamera");
        NiStream::UnregisterLoader("NiClusterAccumulator");
        NiStream::UnregisterLoader("NiCollisionSwitch");
        NiStream::UnregisterLoader("NiColorExtraData");
        NiStream::UnregisterLoader("NiDefaultAVObjectPalette");
        NiStream::UnregisterLoader("NiDirectionalLight");
        NiStream::UnregisterLoader("NiDitherProperty");
        NiStream::UnregisterLoader("NiExtraData");
        NiStream::UnregisterLoader("NiFloatExtraData");
        NiStream::UnregisterLoader("NiFloatsExtraData");
        NiStream::UnregisterLoader("NiFogProperty");
        NiStream::UnregisterLoader("NiIntegerExtraData");
        NiStream::UnregisterLoader("NiIntegersExtraData");
        NiStream::UnregisterLoader("NiLODNode");
        NiStream::UnregisterLoader("NiLines");
        NiStream::UnregisterLoader("NiLinesData");
        NiStream::UnregisterLoader("NiMaterialProperty");
        NiStream::UnregisterLoader("NiNode");
        NiStream::UnregisterLoader("NiPalette");
        NiStream::UnregisterLoader("NiParticleMeshes");
        NiStream::UnregisterLoader("NiParticleMeshesData");
        NiStream::UnregisterLoader("NiParticles");
        NiStream::UnregisterLoader("NiParticlesData");
        NiStream::UnregisterLoader("NiPersistentSrcTextureRendererData");
        NiStream::UnregisterLoader("NiPixelData");
        NiStream::UnregisterLoader("NiPointLight");
        NiStream::UnregisterLoader("NiRangeLODData");
        NiStream::UnregisterLoader("NiRotatingParticles");
        NiStream::UnregisterLoader("NiRotatingParticlesData");
        NiStream::UnregisterLoader("NiScreenLODData");
        NiStream::UnregisterLoader("NiScreenTexture");
        NiStream::UnregisterLoader("NiShadeProperty");
        NiStream::UnregisterLoader("NiSkinData");
        NiStream::UnregisterLoader("NiSkinInstance");
        NiStream::UnregisterLoader("NiSkinPartition");
        NiStream::UnregisterLoader("NiSortAdjustNode");
        NiStream::UnregisterLoader("NiScreenElements");
        NiStream::UnregisterLoader("NiScreenElementsData");
        NiStream::UnregisterLoader("NiSourceTexture");
        NiStream::UnregisterLoader("NiSpecularProperty");
        NiStream::UnregisterLoader("NiSpotLight");
        NiStream::UnregisterLoader("NiStencilProperty");
        NiStream::UnregisterLoader("NiStringExtraData");
        NiStream::UnregisterLoader("NiStringsExtraData");
        NiStream::UnregisterLoader("NiSwitchNode");
        NiStream::UnregisterLoader("NiSwitchStringExtraData");
        NiStream::UnregisterLoader("NiTextureEffect");
        NiStream::UnregisterLoader("NiTexturingProperty");
        NiStream::UnregisterLoader("NiTriShape");
        NiStream::UnregisterLoader("NiTriShapeData");
        NiStream::UnregisterLoader("NiTriShapeDynamicData");
        NiStream::UnregisterLoader("NiTriStrips");
        NiStream::UnregisterLoader("NiTriStripsData");
        NiStream::UnregisterLoader("NiTriStripsDynamicData");
        NiStream::UnregisterLoader("NiVectorExtraData");
        NiStream::UnregisterLoader("NiVertexColorProperty");
        NiStream::UnregisterLoader("NiVertWeightsExtraData");
        NiStream::UnregisterLoader("NiScreenGeometry");
        NiStream::UnregisterLoader("NiScreenGeometryData");
        NiStream::UnregisterLoader("NiScreenPolygon");
        NiStream::UnregisterLoader("NiScreenSpaceCamera");
        NiShaderConstantMap::_SDMShutdown();
        NiTextureEffect::_SDMShutdown();
        NiAlphaProperty::_SDMShutdown();
        NiFogProperty::_SDMShutdown();
        NiMaterialProperty::_SDMShutdown();
        NiMetricsLayer::_SDMShutdown();
        NiShadeProperty::_SDMShutdown();
        NiStencilProperty::_SDMShutdown();
        NiTexturingProperty::_SDMShutdown();
        NiCullingProperty::_SDMShutdown();
        NiImageConverter::_SDMShutdown();
        NiStream::_SDMShutdown();
        NiRenderer::_SDMShutdown();
        NiScreenGeometryData::_SDMShutdown();
        NiTexture::RendererData::_SDMShutdown();
        NiPoint3::_SDMShutdown();
        NiMaterial::_SDMShutdown();
        NiSingleShaderMaterialLibrary::_SDMShutdown();
        NiMaterialLibrary::_SDMShutdown();
        NiTAbstractPoolAllocator<uint32_t>::_SDMShutdown();
        FreeMemoryBlocks();
        NiGlobalStringTable::_SDMShutdown();
    }
}

void NiTAbstractPoolAllocator<uint32_t>::_SDMShutdown() {
    NiTAbstractPoolAllocator<uint32_t>::AllocNode* v0 = NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader;
    if (NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader) {
        NiTAbstractPoolAllocator<uint32_t>::AllocNode* m_pkNext;
        do {
            m_pkNext = v0->m_pkNext;
            MemoryManager_Singleton.Deallocate(v0);
            v0 = m_pkNext;
        }
        while (m_pkNext);
    }
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = 0;
    NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader = 0;
}
