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

//===============================================================================
// desc: This is a helper namespace that mainly deals with the swapchain and resolution.
// modified: Aliyaan Zulfiqar
//===============================================================================

/*
   Change Log:
   [AZB] 16/10/24: Tweaked swapchain data to allow it to be accessed by ImGui
   [AZB] 21/10/24: Passing DLSS into this namespace as part of preliminary integration into rendering pipeline.
   [AZB] 22/10/24: Querying DLSS optimal settings to begin feature creation
   [AZB] 22/10/24: DLSS implementation continued, pipeline now rendering at optimal lower resolution for upscaling!
   [AZB] 23/10/24: DLSS creation continued, decided it was more appropriate to postpone creation until after the rest of the engine is initalised as GraphicsContext requires some timing to be setup
*/

#include "pch.h"
#include "Display.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "ColorBuffer.h"
#include "SystemTime.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "ImageScaling.h"
#include "TemporalEffects.h"


#pragma comment(lib, "dxgi.lib") 

// This macro determines whether to detect if there is an HDR display and enable HDR10 output.
// Currently, with HDR display enabled, the pixel magnfication functionality is broken.
#define CONDITIONALLY_ENABLE_HDR_OUTPUT 1


//
// [AZB]: Custom includes and macro mods
//

#if AZB_MOD
#include "AZB_DLSS.h"
#endif


namespace GameCore { extern HWND g_hWnd; }

#include "CompiledShaders/ScreenQuadPresentVS.h"
#include "CompiledShaders/BufferCopyPS.h"
#include "CompiledShaders/PresentSDRPS.h"
#include "CompiledShaders/PresentHDRPS.h"
#include "CompiledShaders/CompositeSDRPS.h"
#include "CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "CompiledShaders/CompositeHDRPS.h"
#include "CompiledShaders/BlendUIHDRPS.h"
#include "CompiledShaders/ScaleAndCompositeHDRPS.h"
#include "CompiledShaders/MagnifyPixelsPS.h"
#include "CompiledShaders/GenerateMipsLinearCS.h"
#include "CompiledShaders/GenerateMipsLinearOddCS.h"
#include "CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "CompiledShaders/GenerateMipsLinearOddYCS.h"
#include "CompiledShaders/GenerateMipsGammaCS.h"
#include "CompiledShaders/GenerateMipsGammaOddCS.h"
#include "CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "CompiledShaders/GenerateMipsGammaOddYCS.h"

#define SWAP_CHAIN_BUFFER_COUNT 3
DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;

using namespace Math;
using namespace ImageScaling;
using namespace Graphics;

namespace
{
    float s_FrameTime = 0.0f;
    uint64_t s_FrameIndex = 0;
    int64_t s_FrameStartTick = 0;

    // [AZB]: TODO Figure out how to correctly disable VSync!
    // 
    // [AZB]: We need VSync disabled in the final app but this is not (atleast not the ONLY) place to change it
    //        When this is set to false, we get a warning from D3D: 
    //          D3D12 WARNING: ID3D12CommandList::Dispatch: No threads will be dispatched, because at least one of {ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ} is 0.
    //          [ EXECUTION WARNING #1254: EMPTY_DISPATCH]

    BoolVar s_EnableVSync("Timing/VSync", true);
    BoolVar s_LimitTo30Hz("Timing/Limit To 30Hz", false);
    BoolVar s_DropRandomFrames("Timing/Drop Random Frames", false);
}

namespace Graphics
{
    void PreparePresentSDR();
    void PreparePresentHDR();
    void CompositeOverlays( GraphicsContext& Context );

    enum eResolution { k720p, k900p, k1080p, k1440p, k1800p, k2160p };
    enum eEQAAQuality { kEQAA1x1, kEQAA1x8, kEQAA1x16 };

    const uint32_t kNumPredefinedResolutions = 6;

    const char* ResolutionLabels[] = { "1280x720", "1600x900", "1920x1080", "2560x1440", "3200x1800", "3840x2160" };
    EnumVar NativeResolution("Graphics/Display/Native Resolution", k1080p, kNumPredefinedResolutions, ResolutionLabels);
#ifdef _GAMING_DESKTOP
    // This can set the window size to common dimensions.  It's also possible for the window to take on other dimensions
    // through resizing or going full-screen.
    EnumVar DisplayResolution("Graphics/Display/Display Resolution", k1080p, kNumPredefinedResolutions, ResolutionLabels);
#endif

