#include "AZB_MotionVectors.h"
//===============================================================================
// desc: A helper namespace to generate per-pixel motion vectors! The existing implementation in MiniEngine only generates camera velocity
// auth: Aliyaan Zulfiqar
//===============================================================================
// Additional includes

#include "BufferManager.h"
#include "CommandContext.h"
#include "GraphicsCommon.h" // For root signature

//===============================================================================
// Compiled shaders

#include "CompiledShaders/AZB_DecodeMotionVectorsCS.h"

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
#undef CreatePSO

}

void MotionVectors::Shutdown(void)
{
}

void MotionVectors::GeneratePerPixelMotionVectors(ComputeContext& Context)
{

}
