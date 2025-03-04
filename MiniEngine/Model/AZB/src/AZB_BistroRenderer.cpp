#include "AZB_BistroRenderer.h"
//===============================================================================
// desc: This is modelled after the SponzaRenderer namespace, allowing for Bistro to be rendered in a non-HDR environment using baked lights!
// auth: Aliyaan Zulfiqar
//===============================================================================
// 
// From Core
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "TemporalEffects.h"
#include "SSAO.h"
#include "SystemTime.h"
#include "ShadowCamera.h"
#include "ParticleEffects.h"
#include "SponzaRenderer.h"
#include "Renderer.h"
//===============================================================================
// 
// From Model
// [AZB]: Replace ModelH3D with glTF model class
#include "Model.h"
//===============================================================================
// 
// From ModelViewer
#include "LightManager.h"

#include "CompiledShaders/DepthViewerVS.h"
#include "CompiledShaders/DepthViewerPS.h"
#include "CompiledShaders/ModelViewerVS.h"
#include "CompiledShaders/ModelViewerPS.h"
//===============================================================================
#include "ConstantBuffers.h" // For GlobalConstants

using namespace Renderer;

using namespace Math;
using namespace Graphics;

#if AZB_MOD

namespace Bistro
{
    void RenderLightShadows(GraphicsContext& gfxContext, const Camera& camera, ModelInstance& model);

    enum eObjectFilter { kOpaque = 0x1, kCutout = 0x2, kTransparent = 0x4, kAll = 0xF, kNone = 0x0 };
    void RenderObjects(GraphicsContext& Context, ModelInstance& model, const Matrix4& ViewProjMat, const Vector3& viewerPos, eObjectFilter Filter = kAll);

    GraphicsPSO m_DepthPSO = { (L"Bistro: Depth PSO") };
    GraphicsPSO m_CutoutDepthPSO = { (L"Bistro: Cutout Depth PSO") };
    GraphicsPSO m_ModelPSO = { (L"Bistro: Color PSO") };
    GraphicsPSO m_CutoutModelPSO = { (L"Bistro: Cutout Color PSO") };
    GraphicsPSO m_ShadowPSO(L"Bistro: Shadow PSO");
    GraphicsPSO m_CutoutShadowPSO(L"Bistro: Cutout Shadow PSO");

    ModelInstance* m_Model;
    std::vector<bool> m_pMaterialIsCutout;

    Vector3 m_SunDirection;
    ShadowCamera m_SunShadow;
    std::vector<std::string> m_TextureNames;

