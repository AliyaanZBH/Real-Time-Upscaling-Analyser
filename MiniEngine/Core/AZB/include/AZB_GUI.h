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
// Pallete consts for ease of use, readabilty and editability
namespace ThemeColours
{
    const ImVec4 m_HighlightColour = ImVec4(1.0f, 0.4f, 0.4f, 1.f);
    const ImVec4 m_PureBlack = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    const ImVec4 m_PureWhite = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

    const ImVec4 m_RtuaBlack = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    const ImVec4 m_RtuaLightBlack = ImVec4(0.04f, 0.04f, 0.04f, 0.85f);
    const ImVec4 m_PurgatoryGrey = ImVec4(0.25f, 0.25f, 0.25f, 0.5f);
    const ImVec4 m_Charcoal = ImVec4(0.15f, 0.15f, 0.21f, 0.965f);
    const ImVec4 m_GunmetalGrey = ImVec4(0.068f, 0.068f, 0.068f, 0.965f);
    const ImVec4 m_RtuaGold = ImVec4(1.f, 0.8f, 0.f, 1.f);
    const ImVec4 m_DarkerGold = ImVec4(1.f, 0.72f, 0.f, 1.f);
    const ImVec4 m_DarkestGold = ImVec4(1.f, 0.67f, 0.f, 1.f);
    const ImVec4 m_BasicallyRed = ImVec4(0.9f, 0.05f, 0.f, 0.9f);
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


    // Public flag to enable/disable tonemapping!
    bool m_bEnablePostFX = true; 
    
    // Public flag to enable/disable Motion Vector visualisation!
    bool m_bEnableMotionVisualisation = true;

    // Flag to show startup modal
    bool m_bShowStartupModal = true;

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

    // Flag to indicate that the resolution should change next frame
    bool m_bResolutionChangePending = false;
    // Flag to control fullscreen/windowed behaviour
    bool m_bFullscreen = false;

    // Flag to indicate when a change has been made to DLSS
    bool m_bDLSSUpdatePending = false;
    // Flag to control when to enable/disable DLSS
    bool m_bToggleDLSS = false;
    // Flag to indicate that the requested change is to DLSS mode
    bool m_bUpdateDLSSMode = false;



	//
	//	Functions to break up smaller sections of the UI
	//


    void StartupModal();

	void MainWindowTitle();

    void ResolutionSettings();

    void DLSSSettings();

    // Send context through to this function in order to display GBuffers!
    void GraphicsSettings(CommandContext& Context);

    void PerformanceMetrics();


	//
	// Constants to help with the UI
	//


    // How big is the window? Normally I'd make this a constant but the resolution will constantly be changing in this app!
    ImVec2 m_MainWindowSize;

    //
    // Size of child windows
    //

    ImVec2 m_MetricWindowSize;

    // Starting position of main window
    ImVec2 m_MainWindowPos;
    
    //
    // Starting position of child windows
    
    ImVec2 m_HwTimingWindowPos; // Set it directly next to the main window at the top
    ImVec2 m_FrameRateWindowPos;


	// Common window pivot positions
	const ImVec2 kTopLeftPivot = { 0.f, 0.f };
	const ImVec2 kTopRightPivot = { 1.f, 0.f };
	const ImVec2 kCenterPivot = { 0.5f, 0.5f };

	// Common item spacings
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

	// Text alignment
	const void CenterNextTextItem(const char* text) { ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2.f); }

	// Common function to make an ImGui item that contains text fit that text
	const void MakeNextItemFitText(const char* text) { ImGui::PushItemWidth(ImGui::CalcTextSize(text).x + ImGui::GetTextLineHeightWithSpacing()); }


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






	//============================================================================
	//
	// Custom Style
	// 
	//============================================================================


	void SetStyle()
	{
        ImVec4* colors = ImGui::GetStyle().Colors;

        // Pallete consts for ease of use, readabilty and editability
       //const ImVec4 pureBlack(0.04f, 0.04f, 0.04f, 1.00f);
       //const ImVec4 pureWhite(1.00f, 1.00f, 1.00f, 1.00f);
       //
       //const ImVec4 rtuaBlack(0.04f, 0.04f, 0.04f, 1.00f);
       //const ImVec4 rtuaLightBlack(0.04f, 0.04f, 0.04f, 0.85f);
       //const ImVec4 purgatoryGrey(0.25f, 0.25f, 0.25f, 0.5f);
       //const ImVec4 charcoal(0.15f, 0.15f, 0.21f, 0.965f);
       //const ImVec4 gunmetalGrey(0.068f, 0.068f, 0.068f, 0.965f);
       //const ImVec4 rtuaGold(1.f, 0.8f, 0.f, 1.f);
       //const ImVec4 darkerGold(1.f, 0.72f, 0.f, 1.f);
       //const ImVec4 darkestGold(1.f, 0.67f, 0.f, 1.f);
       //const ImVec4 basicallyRed(0.9f, 0.05f, 0.f, 0.9f);

        colors[ImGuiCol_Text] = ThemeColours::m_PureWhite;
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ThemeColours::m_Charcoal;
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ThemeColours::m_Charcoal;
        colors[ImGuiCol_Border] = ThemeColours::m_PureBlack;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ThemeColours::m_GunmetalGrey;
        colors[ImGuiCol_FrameBgHovered] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_FrameBgActive] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_TitleBg] = ThemeColours::m_RtuaGold;
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
        colors[ImGuiCol_ButtonActive] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_Header] = ThemeColours::m_RtuaLightBlack;
        colors[ImGuiCol_HeaderHovered] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_HeaderActive] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_Separator] = ThemeColours::m_RtuaGold;
        colors[ImGuiCol_SeparatorHovered] = ImVec4(1.f, 0.171f, 0.f, 1.f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(1.f, 0.1f, 0.f, 1.f);
        colors[ImGuiCol_ResizeGrip] = ThemeColours::m_DarkerGold;
        colors[ImGuiCol_ResizeGripHovered] = ThemeColours::m_DarkestGold;
        colors[ImGuiCol_ResizeGripActive] = ThemeColours::m_RtuaGold;
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
        colors[ImGuiCol_NavHighlight] = ImVec4(0.98f, 0.26f, 0.36f, 1.00f); //old ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	};
};