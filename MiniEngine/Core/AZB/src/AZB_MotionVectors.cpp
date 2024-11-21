//===============================================================================
// desc: A helper namespace to generate per-pixel motion vectors! The existing implementation in MiniEngine only generates camera velocity
// auth: Aliyaan Zulfiqar
//===============================================================================
 //
 // Additional includes

#include "pch.h"        // Need this - get 15000 errors otherwise when including BufferManager... Weird bug!
#include "AZB_MotionVectors.h"

#include "Camera.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "GraphicsCommon.h" // For root signature
#include "TemporalEffects.h"
#include "PostEffects.h"
#include "EngineProfiling.h"
#include "SystemTime.h"

//===============================================================================
//
// Compiled shaders

#include "CompiledShaders/AZB_DecodeMotionVectorsCS.h"
#include "CompiledShaders/AZB_PerPixelMotionVectorsCS.h"
#include "CompiledShaders/AZB_MotionVectorRenderPS.h"

#include "CompiledShaders/ScreenQuadCommonVS.h" // Allows us to render a quad to the screen for graphics!

namespace MotionVectors
{
    // Define PSOs
    ComputePSO s_AZB_DecodeMotionVectorsCS(L"DLSS: Camera Motion Vector Decode CS");
    ComputePSO s_AZB_PerPixelMotionVectorsCS(L"DLSS: Per-Pixel Motion Vector Creation CS");
    GraphicsPSO s_AZB_MotionVectorRenderPS(L"RTUA: Render Motion Vectors PS");
}

