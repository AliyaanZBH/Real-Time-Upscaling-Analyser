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
//===============================================================================
// desc: This is a helper namespace that mainly deals with the swapchain
// modified: Aliyaan Zulfiqar
//===============================================================================

/*
   Change Log:
   [AZB] 16/10/24: Tweaked swapchain data to allow it to be accessed by ImGui
*/


#include <cstdint>

//
// [AZB]: Custom includes
//

// [AZB]: Container file for code modifications and other helper tools. Contains the global "AZB_MOD" macro.
#include "AZB_Utils.h"

#if AZB_MOD
// [AZB]: Forward declares
class ColorBuffer;
#endif
namespace Display
{
    void Initialize(void);
    void Shutdown(void);
    void Resize(uint32_t width, uint32_t height);
    void Present(void);

#if AZB_MOD
    // [AZB]: Making this function public so that resolution can be changed in ImGui
    void SetResolution(uint32_t width, uint32_t height);
    // [AZB]: Some extra accessors to further ease GUI usability when it comes to controlling rendering
    IDXGISwapChain1* GetSwapchain();
#endif
}

namespace Graphics
{
    extern uint32_t g_DisplayWidth;
    extern uint32_t g_DisplayHeight;
    extern bool g_bEnableHDROutput;

#if AZB_MOD

    // [AZB]: These will be evaluated when the swapchain gets created in Display::Initialise(), which is when the DLSS Query will be called
    extern uint32_t g_DLSSWidth;
    extern uint32_t g_DLSSHeight;

    // [AZB]: These variables already exist but they aren't made external. We need these for DLSS.
    extern ColorBuffer g_DisplayPlane[];
    extern UINT g_CurrentBuffer;
#endif

    // Returns the number of elapsed frames since application start
    uint64_t GetFrameCount(void);

    // The amount of time elapsed during the last completed frame.  The CPU and/or
    // GPU may be idle during parts of the frame.  The frame time measures the time
    // between calls to present each frame.
    float GetFrameTime(void);

    // The total number of frames per second
    float GetFrameRate(void);

    extern bool g_bEnableHDROutput;
}
