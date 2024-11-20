//===============================================================================
// desc: A helper compute shader to get motion vectors into the correct format for DLSS. Also serves as my first ever CS, hence the excessive comments to show my learning :)
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "CommonRS.hlsli"
#include "PixelPacking_Velocity.hlsli"
//===============================================================================

//
// Data
//

cbuffer FrameConstants : register(b0)
{
    matrix InverseViewProj; // Necessary to accurately reconstruct the world position of the current pixel, starting from clip space using the current frame's depth
    matrix CurToPrevXForm;  // Used to get the previous world position by transforming the current clip-space position to the previous frame's world space
    uint ViewportWidth;    
    uint ViewportHeight;    
}

Texture2D DepthBuffer : register(t0);
RWTexture2D<float4> VelocityBuffer : register(u0);


//
//  Entry point
//

// Set root signature for this shader
[RootSignature(Common_RootSig)]
// Using 8,8,1 as that is what the cameraVelocityCS uses. Investigate if other values are better!
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 currentPixel = DTid.xy;
    
    // Fetch the depth for the current pixel so that we can reconstruct the world-space position
    float depth = DepthBuffer.Load(uint3(currentPixel, 0)).r;

    // Reconstruct world position
    float2 ndc = float2(
        (currentPixel.x / ViewportWidth) * 2.0 - 1.0,
        1.0 - (currentPixel.y / ViewportHeight) * 2.0); // Flip Y-axis
    float4 clipPos = float4(ndc * depth, depth, 1.0);
    float4 worldPos = 

    // Transform world position to previous frame
    float4 prevClipPos =
    prevClipPos /= prevClipPos.w;

    // Calculate per-pixel motion
    float2 motionVector = ndc - prevClipPos.xy;

    // Write to motion vector buffer
    VelocityBuffer[DTid.xy] = float4(motionVector, 0.0f, 1.0f);
}
