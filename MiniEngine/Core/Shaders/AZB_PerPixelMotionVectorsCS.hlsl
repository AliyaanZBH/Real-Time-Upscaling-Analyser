//===============================================================================
// desc: A more complete compute shader to that generates per-pixel motion vectors from scratch.
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "CommonRS.hlsli"
#include "PixelPacking_Velocity.hlsli"
//===============================================================================

//
// Data
//

Texture2D DepthBuffer : register(t0);
RWTexture2D<float4> VelocityBuffer : register(u0);
RWTexture2D<float4> OutputVisualiserBuffer : register(u1);          // Creating a texture to help with visual debugging


cbuffer FrameConstants : register(b1)
{
    matrix InverseViewProj; // Necessary to accurately reconstruct the world position of the current pixel, starting from clip space using the current frame's depth
    matrix CurToPrevXForm; // Used to get the previous world position by transforming the current clip-space position to the previous frame's world space
    uint ViewportWidth;
    uint ViewportHeight;
}


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


    // First get the pixel in NDC space
    float2 ndc = float2(
        (currentPixel.x / ViewportWidth) * 2.0 - 1.0,
        1.0 - (currentPixel.y / ViewportHeight) * 2.0);
    
    // Use NDC and the depth value to get accurate clipPos
    float4 clipPos = float4(ndc /** depth*/, depth, 1.0f);
    
    // Reconstruct the current world-space position using the inverse view-projection matrix
    float4 worldPosCurrent = mul(clipPos, InverseViewProj);

    // Reproject to the previous frame's world-space position using CurToPrevXForm
    float4 worldPosPrev = mul(clipPos, CurToPrevXForm);

    // Calculate per-pixel motion - the difference between the current and previous world positions
    float2 motionVector = worldPosCurrent.xy - worldPosPrev.xy;

    // Write to motion vector buffer
    VelocityBuffer[currentPixel] = float4(motionVector, 0.0f, 1.0f);
    
    // For visualisation, encode motion as color
    float colorMagnitude = length(motionVector);
    float colorAngle = atan2(motionVector.y, motionVector.x);
    
    // Send this to our output buffer, normalising the angle first before storing the direction in the red channel, and magnitude in the green channel.
    OutputVisualiserBuffer[currentPixel] = float4(colorAngle / (2.0f * 3.14159265359f), colorMagnitude, 0.0f, 1.0f);
}