void MotionVectors::Initialize(void)
{
    // Create Compute PSOs using the method found in MotionBlur.cpp and more!
#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetRootSignature(Graphics::g_CommonRS); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

    // Create PSO using our compiled shader
    CreatePSO(s_AZB_DecodeMotionVectorsCS, g_pAZB_DecodeMotionVectorsCS);
    CreatePSO(s_AZB_PerPixelMotionVectorsCS, g_pAZB_PerPixelMotionVectorsCS);
#undef CreatePSO

    // Graphics PSO needs setting up in a more traditional way
    s_AZB_MotionVectorRenderPS.SetRootSignature(Graphics::g_CommonRS);
    s_AZB_MotionVectorRenderPS.SetRasterizerState(Graphics::RasterizerTwoSided);
    s_AZB_MotionVectorRenderPS.SetBlendState(Graphics::BlendPreMultiplied);
    s_AZB_MotionVectorRenderPS.SetDepthStencilState(Graphics::DepthStateDisabled);
    s_AZB_MotionVectorRenderPS.SetSampleMask(0xFFFFFFFF);
    s_AZB_MotionVectorRenderPS.SetInputLayout(0, nullptr);
    s_AZB_MotionVectorRenderPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    // Set our vertex shader up to render a quad to the screen
    s_AZB_MotionVectorRenderPS.SetVertexShader(g_pScreenQuadCommonVS, sizeof(g_pScreenQuadCommonVS));
    s_AZB_MotionVectorRenderPS.SetPixelShader(g_pAZB_MotionVectorRenderPS, sizeof(g_pAZB_MotionVectorRenderPS));
    s_AZB_MotionVectorRenderPS.SetRenderTargetFormat(Graphics::g_MotionVectorRTBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
    s_AZB_MotionVectorRenderPS.Finalize();

}

void MotionVectors::Shutdown(void)
{
    
}

void MotionVectors::GeneratePerPixelMotionVectors(CommandContext& BaseContext, const Math::Camera& camera)
{
   ScopedTimer _outerprof(L"Generate Per-Pixel Motion Vectors for DLSS", BaseContext);
   
   ComputeContext& Context = BaseContext.GetComputeContext();
   
   // Set common root signature, this has the correct slots for the bindings we need
   Context.SetRootSignature(Graphics::g_CommonRS);
   
   // Get the width and height of the main frame
   uint32_t Width = Graphics::g_SceneColorBuffer.GetWidth();
   uint32_t Height = Graphics::g_SceneColorBuffer.GetHeight();

    {
        // Take the finished camera motion vectors and decode them
        ScopedTimer _prof(L"Decode Camera Velocity Buffer", BaseContext);

        // Set pipeline state to the one needed for my new shader
        Context.SetPipelineState(s_AZB_DecodeMotionVectorsCS);

        // Transition packed MV buffer so that it can be read by the CS
        Context.TransitionResource(Graphics::g_VelocityBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        // Transition output buffer so that it can be written to
        Context.TransitionResource(Graphics::g_DecodedVelocityBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Set descriptors to upload data to the shader
        Context.SetDynamicDescriptor(1, 0, Graphics::g_VelocityBuffer.GetSRV());
        Context.SetDynamicDescriptor(2, 0, Graphics::g_DecodedVelocityBuffer.GetUAV());

        // Fire off the compute shader!
        Context.Dispatch2D(Width, Height);

        // Transition main packed buffer back to UAV
        Context.TransitionResource(Graphics::g_VelocityBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    // Now generate per-pixel motion vectors
    {
        ScopedTimer _prof2(L"Generate Per-Pixel Motion Vectors", BaseContext);

        // Repeat steps for this compute shader
        Context.SetPipelineState(s_AZB_PerPixelMotionVectorsCS);

        // Set up some extra constants that we need for calculating the previous world-space position for each pixel in the current frame by transforming from clip space

        // Method found from another source
        Math::Matrix4 preMult = Math::Invert(camera.GetProjMatrix());
        Math::Matrix4 postMult = camera.GetViewMatrix();

        //float RcpHalfDimX = 2.0f / Width;
        //float RcpHalfDimY = 2.0f / Height;
        //float nearClip = camera.GetNearClip();
        //float RcpZMagic = nearClip / (camera.GetFarClip()  - nearClip);
        //
        //Math::Matrix4 preMult = Math::Matrix4(
        //    Math::Vector4(RcpHalfDimX, 0.0f, 0.0f, 0.0f),
        //    Math::Vector4(0.0f, -RcpHalfDimY, 0.0f, 0.0f),
        //    Math::Vector4(0.0f, 0.0f, RcpZMagic, 0.0f),
        //    Math::Vector4(-1.0f, 1.0f, -RcpZMagic, 1.0f)
        //);
        //
        //Math::Matrix4 postMult = Math::Matrix4(
        //    Math::Vector4(1.0f / RcpHalfDimX, 0.0f, 0.0f, 0.0f),
        //    Math::Vector4(0.0f, -1.0f / RcpHalfDimY, 0.0f, 0.0f),
        //    Math::Vector4(0.0f, 0.0f, 1.0f, 0.0f),
        //    Math::Vector4(1.0f / RcpHalfDimX, 1.0f / RcpHalfDimY, 0.0f, 1.0f));

        // This is the actual value we want to upload, and represents the transformation that took place in the previous frame
        Math::Matrix4 CurToPrevXForm = postMult * camera.GetReprojectionMatrix() * preMult;

        // Used to calculate current world pos
        Math::Matrix4 ViewProjMatrixInverse = Math::Invert(camera.GetViewProjMatrix());

        __declspec(align(16)) struct ConstantBuffer
        {
            Math::Matrix4 m_ViewProjMatrixInverse;    // To get the current world position of a pixel
            Math::Matrix4 m_CurToPrevXForm;           // To caluclate previous world position
            uint32_t m_SceneWidth, m_SceneHeight;

        };

        ConstantBuffer cbv = {
            ViewProjMatrixInverse,
            CurToPrevXForm,
            Width, Height
        };

        // Set binding and upload to GPU for our compute shader - CBs go last though!
        Context.SetDynamicConstantBufferView(3, sizeof(cbv), &cbv);

        // DEPTH BUFFER IS EMPTY BY THIS POINT!
        //
        // Using the depth buffer as an input instead, as we are not simply decoding, but in fact generating wholly new MVs
        //Context.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        // Using linear depth as that is what other post steps use, e.g. motion blur!
        // Using index mod2 so that we don't crash by using the current frame!
        ColorBuffer& LinearDepth = Graphics::g_LinearDepth[TemporalEffects::GetFrameIndexMod2()];
        Context.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        // The actual velocity data goes here
        Context.TransitionResource(Graphics::g_PerPixelMotionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        // A render target to visualise movement!
        Context.TransitionResource(Graphics::g_MotionVectorVisualisationBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        Context.SetDynamicDescriptor(1, 0, Graphics::g_SceneDepthBuffer.GetDepthSRV());
        Context.SetDynamicDescriptor(2, 0, Graphics::g_PerPixelMotionBuffer.GetUAV());
        Context.SetDynamicDescriptor(2, 1, Graphics::g_MotionVectorVisualisationBuffer.GetUAV());

        Context.Dispatch2D(Width, Height);

        // Transition visual buffer to SRV so we can use it in ImGui!
        Context.TransitionResource(Graphics::g_PerPixelMotionBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(Graphics::g_MotionVectorVisualisationBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    }
}

void MotionVectors::Render()
{
   GraphicsContext& Context = GraphicsContext::Begin(L"Render Motion Vectors");
   
   Context.SetRootSignature(Graphics::g_CommonRS);
   Context.SetPipelineState(s_AZB_MotionVectorRenderPS);

   // Set up the visualisation buffer as a texture
   //Context.SetDynamicDescriptor(1, 0, Graphics::g_MotionVectorVisualisationBuffer.GetSRV());

   // Pass in MV as texture to sample
   Context.TransitionResource(Graphics::g_PerPixelMotionBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
   Context.SetDynamicDescriptor(1, 0, Graphics::g_PerPixelMotionBuffer.GetSRV());

   //// Pass in Visualisation as output
   //Context.TransitionResource(Graphics::g_MotionVectorVisualisationBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
   //Context.SetDynamicDescriptor(1, 0, Graphics::g_MotionVectorVisualisationBuffer.GetUAV());

   
   // Extra step here as sometimes the topology type is different
   Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   
   // Render it to main scene color buffer
   Context.TransitionResource(Graphics::g_MotionVectorRTBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
   Context.ClearColor(Graphics::g_MotionVectorRTBuffer);
   Context.SetRenderTarget(Graphics::g_MotionVectorRTBuffer.GetRTV());
   Context.SetViewportAndScissor(0, 0, Graphics::g_MotionVectorRTBuffer.GetWidth(), Graphics::g_MotionVectorRTBuffer.GetHeight());
   Context.Draw(3);
   
   // Transition to SRV and copy dest for ImGui to use!
   Context.TransitionResource(Graphics::g_MotionVectorRTBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
   Context.Finish();
}