    bool g_bEnableHDROutput = false;
    NumVar g_HDRPaperWhite("Graphics/Display/Paper White (nits)", 200.0f, 100.0f, 500.0f, 50.0f);
    NumVar g_MaxDisplayLuminance("Graphics/Display/Peak Brightness (nits)", 1000.0f, 500.0f, 10000.0f, 100.0f);
    const char* HDRModeLabels[] = { "HDR", "SDR", "Side-by-Side" };
    EnumVar HDRDebugMode("Graphics/Display/HDR Debug Mode", 0, 3, HDRModeLabels);

    uint32_t g_NativeWidth = 0;
    uint32_t g_NativeHeight = 0;
    uint32_t g_DisplayWidth = 1920;
    uint32_t g_DisplayHeight = 1080;
    ColorBuffer g_PreDisplayBuffer;

// [AZB]: Extra values to keep track of DLSS values globally
#if AZB_MOD

    // [AZB]: These will be evaluated when the swapchain gets created in Display::Initialise(), which is when the DLSS Query will be called
    uint32_t g_DLSSWidth = 0;
    uint32_t g_DLSSHeight = 0;

#endif

    // [AZB]: Take in an eResolution, and store the underlying width and height values inside two external variables that are also passed in
    void ResolutionToUINT(eResolution res, uint32_t& width, uint32_t& height)
    {
        switch (res)
        {
        default:
        case k720p:
            width = 1280;
            height = 720;
            break;
        case k900p:
            width = 1600;
            height = 900;
            break;
        case k1080p:
            width = 1920;
            height = 1080;
            break;
        case k1440p:
            width = 2560;
            height = 1440;
            break;
        case k1800p:
            width = 3200;
            height = 1800;
            break;
        case k2160p:
            width = 3840;
            height = 2160;
            break;
        }
    }

    // [AZB]: Original function for setting the pipeline native resolution
    void SetNativeResolution(void)
    {
        uint32_t NativeWidth, NativeHeight;

        ResolutionToUINT(eResolution((int)NativeResolution), NativeWidth, NativeHeight);

        if (g_NativeWidth == NativeWidth && g_NativeHeight == NativeHeight)
            return;
        DEBUGPRINT("Changing native resolution to %ux%u", NativeWidth, NativeHeight);

        g_NativeWidth = NativeWidth;
        g_NativeHeight = NativeHeight;

        g_CommandManager.IdleGPU();

        InitializeRenderingBuffers(NativeWidth, NativeHeight);
    }