    ExpVar m_AmbientIntensity("Bistro/Lighting/Ambient Intensity", 0.1f, -16.0f, 16.0f, 0.1f);
    ExpVar m_SunLightIntensity("Bistro/Lighting/Sun Light Intensity", 1.0f, 0.0f, 16.0f, 0.1f);
    NumVar m_SunOrientation("Bistro/Lighting/Sun Orientation", 50.0f, -100.0f, 100.0f, 0.1f);
    NumVar m_SunInclination("Bistro/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f);
    NumVar ShadowDimX("Bistro/Lighting/Shadow Dim X", 5000, 1000, 10000, 100);
    NumVar ShadowDimY("Bistro/Lighting/Shadow Dim Y", 3000, 1000, 10000, 100);
    NumVar ShadowDimZ("Bistro/Lighting/Shadow Dim Z", 3000, 1000, 10000, 100);
}

void Bistro::Startup(Math::Camera& camera, ModelInstance& model)
{
    DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT NormalFormat = g_SceneNormalBuffer.GetFormat();
    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();
    //DXGI_FORMAT ShadowFormat = g_ShadowBuffer.GetFormat();

    D3D12_INPUT_ELEMENT_DESC vertElem[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Depth-only (2x rate)
    m_DepthPSO.SetRootSignature(Renderer::m_RootSig);
    m_DepthPSO.SetRasterizerState(RasterizerDefault);
    m_DepthPSO.SetBlendState(BlendNoColorWrite);
    m_DepthPSO.SetDepthStencilState(DepthStateReadWrite);
    m_DepthPSO.SetInputLayout(_countof(vertElem), vertElem);
    m_DepthPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    m_DepthPSO.SetRenderTargetFormats(0, nullptr, DepthFormat);
    m_DepthPSO.SetVertexShader(g_pDepthViewerVS, sizeof(g_pDepthViewerVS));
    m_DepthPSO.Finalize();

    // Depth-only shading but with alpha testing
    m_CutoutDepthPSO = m_DepthPSO;
    m_CutoutDepthPSO.SetPixelShader(g_pDepthViewerPS, sizeof(g_pDepthViewerPS));
    m_CutoutDepthPSO.SetRasterizerState(RasterizerTwoSided);
    m_CutoutDepthPSO.Finalize();

    // Depth-only but with a depth bias and/or render only backfaces
    m_ShadowPSO = m_DepthPSO;
    m_ShadowPSO.SetRasterizerState(RasterizerShadow);
    m_ShadowPSO.SetRenderTargetFormats(0, nullptr, g_ShadowBuffer.GetFormat());
    m_ShadowPSO.Finalize();

    // Shadows with alpha testing
    m_CutoutShadowPSO = m_ShadowPSO;
    m_CutoutShadowPSO.SetPixelShader(g_pDepthViewerPS, sizeof(g_pDepthViewerPS));
    m_CutoutShadowPSO.SetRasterizerState(RasterizerShadowTwoSided);
    m_CutoutShadowPSO.Finalize();

    DXGI_FORMAT formats[2] = { ColorFormat, NormalFormat };

    // Full color pass
    m_ModelPSO = m_DepthPSO;
    m_ModelPSO.SetBlendState(BlendDisable);
    m_ModelPSO.SetDepthStencilState(DepthStateTestEqual);
    m_ModelPSO.SetRenderTargetFormats(2, formats, DepthFormat);
    m_ModelPSO.SetVertexShader(g_pModelViewerVS, sizeof(g_pModelViewerVS));
    m_ModelPSO.SetPixelShader(g_pModelViewerPS, sizeof(g_pModelViewerPS));
    m_ModelPSO.Finalize();

    m_CutoutModelPSO = m_ModelPSO;
    m_CutoutModelPSO.SetRasterizerState(RasterizerTwoSided);
    m_CutoutModelPSO.Finalize();

    // [AZB]: Use the passed in model we load from glTF
    m_Model = &model;

    // The caller of this function can override which materials are considered cutouts
    // [AZB]: Use this to find plant textures and indicate that they are cutouts
    int numTextures = m_TextureNames.size();
    m_pMaterialIsCutout.resize(numTextures);
    for (uint32_t i = 0; i < numTextures; ++i)
    {
        const auto mat = m_TextureNames[i];
        if (mat.find("Foliage") != std::string::npos ||
            mat.find("Glass") != std::string::npos)
        {
            m_pMaterialIsCutout[i] = true;
        }
        else
        {
            m_pMaterialIsCutout[i] = false;
        }
    }

    ParticleEffects::InitFromJSON(L"Sponza/particles.json");

    //float modelRadius = Length(m_Model->GetBoundingBox().GetDimensions()) * 0.5f;
    //const Vector3 eye = m_Model->GetBoundingBox().GetCenter() + Vector3(modelRadius * 0.5f, 0.0f, 0.0f);
    //camera.SetEyeAtUp(eye, Vector3(kZero), Vector3(kYUnitVector));

    //Lighting::CreateRandomLights(m_Model->GetBoundingBox().GetDimensions().GetMin(), m_Model->GetBoundingBox().GetMax());
}

const ModelInstance& Bistro::GetModel()
{
    return *Bistro::m_Model;
}

void Bistro::Cleanup(void)
{
    m_Model = nullptr;
    Lighting::Shutdown();
    TextureManager::Shutdown();
}

void Bistro::RenderObjects(GraphicsContext& gfxContext, ModelInstance& modelInst, const Matrix4& ViewProjMat, const Vector3& viewerPos, eObjectFilter Filter)
{
    struct VSConstants
    {
        Matrix4 modelToProjection;
        Matrix4 modelToShadow;
        XMFLOAT3 viewerPos;
    } vsConstants;
    vsConstants.modelToProjection = ViewProjMat;
    vsConstants.modelToShadow = m_SunShadow.GetShadowMatrix();
    XMStoreFloat3(&vsConstants.viewerPos, viewerPos);

    gfxContext.SetDynamicConstantBufferView(Renderer::kMeshConstants, sizeof(vsConstants), &vsConstants);

    __declspec(align(16)) uint32_t materialIdx = 0xFFFFFFFFul;

    // [AZB]: Shortcut for readability
    auto model = modelInst.GetModel();
    if (!model)
        return;

    // [AZB]: Pointer to start of data buffers!
    const auto& dataBufferStart = model->m_DataBuffer.GetGpuVirtualAddress();

    for (uint32_t meshIndex = 0; meshIndex < model->m_NumMeshes; meshIndex++)
    {
        const Mesh& mesh = reinterpret_cast<Mesh*>(model->m_MeshData.get())[meshIndex];

        gfxContext.SetDescriptorTable(Renderer::kMaterialSRVs, s_TextureHeap[mesh.srvTable]);
        gfxContext.SetDescriptorTable(kMaterialSamplers, s_SamplerHeap[mesh.samplerTable]);

        uint32_t VertexStride = mesh.vbStride;

        // [AZB]: Set correct index and vertex buffer according to mesh values!
        gfxContext.SetVertexBuffer(0, { dataBufferStart + mesh.vbOffset, mesh.vbSize, mesh.vbStride });
        gfxContext.SetIndexBuffer({ dataBufferStart + mesh.ibOffset, mesh.ibSize, (DXGI_FORMAT)mesh.ibFormat });


        for (uint32_t drawIdx = 0; drawIdx < mesh.numDraws; drawIdx++) {
            const Mesh::Draw& draw = mesh.draw[drawIdx];

            // Check for cutout materials and filter
            if (mesh.materialCBV != materialIdx)
            {
                if (m_pMaterialIsCutout[mesh.materialCBV] && !(Filter & kCutout) ||
                    !m_pMaterialIsCutout[mesh.materialCBV] && !(Filter & kOpaque))
                    continue;

                materialIdx = mesh.materialCBV;

                gfxContext.SetDynamicConstantBufferView(Renderer::kCommonCBV, sizeof(uint32_t), &materialIdx);
            }

            gfxContext.DrawIndexed(draw.primCount, draw.startIndex, draw.baseVertex);
        }

    }
}


void Bistro::RenderLightShadows(GraphicsContext& gfxContext, const Camera& camera, ModelInstance& modelInst)
{
    using namespace Lighting;

    ScopedTimer _prof(L"RenderLightShadows", gfxContext);

    static uint32_t LightIndex = 0;
    if (LightIndex >= MaxLights)
        return;

    m_LightShadowTempBuffer.BeginRendering(gfxContext);
    {
        gfxContext.SetPipelineState(m_ShadowPSO);
        RenderObjects(gfxContext, modelInst, m_LightShadowMatrix[LightIndex], camera.GetPosition(), kOpaque);
        gfxContext.SetPipelineState(m_CutoutShadowPSO);
        RenderObjects(gfxContext, modelInst, m_LightShadowMatrix[LightIndex], camera.GetPosition(), kCutout);
    }
    //m_LightShadowTempBuffer.EndRendering(gfxContext);

    gfxContext.TransitionResource(m_LightShadowTempBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
    gfxContext.TransitionResource(m_LightShadowArray, D3D12_RESOURCE_STATE_COPY_DEST);

    gfxContext.CopySubresource(m_LightShadowArray, LightIndex, m_LightShadowTempBuffer, 0);

    gfxContext.TransitionResource(m_LightShadowArray, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    ++LightIndex;
}

void Bistro::RenderScene(
    GraphicsContext& gfxContext,
    const Camera& camera,
    ModelInstance& model,
    const D3D12_VIEWPORT& viewport,
    const D3D12_RECT& scissor,
    bool skipDiffusePass,
    bool skipShadowMap)
{
    Renderer::UpdateGlobalDescriptors();

    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();

    float costheta = cosf(m_SunOrientation);
    float sintheta = sinf(m_SunOrientation);
    float cosphi = cosf(m_SunInclination * 3.14159f * 0.5f);
    float sinphi = sinf(m_SunInclination * 3.14159f * 0.5f);
    m_SunDirection = Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));

    __declspec(align(16)) struct
    {
        Vector3 sunDirection;
        Vector3 sunLight;
        Vector3 ambientLight;
        float ShadowTexelSize[4];

        float InvTileDim[4];
        uint32_t TileCount[4];
        uint32_t FirstLightIndex[4];

        uint32_t FrameIndexMod2;
    } psConstants;

    psConstants.sunDirection = m_SunDirection;
    psConstants.sunLight = Vector3(1.0f, 1.0f, 1.0f) * m_SunLightIntensity;
    psConstants.ambientLight = Vector3(1.0f, 1.0f, 1.0f) * m_AmbientIntensity;
    psConstants.ShadowTexelSize[0] = 1.0f / g_ShadowBuffer.GetWidth();
    psConstants.InvTileDim[0] = 1.0f / Lighting::LightGridDim;
    psConstants.InvTileDim[1] = 1.0f / Lighting::LightGridDim;
    psConstants.TileCount[0] = Math::DivideByMultiple(g_SceneColorBuffer.GetWidth(), Lighting::LightGridDim);
    psConstants.TileCount[1] = Math::DivideByMultiple(g_SceneColorBuffer.GetHeight(), Lighting::LightGridDim);
    psConstants.FirstLightIndex[0] = Lighting::m_FirstConeLight;
    psConstants.FirstLightIndex[1] = Lighting::m_FirstConeShadowedLight;
    psConstants.FrameIndexMod2 = FrameIndex;

    // Set the default state for command lists
    auto& pfnSetupGraphicsState = [&](void)
        {
            gfxContext.SetRootSignature(Renderer::m_RootSig);
            gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Renderer::s_TextureHeap.GetHeapPointer());
            gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, s_SamplerHeap.GetHeapPointer());
            // [AZB]: RED HERRING! Meshes will use their own index buffer!
            gfxContext.SetIndexBuffer(model.GetModel()->m_DataBuffer.IndexBufferView());
            gfxContext.SetIndexBuffer(model.GetModel()->m_DataBuffer.IndexBufferView());
           //gfxContext.SetVertexBuffer(0, m_Model->GetModel()->m_DataBuffer.VertexBufferView());
           //gfxContext.SetVertexBuffer(0, m_Model->GetModel()->m_DataBuffer.VertexBufferView());
        };

