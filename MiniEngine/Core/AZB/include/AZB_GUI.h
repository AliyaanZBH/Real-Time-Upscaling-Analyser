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

class GUI
{
public:
	GUI() {}

	// Get ImGui started!
	void Init(void* Hwnd, ID3D12Device* pDevice, int numFramesInFlight, const DXGI_FORMAT& renderTargetFormat);

	// Run ImGui render loop!
	void Run(CommandContext& ImGuiContext);

    // Our GUI can control resolution and other pipeline state at runtime, however we don't want to be messing about with this while frames are in flight!
    // This function will be called at the start of the update loop of the next frame, submitting any changes we made on the previous one.
    void UpdateGraphics();

	// Shutdown ImGui safely
	void Terminate();
    

	// A handle to render ImGui within MiniEngine
	ID3D12DescriptorHeap* m_pSrvDescriptorHeap = nullptr;

    // Public pointer to the scene we will be investigating!
    const Model* m_pScene = nullptr;

    // Public flag to enable/disable tonemapping!
    bool m_bEnablePostFX = true; 
    
    // Public flag to enable/disable Motion Vector visualisation!
    bool m_bEnableMotionVisualisation = true;

    // Flag to show startup modal
    bool m_bShowStartupModal = true;

    // Flag to track display mode and adjust behaviour accordingly
    bool m_bFullscreen = false;
    // Flag to indicate that display mode should change next frame
    bool m_bDisplayModeChangePending = false;

private:


    //
    //  MiniEngine Integration
    //  
    
    // Handle to global device - needed to update descriptor heap!
    ID3D12Device* m_pD3DDevice = nullptr;


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
    // IF the above enum changes, you need to change this!
    std::string m_BufferNames[eGBuffers::NUM_BUFFERS] =
    {
        "Main Color"            ,
        "Depth"                 ,
        "Motion Vectors Raw"    ,
        "MV Visualisation"       
    };

    // The actual array of buffer handles
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NUM_BUFFERS> m_GBuffers{};


    //
    // Member variables that allow for safe manipulation of graphics pipeline.
    //

    // Values that represent the new resolution to change to next frame (if the user requests a change!)
    uint32_t m_NewWidth = 0u;
    uint32_t m_NewHeight = 0u;

    // The size of the the title bar in windowed mode - this was originally the total window resolution but this led to memory leaks when resizing!
    Resolution m_TitleBarSize{};




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

    std::string m_RenderModeNames[eRenderingMode::NUM_RENDER_MODES] =
    {
        "Native"            ,
        "Bilinear Upscale"  ,
        "DLSS "
    };

    // Tracker for rendering mode
    eRenderingMode m_CurrentRenderingMode = eRenderingMode::NATIVE;
    // Track last mode so that we can reset any state
    eRenderingMode m_PreviousRenderingMode = eRenderingMode::NATIVE;

    // Tracker for bilinear upscale input, so it can be saved and returned to when swapping modes
    Resolution m_BilinearInputRes = { 640, 480 };

    // Flag to indicate that the resolution should change next frame
    bool m_bResolutionChangePending = false;
    
    // Flag to indicate that common graphics state needs re-initalising next frame e.g. due to a change in mip bias for texture sampling
    bool m_bCommonStateChangePending = false;
    // Flag that tracks the checkbox which controls LOD bias
    bool m_bOverrideLodBias = false;
    // Actual value that can be controlled by the user to determine a custom LOD bias
    float m_ForcedLodBias = 0.f;
    // Original LOD bias before overriding
    float m_OriginalLodBias = 0.f;


    // Flag to indicate when a change has been made to DLSS
    bool m_bDLSSUpdatePending = false;
    // Flag to control when to enable/disable DLSS
    bool m_bToggleDLSS = false;
    // Flag to indicate that the requested change is to DLSS mode
    bool m_bUpdateDLSSMode = false;


    // Checkbox that controls whether to show metrics window
    bool m_ShowHardwareMetrics = false;
    bool m_ShowFrameRate = false;





	//
	//	Functions to break up smaller sections of the UI
	//


    //
    //  Main Sections that users will see
    //

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
	// Constants to help with the UI
	//


    // How big is the window? Normally I'd make this a constant but the resolution will constantly be changing in this app!
    ImVec2 m_MainWindowSize;

    //
    // Size of child windows
    //

    ImVec2 m_BufferWindowSize;
    ImVec2 m_MetricWindowSize;

    // Starting position of main window
    ImVec2 m_MainWindowPos;
    
    //
    // Starting position of child windows
    
    ImVec2 m_BufferWindowPos;       // Set it at far corner
    ImVec2 m_HwTimingWindowPos;     // Set it directly next to the main window at the top
    ImVec2 m_FrameRateWindowPos;    // Set underneath the other one


	// Common window pivot positions
	const ImVec2 kTopLeftPivot = { 0.f, 0.f };
	const ImVec2 kTopRightPivot = { 1.f, 0.f };
	const ImVec2 kCenterPivot = { 0.5f, 0.5f };
	const ImVec2 kBottomLeftPivot = { 0.f, 1.f };