    void SetDisplayResolution(void)
    {
#ifdef _GAMING_DESKTOP
        static int SelectedDisplayRes = DisplayResolution;
        if (SelectedDisplayRes == DisplayResolution)
            return;

        SelectedDisplayRes = DisplayResolution;
        ResolutionToUINT((eResolution)SelectedDisplayRes, g_DisplayWidth, g_DisplayHeight);
        DEBUGPRINT("Changing display resolution to %ux%u", g_DisplayWidth, g_DisplayHeight);

        g_CommandManager.IdleGPU();

        Display::Resize(g_DisplayWidth, g_DisplayHeight);

        SetWindowPos(GameCore::g_hWnd, 0, 0, 0, g_DisplayWidth, g_DisplayHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
    }

    ColorBuffer g_DisplayPlane[SWAP_CHAIN_BUFFER_COUNT];
    UINT g_CurrentBuffer = 0;

    IDXGISwapChain1* s_SwapChain1 = nullptr;

    RootSignature s_PresentRS;
    GraphicsPSO s_BlendUIPSO(L"Core: BlendUI");
    GraphicsPSO s_BlendUIHDRPSO(L"Core: BlendUIHDR");
    GraphicsPSO PresentSDRPS(L"Core: PresentSDR");
    GraphicsPSO PresentHDRPS(L"Core: PresentHDR");
    GraphicsPSO CompositeSDRPS(L"Core: CompositeSDR");
    GraphicsPSO ScaleAndCompositeSDRPS(L"Core: ScaleAndCompositeSDR");
    GraphicsPSO CompositeHDRPS(L"Core: CompositeHDR");
    GraphicsPSO ScaleAndCompositeHDRPS(L"Core: ScaleAndCompositeHDR");
    GraphicsPSO MagnifyPixelsPS(L"Core: MagnifyPixels");

    const char* FilterLabels[] = { "Bilinear", "Sharpening", "Bicubic", "Lanczos" };
    EnumVar UpsampleFilter("Graphics/Display/Scaling Filter", kSharpening, kFilterCount, FilterLabels);

    enum DebugZoomLevel { kDebugZoomOff, kDebugZoom2x, kDebugZoom4x, kDebugZoom8x, kDebugZoom16x, kDebugZoomCount };
    const char* DebugZoomLabels[] = { "Off", "2x Zoom", "4x Zoom", "8x Zoom", "16x Zoom" };
    EnumVar DebugZoom("Graphics/Display/Magnify Pixels", kDebugZoomOff, kDebugZoomCount, DebugZoomLabels);
}


// [AZB]: The new version which takes in a value that we pass in, used for DLSS and regular downscaling!
#if AZB_MOD
void Display::SetPipelineResolution(bool bDLSS, uint32_t queriedWidth, uint32_t queriedHeight, bool bFullscreen)
{
    g_CommandManager.IdleGPU();

    if (bDLSS)
    {
        // [AZB]: Updating the print statement to signal that DLSS is responsible
        DEBUGPRINT("Changing native resolution to match DLSS query result %ux%u", queriedWidth, queriedHeight);
    }
    else
    {
        DEBUGPRINT("Changing internal resolution to %ux%u", queriedWidth, queriedHeight);
    }

    // [AZB]: Still update the existing native global as it may be used in other places of the pipeline that DLSS doesn't touch (e.g. post-effects)
    // [AZB]: If we are working in fullscreen, don't change the native width and height as we want to maintain the size of the swap chain
    //if (bFullscreen)
    //{
        //g_NativeWidth = queriedWidth;
        //g_NativeHeight = queriedHeight;
    //}
    //if (bDLSS)
    //{
    // 
    // [AZB]: Also update our DLSS globals

    //g_DLSSWidth = queriedWidth;
    //g_DLSSHeight = queriedHeight;
    //}

    // [AZB]: Extra pause here to ensure there is no synchronisation issues
    g_CommandManager.IdleGPU();

    // [AZB]: If we are resizing DLSS, use the bespoke version to init rendering buffers
    if (bDLSS)
        InitializeRenderingBuffersDLSS(queriedWidth, queriedHeight, g_DLSSWidth, g_DLSSHeight);
    else
        InitializeRenderingBuffers(queriedWidth, queriedHeight);
}

void Display::SetDLSSInputResolution(uint32_t queriedWidth, uint32_t queriedHeight)
{
    ResizeDLSSInputBuffer(queriedWidth, queriedHeight);
}
#endif

void Display::Resize(uint32_t width, uint32_t height)
{
    g_CommandManager.IdleGPU();

    g_DisplayWidth = width;
    g_DisplayHeight = height;

    DEBUGPRINT("Changing display resolution to %ux%u", width, height);

// [AZB]: Create DLSS - repeat steps in Initialise() but also release the old feature and create a new one
#if AZB_MOD

    DLSS::Release();

    // [AZB]: Even when DLSS is disabled, query the settings here so that we can safely and corectly enable later!
    DLSS::PreQueryAllSettings(g_DisplayWidth, g_DisplayHeight);
   
    // [AZB]: Set the queried results here
    g_DLSSWidth = DLSS::m_DLSS_Modes[DLSS::m_CurrentQualityMode].m_RenderWidth;
    g_DLSSHeight = DLSS::m_DLSS_Modes[DLSS::m_CurrentQualityMode].m_RenderHeight;

    // [AZB]: These steps are similar to those we perform in GraphicsCore.cpp, Initialize()
    ComputeContext& Context = ComputeContext::Begin(L"DLSS Resize");
    // [AZB]: Fill in requirements struct ready for the feature creation
    DLSS::CreationRequirements reqs;
    reqs.m_pCmdList = Context.GetCommandList();

    NVSDK_NGX_Feature_Create_Params dlssParams = { g_DLSSWidth, g_DLSSHeight, g_DisplayWidth, g_DisplayHeight, static_cast<NVSDK_NGX_PerfQuality_Value>(DLSS::m_CurrentQualityMode) };
    // [AZB]: Even though we may not render to HDR, our color buffer is infact in HDR format, so set the appropriate flag!
    reqs.m_DlSSCreateParams = NVSDK_NGX_DLSS_Create_Params{ dlssParams, NVSDK_NGX_DLSS_Feature_Flags_None /*| NVSDK_NGX_DLSS_Feature_Flags_IsHDR*/ };
    DLSS::Create(reqs);

    Context.Finish();

    //// [AZB]: Additional flag so that we can toggle DLSS at run-time
    //if (DLSS::m_DLSS_Enabled)
    //{
    //    // [AZB]: Resize internal buffers to use the lower resolution that DLSS will upscale from
    //    SetPipelineResolution(true, g_DLSSWidth, g_DLSSHeight);
    //    // [AZB]: Recreate this buffer with DLSS data
    //    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", g_DLSSWidth, g_DLSSHeight, 1, SwapChainFormat);
    //}
    //else
    //{
        // [AZB]: Original pre-buffer creation
        g_PreDisplayBuffer.Create(L"PreDisplay Buffer", width, height, 1, SwapChainFormat);
    //}

#else
    // [AZB]: Original pre-buffer creation
    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", width, height, 1, SwapChainFormat);
#endif

    // [AZB]: Continue with regular swap chain creation, this should be unaffected by DLSS as it our render targets will eventually upscale up to here
    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        g_DisplayPlane[i].Destroy();

    ASSERT(s_SwapChain1 != nullptr);
    ASSERT_SUCCEEDED(s_SwapChain1->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, SwapChainFormat, 0));

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        ComPtr<ID3D12Resource> DisplayPlane;
        ASSERT_SUCCEEDED(s_SwapChain1->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane)));
        // [AZB]: This does not need to match DLSS as it should match the display buffer
        g_DisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
    }

    g_CurrentBuffer = 0;

    g_CommandManager.IdleGPU();

    // [AZB]: This does not need to match DLSS as it should match the display buffer
    ResizeDisplayDependentBuffers(g_NativeWidth, g_NativeHeight);

    // [AZB]: Use this opportunity to resize DLSS input buffer though!
