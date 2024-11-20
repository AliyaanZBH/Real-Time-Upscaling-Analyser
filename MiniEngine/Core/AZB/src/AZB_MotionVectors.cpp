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

namespace MotionVectors
{
    // Define PSOs
    ComputePSO s_AZB_DecodeMotionVectorsCS(L"DLSS: Camera Motion Vector Decode CS");
    ComputePSO s_AZB_PerPixelMotionVectorsCS(L"DLSS: Per-Pixel Motion Vector Creation CS");
}

void MotionVectors::Initialize(void)
{
    // Create PSOs using the method found in MotionBlur.cpp and more!
#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetRootSignature(Graphics::g_CommonRS); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

    // Create PSO using our compiled shader
    CreatePSO(s_AZB_DecodeMotionVectorsCS, g_pAZB_DecodeMotionVectorsCS);
    CreatePSO(s_AZB_PerPixelMotionVectorsCS, g_pAZB_PerPixelMotionVectorsCS);
#undef CreatePSO

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

        // Using the depth buffer as an input instead, as we are not simply decoding, but in fact generating wholly new MVs
        Context.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(Graphics::g_PerPixelMotionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        Context.SetDynamicDescriptor(1, 0, Graphics::g_SceneDepthBuffer.GetDepthSRV());
        Context.SetDynamicDescriptor(2, 0, Graphics::g_PerPixelMotionBuffer.GetUAV());

        Context.Dispatch2D(Width, Height);

        Context.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
}