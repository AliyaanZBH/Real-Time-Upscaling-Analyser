#pragma once
//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "imgui_internal.h" // For ImLerp

#include "implot.h" // For our lovely graphs!

#include "AZB_Utils.h"

#include <string>   // For GBuffer LUT
#include <array>  // For GBuffer array
#include <d3d12.h>  // For GBuffer array
//===============================================================================

class CommandContext;
class Model;

namespace RTUA_GUI
{
	// Get ImGui started!
	void Init(void* Hwnd, ID3D12Device* pDevice, int numFramesInFlight, const DXGI_FORMAT& renderTargetFormat);

	// Run ImGui render loop!
	void Run(CommandContext& ImGuiContext);

    // Our GUI can control resolution and other pipeline state at runtime, however we don't want to be messing about with this while frames are in flight!
    // This function will be called at the start of the update loop of the next frame, submitting any changes we made on the previous one.
    void UpdateGraphics();

	// Shutdown ImGui safely
	void Terminate();



    //
    //
    //  Namespace Members
    //
    //
    // 

    
    //
    //  Key GUI Members
    //  

	// A handle to render ImGui within MiniEngine
	extern ID3D12DescriptorHeap* m_pSrvDescriptorHeap;

    // Handle to global device - needed to update descriptor heap!
    extern ID3D12Device* m_pD3DDevice;

    // Pointer to the scene we will be investigating!
    extern const Model* m_pScene;

    // Values that represent the new resolution to change to next frame (if the user requests a change!)
    extern uint32_t m_NewWidth;
    extern uint32_t m_NewHeight;

    // The size of the the title bar in windowed mode - this was originally the total window resolution but this led to memory leaks when resizing!
    extern Resolution m_TitleBarSize;

    // Key Flags
    //

    // Bool that tracks when GUI, and by proxy the rest of the app, is fully setup. Used to ensure smooth alt+tabbing and startup behaviour across devices!
    extern bool m_bReady;

    // Public flag to enable/disable tonemapping!
    extern bool m_bEnablePostFX;

    // Public flag to enable/disable Motion Vector visualisation!
    extern bool m_bEnableMotionVisualisation;

    // Flag to show startup modal
    extern bool m_bShowStartupModal;

    // Flag to track display mode and adjust behaviour accordingly
    extern bool m_bFullscreen;
    // Flag to indicate that display mode should change next frame
    extern bool m_bDisplayModeChangePending;

    //
    //  Rendering mode selection helpers
    //

    enum eRenderingMode : uint8_t
    {
        NATIVE,
        BILINEAR_UPSCALE,
        DLSS,
        NUM_RENDER_MODES
    };

    extern std::string m_RenderModeNames[eRenderingMode::NUM_RENDER_MODES];

    // Tracker for rendering mode
    extern eRenderingMode m_CurrentRenderingMode;
    // Track last mode so that we can reset any state
    extern  eRenderingMode m_PreviousRenderingMode;

    // Track scaling factor
    extern float m_ScalingFactor;

    // Tracker for bilinear upscale input, so it can be saved and returned to when swapping modes
    extern Resolution m_BilinearInputRes;

    // Flag to indicate that the resolution should change next frame
    extern bool m_bResolutionChangePending;
    
    // Flag to indicate that common graphics state needs re-initalising next frame e.g. due to a change in mip bias for texture sampling
    extern bool m_bCommonStateChangePending;
    // Flag that tracks the checkbox which controls LOD bias
    extern bool m_bOverrideLodBias;
    // Actual value that can be controlled by the user to determine a custom LOD bias
    extern float m_ForcedLodBias;
    // Original LOD bias before overriding
    extern float m_OriginalLodBias;


    // Flag to indicate when a change has been made to DLSS
    extern bool m_bDLSSUpdatePending;
    // Flag to control when to enable/disable DLSS
    extern bool m_bToggleDLSS;
    // Flag to indicate that the requested change is to DLSS mode
    extern bool m_bUpdateDLSSMode;


    // Checkbox that controls whether to show metrics window
    extern bool m_ShowHardwareMetrics;
    extern bool m_ShowFrameRate;

    // Counter variable to track "pages" on the tutorial
    extern uint8_t m_Page;

    //
    // GBuffer handling
    //

    enum eGBuffers : uint8_t
    {
        SCENE_COLOR,
        SCENE_DEPTH,
        MOTION_VECTORS,
        VISUAL_MOTION_VECTORS,
        NUM_BUFFERS
    };

    // String look-up table to give names to buffers in ImGui!
    // If the above enum changes, you need to change this!
    extern std::string m_BufferNames[eGBuffers::NUM_BUFFERS];

    // The actual array of buffer handles
    extern std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NUM_BUFFERS> m_GBuffers;