#if AZB_MOD
    ResizeDLSSInputBuffer(g_DLSSWidth, g_DLSSHeight);
#endif
}

// Initialize the DirectX resources required to run.
void Display::Initialize(void)
{
    ASSERT(s_SwapChain1 == nullptr, "Graphics has already been initialized");

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    ASSERT_SUCCEEDED(CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = g_DisplayWidth;
    swapChainDesc.Height = g_DisplayHeight;
    swapChainDesc.Format = SwapChainFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;
#if AZB_MOD
    // [AZB]: Allows for stretching in fullscreen
    fsSwapChainDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
#endif
    ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(
        g_CommandManager.GetCommandQueue(),
        GameCore::g_hWnd,
        &swapChainDesc,
        &fsSwapChainDesc,
        nullptr,
        &s_SwapChain1));

#if CONDITIONALLY_ENABLE_HDR_OUTPUT
    {
        IDXGISwapChain4* swapChain = (IDXGISwapChain4*)s_SwapChain1;
        ComPtr<IDXGIOutput> output;
        ComPtr<IDXGIOutput6> output6;
        DXGI_OUTPUT_DESC1 outputDesc;
        UINT colorSpaceSupport;

        // Query support for ST.2084 on the display and set the color space accordingly
        if (SUCCEEDED(swapChain->GetContainingOutput(&output)) && SUCCEEDED(output.As(&output6)) &&
            SUCCEEDED(output6->GetDesc1(&outputDesc)) && outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
            SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
            (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
            SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
        {
            g_bEnableHDROutput = true;
        }
    }
#endif // End CONDITIONALLY_ENABLE_HDR_OUTPUT

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        ComPtr<ID3D12Resource> DisplayPlane;
        ASSERT_SUCCEEDED(s_SwapChain1->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane)));
        g_DisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
    }

    {
        s_PresentRS.Reset(4, 2);
        s_PresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
        s_PresentRS[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
        s_PresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
        s_PresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
        s_PresentRS.InitStaticSampler(0, SamplerLinearClampDesc);
        s_PresentRS.InitStaticSampler(1, SamplerPointClampDesc);
        s_PresentRS.Finalize(L"Present");

        // Initialize PSOs
        s_BlendUIPSO.SetRootSignature(s_PresentRS);
        s_BlendUIPSO.SetRasterizerState(RasterizerTwoSided);
        s_BlendUIPSO.SetBlendState(BlendPreMultiplied);
        s_BlendUIPSO.SetDepthStencilState(DepthStateDisabled);
        s_BlendUIPSO.SetSampleMask(0xFFFFFFFF);
        s_BlendUIPSO.SetInputLayout(0, nullptr);
        s_BlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        s_BlendUIPSO.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
        s_BlendUIPSO.SetPixelShader(g_pBufferCopyPS, sizeof(g_pBufferCopyPS));
        s_BlendUIPSO.SetRenderTargetFormat(SwapChainFormat, DXGI_FORMAT_UNKNOWN);
        s_BlendUIPSO.Finalize();

        s_BlendUIHDRPSO = s_BlendUIPSO;
        s_BlendUIHDRPSO.SetPixelShader(g_pBlendUIHDRPS, sizeof(g_pBlendUIHDRPS));
        s_BlendUIHDRPSO.Finalize();
    }
#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName = s_BlendUIPSO; \
    ObjName.SetBlendState( BlendDisable ); \
    ObjName.SetPixelShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

    CreatePSO(PresentSDRPS, g_pPresentSDRPS);
    CreatePSO(CompositeSDRPS, g_pCompositeSDRPS);
    CreatePSO(ScaleAndCompositeSDRPS, g_pScaleAndCompositeSDRPS);
    CreatePSO(CompositeHDRPS, g_pCompositeHDRPS);
    CreatePSO(ScaleAndCompositeHDRPS, g_pScaleAndCompositeHDRPS);
    CreatePSO(MagnifyPixelsPS, g_pMagnifyPixelsPS);

    PresentHDRPS = PresentSDRPS;
    PresentHDRPS.SetPixelShader(g_pPresentHDRPS, sizeof(g_pPresentHDRPS));
    DXGI_FORMAT SwapChainFormats[2] = { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM };
    PresentHDRPS.SetRenderTargetFormats(2, SwapChainFormats, DXGI_FORMAT_UNKNOWN );
    PresentHDRPS.Finalize();

#undef CreatePSO



// [AZB]: Continue DLSS intialisation after creating main swap chain by querying DLSS modes and their optimal settings to determine the resolution of our render targets
#if AZB_MOD
  
    // [AZB]: We figured out the maximum native fullscreen resolution inside GraphicsCore.cpp, pass this up to Display
    g_NativeWidth = DLSS::m_CurrentNativeResolution.m_Width;
    g_NativeHeight = DLSS::m_CurrentNativeResolution.m_Height;

    //g_DisplayWidth = g_NativeWidth;
    //g_DisplayHeight = g_NativeHeight;

    // [AZB]: Container that will store results of query that will be needed for DLSS feature creation. By default this will check the balanced setting
    //DLSS::OptimalSettings dlssSettings;

    // [AZB]: Query optimal settings based on current native resolution
    //DLSS::QueryOptimalSettings(g_DisplayWidth, g_DisplayHeight, dlssSettings);

    // [AZB]: Pre-query all settings for current native resolution
    DLSS::PreQueryAllSettings(g_DisplayWidth, g_DisplayHeight);

    // [AZB] Set the initial value of DLSS res to the balanced one
    g_DLSSWidth = DLSS::m_DLSS_Modes[1].m_RenderWidth;
    g_DLSSHeight = DLSS::m_DLSS_Modes[1].m_RenderHeight;

    // [AZB]: At this point we could create DLSS however we can't create a graphics context just yet as the rest of the engine needs to initalise first. 
    //        DLSS creation is therefore postponed until after these steps.

    // [AZB]: Call my version of setNativeRes, which sets the resolution for all buffers in the pipeline! (smaller buffers work off of divisions of this total size)
    //        It also sets the internal resolution for DLSS
    SetPipelineResolution(true, g_DisplayWidth, g_DisplayHeight);

    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", g_DisplayWidth, g_DisplayHeight, 1, SwapChainFormat);
    ImageScaling::Initialize(g_PreDisplayBuffer.GetFormat());
#else
    // [AZB]: This is where native resolution gets set originally, we need to override this with DLSS recommended lower resolution
    SetNativeResolution();

    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", g_DisplayWidth, g_DisplayHeight, 1, SwapChainFormat);
    ImageScaling::Initialize(g_PreDisplayBuffer.GetFormat());
#endif

}

