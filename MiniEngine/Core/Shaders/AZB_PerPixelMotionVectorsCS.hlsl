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
RWTexture2D<float4> OutputVisualiserBuffer : register(u1);          // Creating a texture to help with visual debugging by generating arrows!


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
    // Get the center of the current pixel
    uint2 currentPixel = (DTid.xy + 0.5);
    //uint2 currentPixel = DTid.xy;
        
    // Fetch the depth for the current pixel so that we can reconstruct the world-space position - using Linear Depth now!
    float depth = DepthBuffer[currentPixel];

    // First get the pixel in NDC space
    float2 ndcCoords = float2(
        (currentPixel.x / ViewportWidth) * 2.0 - 1.0,
        1.0 - (currentPixel.y / ViewportHeight) * 2.0);
        
    //float4 clipPos = float4(ndc * depth, depth, 1.0f);
    
    // Use NDC and the depth value to get accurate world position
    float4 ndcPos = float4(ndcCoords * depth, depth, 1.0f);
    
    // Homogeneous point - multiply with linear depth
    //float4 clipPos = float4(currentPixel * depth, depth, 1.0f);
    
    // Reconstruct the current world-space position using the inverse view-projection matrix
    //float4 worldPosCurrent = mul(clipPos, InverseViewProj);
    float4 worldPosCurrent = mul(ndcPos, InverseViewProj);
    
    // Divide out the perspective to get correct world space position
    //worldPosCurrent.xyz /= worldPosCurrent.w;
    ////worldPosCurrent.z = worldPosCurrent.w;
    //
    // Reproject to the previous frame's world-space position using CurToPrevXForm.
    float4 worldPosPrev = mul(worldPosCurrent, CurToPrevXForm);
    worldPosPrev.xyz /= worldPosPrev.w;
    //worldPosPrev.z = worldPosPrev.w;
    
    //float4 worldPosPrev = mul(clipPos, CurToPrevXForm);
    //float4 worldPosPrev = mul(worldPosPrev, InverseViewProj);
   
    // Calculate per-pixel motion - the difference between the current and previous world positions
    float3 motionVector = worldPosCurrent.xyz - worldPosPrev.xyz;

    // Write to motion vector buffer
    VelocityBuffer[DTid.xy] = float4(motionVector, 1.0f);
    //VelocityBuffer[currentPixel] = PackVelocity(PrevHPos.xyz - float3(CurPixel, Depth)
    
    // For visualisation, encode motion as color
   // float colorMagnitude = length(motionVector);
   // float colorAngle = atan2(motionVector.y, motionVector.x);
   // 
   // // Send this to our output buffer, normalising the angle first before storing the direction in the red channel, and magnitude in the green channel.
   // OutputVisualiserBuffer[currentPixel] = float4(colorAngle / (2.0f * 3.14159265359f), colorMagnitude, 0.0f, 1.0f);
    
    
         // Fetch the motion vector at the current pixel
   //float2 motionVector = MVTex.Sample(MVSamplerState, uv).xy;
    
    
    //
    //  Begin rendering arrows
    //
    
    // Move the pixel pivot from the center back to the top left
    currentPixel = DTid.xy;
    
    // Use normalised pixel coordinates for better precision across resolutions, good for GBuffer displays!
    //currentPixel = DTid.xy / float2(ViewportWidth, ViewportHeight);
    
    // A scale factor to make the motion vector big enough to visualize
    float arrowScale = 1; // Adjust as needed for visibility

    // Scale the motion vector to control the size of the arrow
    motionVector *= arrowScale;
    
    // Create the shaft of the arrow (line)
    // We want to calculate a perpendicular vector to the motion direction for drawing the arrow body
    float2 perpendicular = float2(-motionVector.y, motionVector.x); // A vector perpendicular to the motion vector
    
    // We need to check for a region of pixels that form the shaft and the head
    float2 arrowHeadDirection = normalize(motionVector); // Arrow's direction
    float2 arrowBodyDirection = normalize(perpendicular);

    // Define the thickness of the arrow shaft (a bit of tolerance)
    float shaftThickness = 0.2;
    float arrowHeadSize = 0.002; // Size of the arrowhead

    // Conditions to check if the current pixel should belong to the shaft or tip
    float4 arrowColor = float4(0.0, 0.0, 0.1, 0.1); // Default to semi-transparent semi-blue background
   //float4 arrowColor = float4(0.0, 0.0, 0.0, 1.0); // Default to black
    
    // Checking for the shaft area (region around the line)
    if (abs(dot(currentPixel - float2(0.5, 0.5), arrowBodyDirection)) < shaftThickness)
    {
        arrowColor = float4(1.0, 0.0, 0.0, 1.0); // Red shaft
    }
    
    // Checking for the arrowhead area (region forming the tip of the arrow)
    if (abs(dot(currentPixel - float2(0.5, 0.5), arrowHeadDirection)) < arrowHeadSize)
    {
        arrowColor = float4(0.0, 1.0, 0.0, 1.0); // Green arrowhead
    }
    
    // Compare pixel positions relative to the shaft and arrowhead size
    //if (abs(dot(DTid.xy - float2(0.5, 0.5), perpendicular)) < shaftThickness)
    //{
    //    arrowColor = float4(1.0, 0.0, 0.0, 1.0); // Red shaft
    //}
    //
    //// Check if the pixel is near the arrowhead
    //if (length(DTid.xy - (DTid.xy + motionVector.xy)) < arrowHeadSize)
    //{
    //    arrowColor = float4(0.0, 1.0, 0.0, 1.0); // Green arrowhead
    //}
    
    OutputVisualiserBuffer[currentPixel] = arrowColor;

}
