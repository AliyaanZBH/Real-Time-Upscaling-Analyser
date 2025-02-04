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

// This is the original packed velocity buffer, which should have been written to in cameraVelocityCS.hlsl!
Texture2D<uint> PackedVelocityBuffer : register(t0); // SRV, input only

// This is where we are going to output our decoded buffers, which is a custom buffer created in BufferManager, and will be passed to DLSS.
RWTexture2D<float2> DecodedMotionVectors : register(u0); // UAV, we are writing to this buffer.


//
//  Entry point
//

// Set root signature for this shader
[RootSignature(Common_RootSig)]
// Using 8,8,1 as that is what the cameraVelocityCS uses. Investigate if other values are better!
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Get the current pixel
    uint2 pixel = DTid.xy;
    
    // Read the packed motion vectors for this pixel
    uint packedMV = PackedVelocityBuffer[pixel];
    
    // Unpack it - thank you MiniEngine for this function!
    float3 unpackedMV = UnpackVelocity(packedMV);
    
     // Store only the X and Y components for DLSS - the motion vectors are 3D but we don't need Z!
    DecodedMotionVectors[pixel] = unpackedMV.xy;
}
