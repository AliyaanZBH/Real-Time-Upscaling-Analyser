//===============================================================================
// desc: A helper pixel shader to render motion vectors on screen
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "CommonRS.hlsli"
#include "ShaderUtility.hlsli"
#include "PresentRS.hlsli"
//===============================================================================

Texture2D<float3> ColorTex : register(t0);
SamplerState MVSamplerState : register(s0);


[RootSignature(Common_RootSig)]
float3 main(float4 position : SV_Position, float2 uv : TexCoord) : SV_Target0
{
    return ColorTex.Sample(MVSamplerState, uv);
}
