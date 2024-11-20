//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once

#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ShadowBuffer.h"
#include "GpuBuffer.h"
#include "GraphicsCore.h"

//===============================================================================
// desc: This is a fancy manager for memory, going to try and integrate a buffer for ImGui here
// modified: Aliyaan Zulfiqar
//===============================================================================
#include "AZB_Utils.h"

/*
   Change Log:

   [AZB] 16/10/24: Created buffers for ImGui
*/

namespace Graphics
{
    extern DepthBuffer g_SceneDepthBuffer;  // D32_FLOAT_S8_UINT
    extern ColorBuffer g_SceneColorBuffer;  // R11G11B10_FLOAT
    extern ColorBuffer g_SceneNormalBuffer; // R16G16B16A16_FLOAT
    extern ColorBuffer g_PostEffectsBuffer; // R32_UINT (to support Read-Modify-Write with a UAV)
    extern ColorBuffer g_OverlayBuffer;     // [AZB:] Previouisly R8G8B8A8_UNORM, now R10G10B10A2_UNORM
    extern ColorBuffer g_HorizontalBuffer;  // For separable (bicubic) upsampling

#if AZB_MOD
    extern ColorBuffer g_ImGuiBuffer;                           // [AZB]: For ImGui R10G10B10A2_UNORM
    extern ColorBuffer g_DLSSOutputBuffer;                      // [AZB]: For DLSS to Upscale to R10G10B10A2_UNORM
    extern ColorBuffer g_PerPixelMotionBuffer;                  // [AZB]: For DLSS to use, per-pixel motion vectors, to R32G32_FLOAT
    extern ColorBuffer g_MotionVectorVisualisationBuffer;       // [AZB]: For debugging per-pixel motion vectors, a texture that can be displayed 
    extern ColorBuffer g_MotionVectorRTBuffer;                  // [AZB]: For debugging per-pixel motion vectors, a render target R8G8B8A8
    extern ColorBuffer g_DecodedVelocityBuffer;                 // [AZB]: For DLSS to use in addition to per-pixel MVs, decoded from g_VelocityBuffer, to R32G32_FLOAT
#endif
    // [AZB]: This only stores camera velocity, which is not enough for DLSS! We need per-pixel motion vectors
    extern ColorBuffer g_VelocityBuffer;    // R10G10B10  (3D velocity)
    extern ShadowBuffer g_ShadowBuffer;

    extern ColorBuffer g_SSAOFullScreen;	// R8_UNORM
    extern ColorBuffer g_LinearDepth[2];	// Normalized planar distance (0 at eye, 1 at far plane) computed from the SceneDepthBuffer
    extern ColorBuffer g_MinMaxDepth8;		// Min and max depth values of 8x8 tiles
    extern ColorBuffer g_MinMaxDepth16;		// Min and max depth values of 16x16 tiles
    extern ColorBuffer g_MinMaxDepth32;		// Min and max depth values of 16x16 tiles
    extern ColorBuffer g_DepthDownsize1;
    extern ColorBuffer g_DepthDownsize2;
    extern ColorBuffer g_DepthDownsize3;
    extern ColorBuffer g_DepthDownsize4;
    extern ColorBuffer g_DepthTiled1;
    extern ColorBuffer g_DepthTiled2;
    extern ColorBuffer g_DepthTiled3;
    extern ColorBuffer g_DepthTiled4;
    extern ColorBuffer g_AOMerged1;
    extern ColorBuffer g_AOMerged2;
    extern ColorBuffer g_AOMerged3;
    extern ColorBuffer g_AOMerged4;
    extern ColorBuffer g_AOSmooth1;
    extern ColorBuffer g_AOSmooth2;
    extern ColorBuffer g_AOSmooth3;
    extern ColorBuffer g_AOHighQuality1;
    extern ColorBuffer g_AOHighQuality2;
    extern ColorBuffer g_AOHighQuality3;
    extern ColorBuffer g_AOHighQuality4;

    extern ColorBuffer g_DoFTileClass[2];
    extern ColorBuffer g_DoFPresortBuffer;
    extern ColorBuffer g_DoFPrefilter;
    extern ColorBuffer g_DoFBlurColor[2];
    extern ColorBuffer g_DoFBlurAlpha[2];
    extern StructuredBuffer g_DoFWorkQueue;
    extern StructuredBuffer g_DoFFastQueue;
    extern StructuredBuffer g_DoFFixupQueue;

    extern ColorBuffer g_MotionPrepBuffer;		// R10G10B10A2
    extern ColorBuffer g_LumaBuffer;
    extern ColorBuffer g_TemporalColor[2];
    extern ColorBuffer g_TemporalMinBound;
    extern ColorBuffer g_TemporalMaxBound;

    extern ColorBuffer g_aBloomUAV1[2];		// 640x384 (1/3)
    extern ColorBuffer g_aBloomUAV2[2];		// 320x192 (1/6)  
    extern ColorBuffer g_aBloomUAV3[2];		// 160x96  (1/12)
    extern ColorBuffer g_aBloomUAV4[2];		// 80x48   (1/24)
    extern ColorBuffer g_aBloomUAV5[2];		// 40x24   (1/48)
    extern ColorBuffer g_LumaLR;
    extern ByteAddressBuffer g_Histogram;
    extern ByteAddressBuffer g_FXAAWorkQueue;
    extern TypedBuffer g_FXAAColorQueue;

    void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight );
    void ResizeDisplayDependentBuffers(uint32_t NativeWidth, uint32_t NativeHeight);
    void DestroyRenderingBuffers();

} // namespace Graphics