    pfnSetupGraphicsState();

    {
        RenderLightShadows(gfxContext, camera, model);
    
        {
            ScopedTimer _prof(L"Z PrePass", gfxContext);
    
            gfxContext.SetDynamicConstantBufferView(Renderer::kMaterialConstants, sizeof(psConstants), &psConstants);
    
            {
                ScopedTimer _prof2(L"Opaque", gfxContext);
                {
                    gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
                    gfxContext.ClearDepth(g_SceneDepthBuffer);
                    gfxContext.SetPipelineState(m_DepthPSO);
                    gfxContext.SetDepthStencilTarget(g_SceneDepthBuffer.GetDSV());
                    gfxContext.SetViewportAndScissor(viewport, scissor);
                }
                RenderObjects(gfxContext, model, camera.GetViewProjMatrix(), camera.GetPosition(), kOpaque);
            }
    
            {
                ScopedTimer _prof2(L"Cutout", gfxContext);
                {
                    gfxContext.SetPipelineState(m_CutoutDepthPSO);
                }
                RenderObjects(gfxContext, model, camera.GetViewProjMatrix(), camera.GetPosition(), kCutout);
            }
        }
    
        SSAO::Render(gfxContext, camera);
    
        if (!skipDiffusePass)
        {
            Lighting::FillLightGrid(gfxContext, camera);
    
            if (!SSAO::DebugDraw)
            {
                ScopedTimer _prof(L"Main Render", gfxContext);
                {
                    gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
                    gfxContext.TransitionResource(g_SceneNormalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
                    gfxContext.ClearColor(g_SceneColorBuffer);
                }
            }
        }
    
        if (!skipShadowMap)
        {
            if (!SSAO::DebugDraw)
            {
                pfnSetupGraphicsState();
                {
                    ScopedTimer _prof2(L"Render Shadow Map", gfxContext);
    
                    m_SunShadow.UpdateMatrix(-m_SunDirection, Vector3(0, -500.0f, 0), Vector3(ShadowDimX, ShadowDimY, ShadowDimZ),
                        (uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16);
    
                    g_ShadowBuffer.BeginRendering(gfxContext);
                    gfxContext.SetPipelineState(m_ShadowPSO);
                    RenderObjects(gfxContext, model, m_SunShadow.GetViewProjMatrix(), camera.GetPosition(), kOpaque);
                    gfxContext.SetPipelineState(m_CutoutShadowPSO);
                    RenderObjects(gfxContext, model, m_SunShadow.GetViewProjMatrix(), camera.GetPosition(), kCutout);
                    g_ShadowBuffer.EndRendering(gfxContext);
                }
            }
        }
    
        if (!skipDiffusePass)
        {
            if (!SSAO::DebugDraw)
            {
                if (SSAO::AsyncCompute)
                {
                    gfxContext.Flush();
                    pfnSetupGraphicsState();
    
                    // Make the 3D queue wait for the Compute queue to finish SSAO
                    g_CommandManager.GetGraphicsQueue().StallForProducer(g_CommandManager.GetComputeQueue());
                }
    
                {
                    ScopedTimer _prof2(L"Render Color", gfxContext);
    
                    gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
                    gfxContext.SetDescriptorTable(Renderer::kCommonSRVs, Renderer::m_CommonTextures);
                    gfxContext.SetDynamicConstantBufferView(Renderer::kMaterialConstants, sizeof(psConstants), &psConstants);
    
                    {
                        gfxContext.SetPipelineState(m_ModelPSO);
                        gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
                        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]{ g_SceneColorBuffer.GetRTV(), g_SceneNormalBuffer.GetRTV() };
                        gfxContext.SetRenderTargets(ARRAYSIZE(rtvs), rtvs, g_SceneDepthBuffer.GetDSV_DepthReadOnly());
                        gfxContext.SetViewportAndScissor(viewport, scissor);
                    }
                    RenderObjects(gfxContext, model, camera.GetViewProjMatrix(), camera.GetPosition(), Bistro::kOpaque);
    
                    gfxContext.SetPipelineState(m_CutoutModelPSO);
                    RenderObjects(gfxContext, model, camera.GetViewProjMatrix(), camera.GetPosition(), Bistro::kCutout);
                }
            }
        }
    }
}

#endif