    //
	//
	//	Functions to break up smaller sections of the UI
	//
    //


    //
    //  Main Sections that users will see
    //

    // This section also acts as the tutorial
    void StartupModal();

	void MainWindowTitle();

    void ResolutionDisplay();

    // Main render mode selection area
    void RenderModeSelection();

    //
    //   Debug sections for developers!
    //

    void DLSSSettings();
    void ResolutionSettingsDebug();

    // Send context through to this function in order to display GBuffers!
    void GraphicsSettings(CommandContext& Context);
    void GraphicsSettingsDebug(CommandContext& Context);

    void PerformanceMetrics();


	//
	// Constants to help with this specific GUI
	//

    // How big is the main GUI window? Normally I'd make this a constant but the resolution will constantly be changing in this app!
    extern ImVec2 m_MainWindowSize;
    // Starting position of main window
    extern ImVec2 m_MainWindowPos;


    // Size of child windows
    //

    extern ImVec2 m_BufferWindowSize;
    extern ImVec2 m_MetricWindowSize;

    //
    // Starting position of child windows
    
    extern ImVec2 m_BufferWindowPos;       // Set it at far corner
    extern ImVec2 m_HwTimingWindowPos;     // Set it directly next to the main window at the top
    extern ImVec2 m_FrameRateWindowPos;    // Set underneath the other one

};

// Pallete consts for ease of use, readabilty and editability
namespace ThemeColours
{
    // const ImVec4 m_HighlightColour = ImVec4(1.0f, 1.f, 0.4f, 1.f);
    const ImVec4 m_PureBlack = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    const ImVec4 m_PureWhite = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    const ImVec4 m_RtuaGold = ImVec4(1.f, 0.8f, 0.f, 1.f);

    const ImVec4 m_RtuaBlack = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    const ImVec4 m_RtuaLightBlack = ImVec4(0.04f, 0.04f, 0.04f, 0.85f);
    const ImVec4 m_PurgatoryGrey = ImVec4(0.25f, 0.25f, 0.25f, 0.5f);
    const ImVec4 m_Charcoal = ImVec4(0.15f, 0.15f, 0.21f, 0.965f);
    const ImVec4 m_GunmetalGrey = ImVec4(0.068f, 0.068f, 0.068f, 0.965f);
    const ImVec4 m_DarkerGold = ImVec4(0.9f, 0.7f, 0.f, 0.9f);
    const ImVec4 m_DarkestGold = ImVec4(0.8f, 0.6f, 0.f, 0.8f);;
    const ImVec4 m_BasicallyRed = ImVec4(0.9f, 0.05f, 0.f, 0.9f);

    const ImVec4 m_HighlightColour = m_RtuaGold;
};

// Helper namespace (separated for readability) that helps with styling any GUI!
namespace GUI_Style
{
   //
   // Custom Style Setter
   // 

    void SetStyle();


    //
    // Style Helper Methods
    //

    // Common item spacings
    const void QuarterLineBreak();
    const void HalfLineBreak();  
    const void SingleLineBreak();
    const void DoubleLineBreak();

    const void SingleTabSpace();
    const void DoubleTabSpace();
    const void TripleTabSpace();

    // Section Separator
    const void Separator();

    //
    // Text item alignment (works for buttons too! basically, anything that uses text!)
    //
    const void CenterNextTextItem(const char* text);
    const void RightAlignNextTextItem(const char* text);

    // Used for aligning a button onto the right side of the window
    const void RightAlignSameLine(const char* text);

    // Common function to make an ImGui item that contains text fit that text
    const void MakeNextItemFitText(const char* text);

    // Custom function for combos - they have an additional arrow to account for
    const void CenterNextCombo(const char* text);

    // Reusable section title function
    const void SectionTitle(const char* titleText);

    // Reusable wrapped bullet text! Self-Indents by 1 level, call xxTabSpace() for more levels!
    const void WrappedBullet(const char* bulletText);

    // Reusable function to highlight items!
    const void HighlightTextItem(const char* itemText, bool center = true, bool wrapped = false, float spacing = 7.5f, float thickness = 3.f);

    // Function to create helpful little tooltips
    const void HelpMarker(const char* desc);

    // Reusable page button code - using a ref as the page number will belong to the specific GUI (RTUA_GUI) that implements this function as part of its tutorial
    const void TutorialPageButtons(uint8_t& pageNum);

    // 
    // Style Helper Members
    //
   
    // Common window pivot positions
    const ImVec2 kTopLeftPivot = { 0.f, 0.f };
    const ImVec2 kTopRightPivot = { 1.f, 0.f };
    const ImVec2 kCenterPivot = { 0.5f, 0.5f };
    const ImVec2 kBottomLeftPivot = { 0.f, 1.f };
};