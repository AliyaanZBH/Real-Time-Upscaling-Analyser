//===============================================================================
// desc: A helper pixel shader to render motion vectors on screen
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "CommonRS.hlsli"
#include "ShaderUtility.hlsli"
#include "PresentRS.hlsli"
//===============================================================================

Texture2D<float3> MVTex : register(t0);
SamplerState MVSamplerState : register(s0);


[RootSignature(Common_RootSig)]
float3 main(float4 position : SV_Position, float2 uv : TexCoord) : SV_Target0
{
    //return MVTex.Sample(MVSamplerState, uv);
    
     // Fetch the motion vector at the current pixel
    float2 motionVector = MVTex.Sample(MVSamplerState, uv).xy;
    
    // A scale factor to make the motion vector big enough to visualize
    float arrowScale = 0.05; // Adjust as needed for visibility

    // Scale the motion vector to control the size of the arrow
    motionVector *=  arrowScale;
    
    // Create the shaft of the arrow (line)
    // We want to calculate a perpendicular vector to the motion direction for drawing the arrow body
    float2 perpendicular = float2(-motionVector.y, motionVector.x); // A vector perpendicular to the motion vector
    
    // We need to check for a region of pixels that form the shaft and the head
    float2 arrowHeadDirection = normalize(motionVector); // Arrow's direction
    float2 arrowBodyDirection = normalize(perpendicular);

    // Define the thickness of the arrow shaft (a bit of tolerance)
    float shaftThickness = 0.005;
    float arrowHeadSize = 0.02; // Size of the arrowhead

    // Conditions to check if the current pixel should belong to the shaft or tip
   // float4 arrowColor = float4(0.0, 0.5, 0.0, 0.5); // Default to semi-transparent semi-blue background
    float4 arrowColor = float4(0.0, 0.0, 0.0, 1.0); // Default to black
    
    // Checking for the shaft area (region around the line)
    if (abs(dot(uv - float2(0.5, 0.5), arrowBodyDirection)) < shaftThickness)
    {
        arrowColor = float4(1.0, 0.0, 0.0, 1.0); // Red shaft
    }
    
    // Checking for the arrowhead area (region forming the tip of the arrow)
    if (abs(dot(uv - float2(0.5, 0.5), arrowHeadDirection)) < arrowHeadSize)
    {
        arrowColor = float4(0.0, 1.0, 0.0, 1.0); // Green arrowhead
    }

    // Return the color of the arrow or the background color
    return arrowColor;
}