void Display::Shutdown( void )
{
    s_SwapChain1->SetFullscreenState(FALSE, nullptr);
    s_SwapChain1->Release();

    for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        g_DisplayPlane[i].Destroy();

    g_PreDisplayBuffer.Destroy();
}

void Graphics::PreparePresentHDR(void)
{
    GraphicsContext& Context = GraphicsContext::Begin(L"Present");

    bool NeedsScaling = g_NativeWidth != g_DisplayWidth || g_NativeHeight != g_DisplayHeight;

    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, 0, g_SceneColorBuffer.GetSRV());

    ColorBuffer& Dest = DebugZoom == kDebugZoomOff ? g_DisplayPlane[g_CurrentBuffer] : g_PreDisplayBuffer;

    // On Windows, prefer scaling and compositing in one step via pixel shader
    Context.SetRootSignature(s_PresentRS);
    Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (DebugZoom == kDebugZoomOff)
    {
        Context.SetDynamicDescriptor(0, 1, g_OverlayBuffer.GetSRV());
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeHDRPS : CompositeHDRPS);
    }
    else
    {
        Context.SetDynamicDescriptor(0, 1, GetDefaultTexture(kBlackTransparent2D));
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeHDRPS : PresentHDRPS);
    }
    Context.SetConstants(1, (float)g_HDRPaperWhite / 10000.0f, (float)g_MaxDisplayLuminance,
        0.7071f / g_NativeWidth, 0.7071f / g_NativeHeight);
    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(Dest.GetRTV());
    Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
    Context.Draw(3);

    // Magnify without stretching
    if (DebugZoom != kDebugZoomOff)
    {
        Context.SetPipelineState(MagnifyPixelsPS);
        Context.TransitionResource(g_PreDisplayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV());
        Context.SetDynamicDescriptor(0, 0, g_PreDisplayBuffer.GetSRV());
        Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
        Context.SetConstants(1, 1.0f / ((int)DebugZoom + 1.0f));
        Context.Draw(3);

        CompositeOverlays(Context);
    }

    Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);

    // Close the final context to be executed before frame present.
    Context.Finish();
}

