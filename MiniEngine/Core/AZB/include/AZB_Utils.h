#pragma once
//===============================================================================
// desc: A collection of constants and utilities to aid all aspects of code modification
// auth: Aliyaan Zulfiqar
//===============================================================================

// A macro to clearly identify my contributions to the starting code
#define AZB_MOD 1	// Change to 0 to exclude my modifications and run unmodified sample code
#define AZB_DBG 0	// Another flag for me to quickly test and debug certain GUI functions and more!
#include "dxgiformat.h"
#include "stdint.h"

// This is defined in Display.cpp
constexpr int SWAP_CHAIN_BUFFER_COUNT = 3;
constexpr DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;

// Used to adjust window size in ImGui due to the extra size added by the windows title
constexpr int kWindowTitleX = 16;
constexpr int kWindowTitleY = 39;

// Set in GameCore.cpp
extern bool g_bMouseExclusive;	// Used to keep track of if the mouse is being exclusively used by the MiniEngine. Set to 0 by default, app starts non-exclusive

//
//	Helper structs to organise data
//

struct Resolution
{
	uint32_t m_Width = 0u;
	uint32_t m_Height = 0u;
};


// Used in ModelViewer.cpp to dictate size of model array
constexpr int kNumScenes = 2;