	// Common item spacings
	const void QuarterLineBreak() { ImGui::Dummy(ImVec2(0.0f, 5.0f)); }
	const void HalfLineBreak() { ImGui::Dummy(ImVec2(0.0f, 10.0f)); }
	const void SingleLineBreak() { ImGui::Dummy(ImVec2(0.0f, 20.0f)); }
	const void DoubleLineBreak() { ImGui::Dummy(ImVec2(0.0f, 40.0f)); }

	const void SingleTabSpace() { ImGui::SameLine(0.0f, 20.0f); }
	const void DoubleTabSpace() { ImGui::SameLine(0.0f, 40.0f); }
	const void TripleTabSpace() { ImGui::SameLine(0.0f, 60.0f); }

	// Section separator
	const void Separator() {
		// Add space between previous item
		DoubleLineBreak();
		// Draw separator
		ImGui::Separator();
		// Add space for next item
		DoubleLineBreak();
	}

    //
	// Text item alignment (works for buttons too! basically, anything that uses text!)
    //
	const void CenterNextTextItem(const char* text) { ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2.f); }
	const void RightAlignNextTextItem(const char* text) { ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x)); }

    // Used for aligning a button onto the right side of the window
    const void RightAlignSameLine(const char* text) { ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(text).x + ImGui::GetTextLineHeightWithSpacing())); }

	// Common function to make an ImGui item that contains text fit that text
	const void MakeNextItemFitText(const char* text) { ImGui::PushItemWidth(ImGui::CalcTextSize(text).x + ImGui::GetTextLineHeightWithSpacing()); }

    // Custom function for combos - they have an additional arrow to account for
	const void CenterNextCombo(const char* text) { ImGui::SetCursorPosX(((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2.f) - ImGui::GetFrameHeight()) ; }

	// Reusable section title function
	const void SectionTitle(const char* titleText)
	{
		CenterNextTextItem(titleText);
		ImGui::Text(titleText);
	}

    // Reusable wrapped bullet text! Self-Indents by 1 level, call xxTabSpace() for more levels!
    const void WrappedBullet(const char* bulletText)
    {
        ImGui::Bullet();
        SingleTabSpace();
        ImGui::TextWrapped(bulletText);
    }

    // Reusable function to highlight items!
    const void HighlightTextItem(const char* itemText, bool center = true, float spacing = 7.5f, float thickness = 3.f)
    {
        if (center)
            CenterNextTextItem(itemText);
        ImGui::Text(itemText);

        // Calculate position to draw rect of last item
        ImVec2 firstItemPosMin = ImGui::GetItemRectMin();
        ImVec2 firstItemPosMax = ImGui::GetItemRectMax();

        // Add an offset to create some space around the item we are highlighting
        ImVec2 firstRectPosMin = ImVec2(firstItemPosMin.x - spacing, firstItemPosMin.y - spacing);
        ImVec2 firstRectPosMax = ImVec2(firstItemPosMax.x + spacing, firstItemPosMax.y + spacing);

        // Submit the highlight rectangle to ImGui's draw list!
        ImGui::GetWindowDrawList()->AddRect(firstRectPosMin, firstRectPosMax, ImColor(ThemeColours::m_HighlightColour), 0, ImDrawFlags_None, thickness);
    }

    // Slightly more advanced function that can highlight variable items! 
    const void HighlightItem();

    // Function to create helpful little tooltips
    const void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        // Added this to give it a custom color
        ImGui::PushStyleColor(0, ThemeColours::m_RtuaGold);
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopStyleColor();
    }


	//============================================================================
	//
	// Custom Style
	// 
	//============================================================================


	void SetStyle()
	{
        ImVec4* colors = ImGui::GetStyle().Colors;

        colors[ImGuiCol_Text] = ThemeColours::m_PureWhite;
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ThemeColours::m_Charcoal;
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ThemeColours::m_Charcoal;
        colors[ImGuiCol_Border] = ThemeColours::m_PureBlack;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ThemeColours::m_GunmetalGrey;
        colors[ImGuiCol_FrameBgHovered] = ThemeColours::m_DarkestGold;
        colors[ImGuiCol_FrameBgActive] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_TitleBg] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_TitleBgActive] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_TitleBgCollapsed] = ThemeColours::m_DarkestGold;
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.25f, 0.25f, 0.5f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 0.171f, 0.f, 0.9f);
        colors[ImGuiCol_Button] = ThemeColours::m_RtuaBlack;
        colors[ImGuiCol_ButtonHovered] = ThemeColours::m_DarkestGold;
        colors[ImGuiCol_ButtonActive] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_Header] = ThemeColours::m_RtuaLightBlack;
        colors[ImGuiCol_HeaderHovered] = ThemeColours::m_DarkestGold;
        colors[ImGuiCol_HeaderActive] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_Separator] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_SeparatorHovered] = ImVec4(1.f, 0.171f, 0.f, 1.f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(1.f, 0.1f, 0.f, 1.f);
        colors[ImGuiCol_ResizeGrip] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_ResizeGripHovered] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_ResizeGripActive] = ThemeColours::m_DarkestGold;
        colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
        colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
        colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
        colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.f, 0.5f, 1.f, 1.f);                   // Gamepad / KBM highlight.
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	};
};