void Graphics::CompositeOverlays( GraphicsContext& Context )
{
    // Now blend (or write) the UI overlay
    Context.SetRootSignature(s_PresentRS);
    Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, 0, g_OverlayBuffer.GetSRV());
    Context.SetPipelineState(g_bEnableHDROutput ? s_BlendUIHDRPSO : s_BlendUIPSO);
    Context.SetConstants(1, (float)g_HDRPaperWhite / 10000.0f, (float)g_MaxDisplayLuminance);
    Context.Draw(3);
}

void Graphics::PreparePresentSDR(void)
{
    GraphicsContext& Context = GraphicsContext::Begin(L"Present");

    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#if AZB_MOD
    //[AZB]: Additional check as DLSS may be toggled off
    if (DLSS::m_DLSS_Enabled)
    {
        // [AZB]: Our color buffer is was downscaled and used as an input for DLSS, so instead read from the DLSS output!
        Context.TransitionResource(g_DLSSOutputBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
        Context.SetDynamicDescriptor(0, 0, g_DLSSOutputBuffer.GetSRV());
    }
    else
    {
        // [AZB]: Use the regular, non-upscaled colour buffer
        Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 0, g_SceneColorBuffer.GetSRV());
    }
#else
    // We're going to be reading these buffers to write to the swap chain buffer(s)
    Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | 
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, 0, g_SceneColorBuffer.GetSRV());
#endif

    bool NeedsScaling = g_NativeWidth != g_DisplayWidth || g_NativeHeight != g_DisplayHeight;

    // On Windows, prefer scaling and compositing in one step via pixel shader
    if (DebugZoom == kDebugZoomOff && (UpsampleFilter == kSharpening || !NeedsScaling))
    {
//#if AZB_MOD
//        // [AZB]: Set both imgui buffer and existing UI buffer descriptors
//        Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
//        Context.TransitionResource(g_ImGuiBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
//        const D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { g_OverlayBuffer.GetSRV(), g_ImGuiBuffer.GetSRV() };
//        Context.SetDynamicDescriptors(0, 1, 2, handles);
//#else
        // [AZB]: Original descriptor set for UI overlay
        Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 1, g_OverlayBuffer.GetSRV());
//#endif
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeSDRPS : CompositeSDRPS);
        Context.SetConstants(1, 0.7071f / g_NativeWidth, 0.7071f / g_NativeHeight);
        Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV());
        Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
        Context.Draw(3);
    }
    else
    {
        ColorBuffer& Dest = DebugZoom == kDebugZoomOff ? g_DisplayPlane[g_CurrentBuffer] : g_PreDisplayBuffer;

        // Scale or Copy
        if (NeedsScaling)
        {
            ImageScaling::Upscale(Context, Dest, g_SceneColorBuffer, eScalingFilter((int)UpsampleFilter));
        }
        else
        {
            Context.SetPipelineState(PresentSDRPS);
            Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetRenderTarget(Dest.GetRTV());
            Context.SetViewportAndScissor(0, 0, g_NativeWidth, g_NativeHeight);
            Context.Draw(3);
        }

        // Magnify without stretching
        if (DebugZoom != kDebugZoomOff)
        {
            Context.SetPipelineState(MagnifyPixelsPS);
            Context.TransitionResource(g_PreDisplayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV());
            Context.SetDynamicDescriptor(0, 0, g_PreDisplayBuffer.GetSRV());
            Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
            Context.SetConstants(1, 1.0f / ((int)DebugZoom + 1.0f));
            Context.Draw(3);
        }

        CompositeOverlays(Context);
    }


    Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);

    // Close the final context to be executed before frame present.
    Context.Finish();
}

void Display::Present(void)
{
    if (g_bEnableHDROutput)
        PreparePresentHDR();
    else
        PreparePresentSDR();

    UINT PresentInterval = s_EnableVSync ? std::min(4, (int)Round(s_FrameTime * 60.0f)) : 0;

    s_SwapChain1->Present(PresentInterval, 0);

    g_CurrentBuffer = (g_CurrentBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

    // Test robustness to handle spikes in CPU time
    //if (s_DropRandomFrames)
    //{
    //	if (std::rand() % 25 == 0)
    //		BusyLoopSleep(0.010);
    //}

    int64_t CurrentTick = SystemTime::GetCurrentTick();

    if (s_EnableVSync)
    {
        // With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
        // to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
        // long the previous frame should be displayed (i.e. the present interval.)
        s_FrameTime = (s_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
        if (s_DropRandomFrames)
        {
            if (std::rand() % 50 == 0)
                s_FrameTime += (1.0f / 60.0f);
        }
    }
    else
    {
        // When running free, keep the most recent total frame time as the time step for
        // the next frame simulation.  This is not super-accurate, but assuming a frame
        // time varies smoothly, it should be close enough.
        s_FrameTime = (float)SystemTime::TimeBetweenTicks(s_FrameStartTick, CurrentTick);
    }

    s_FrameStartTick = CurrentTick;

    ++s_FrameIndex;

    TemporalEffects::Update((uint32_t)s_FrameIndex);
    
#if AZB_MOD
   // [AZB]: Resize according to DLSS
   //if (DLSS::m_DLSS_Enabled)
   //     SetPipelineResolution(true, g_DisplayWidth, g_DisplayHeight);
   // else
   //    SetPipelineResolution(false, g_DisplayWidth, g_DisplayHeight);
#else

    // [AZB]: Original call here to resize internal rendering resolution
    SetNativeResolution();
#endif
    SetDisplayResolution();
}


#if AZB_MOD

// [AZB]: This function is essentially a copy of SetDisplayResolution at the top, but with a passed in resolution, and stripping away some of the extra steps it was taken
void Display::SetWindowedResolution(uint32_t width, uint32_t height)
{
   g_CommandManager.IdleGPU();

   // [AZB]: If DLSS is active, the resize here will break. So, we need to check for this, then release DLSS and recreate it!
   if (DLSS::m_DLSS_Enabled)
   {
   }

   // [AZB]: The offsets here are necessary to account for the windows title bar
   //          NB: This function triggers WM_SIZE which in turn calls Display::Resize
   SetWindowPos(GameCore::g_hWnd, 0, 0, 0, width + kWindowTitleX, height + kWindowTitleY, SWP_NOZORDER| SWP_NOACTIVATE);
}

IDXGISwapChain1* Display::GetSwapchain()
{
    return s_SwapChain1;
}

#endif

uint64_t Graphics::GetFrameCount(void)
{
    return s_FrameIndex;
}

float Graphics::GetFrameTime(void)
{
    return s_FrameTime;
}

float Graphics::GetFrameRate(void)
{
    return s_FrameTime == 0.0f ? 0.0f : 1.0f / s_FrameTime;
}
