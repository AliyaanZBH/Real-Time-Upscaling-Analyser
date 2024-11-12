#pragma once
//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "imgui.h"
#include "imgui_internal.h" // For ImLerp
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "implot.h"

#include "AZB_Utils.h"

//===============================================================================

class GUI
{
public:
	GUI() {}

	// Get ImGui started!
	void Init(void* Hwnd, ID3D12Device* pDevice, int numFramesInFlight, const DXGI_FORMAT& renderTargetFormat);

	// Run ImGui render loop!
	void Run();

    // Our GUI can control resolution and other pipeline state at runtime, however we don't want to be messing about with this while frames are in flight!
    // This function will be called at the start of the update loop of the next frame, submitting any changes we made on the previous one.
    void UpdateGraphics();

	// Shutdown ImGui safely
	void Terminate();

	// A handle to render ImGui within MiniEngine
	ID3D12DescriptorHeap* m_pSrvDescriptorHeap = nullptr;

private:

    //
    // Member variables that allow for safe manipulation of graphics pipeline.
    //

    // Values that represent the new resolution to change to next frame (if the user requests a change!)
    uint32_t m_NewWidth = 0u;
    uint32_t m_NewHeight = 0u;

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

	void MainWindowTitle();

	//
	// Constants to help with the UI
	//

	// Situate window on the left side, just underneath the existing profiling
	const ImVec2 kMainWindowStartPos = { 0.f, 75.f };

	// How big is the window at program start?
	const ImVec2 kMainWindowStartSize = { 680.f, 850.f };

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







	//============================================================================
	//
	// Custom Style
	// 
	//============================================================================

	void SetStyle()
	{
        ImVec4* colors = ImGui::GetStyle().Colors;

        // Pallete consts for ease of use, readabilty and editability
        const ImVec4 pureBlack(0.04f, 0.04f, 0.04f, 1.00f);
        const ImVec4 pureWhite(1.00f, 1.00f, 1.00f, 1.00f);

        const ImVec4 rtuaBlack(0.04f, 0.04f, 0.04f, 1.00f);
        const ImVec4 rtuaLightBlack(0.04f, 0.04f, 0.04f, 0.85f);
        const ImVec4 purgatoryGrey(0.25f, 0.25f, 0.25f, 0.5f);
        const ImVec4 gunmetalGrey(0.068f, 0.068f, 0.068f, 0.965f);
        const ImVec4 rtuaGold(1.f, 0.8f, 0.f, 1.f);
        const ImVec4 darkerGold(1.f, 0.72f, 0.f, 1.f);
        const ImVec4 darkestGold(1.f, 0.67f, 0.f, 1.f);
        const ImVec4 basicallyRed(0.9f, 0.05f, 0.f, 0.9f);

        colors[ImGuiCol_Text] = pureWhite;
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = gunmetalGrey;
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = gunmetalGrey;
        colors[ImGuiCol_Border] = pureBlack;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = rtuaLightBlack;
        colors[ImGuiCol_FrameBgHovered] = rtuaGold;
        colors[ImGuiCol_FrameBgActive] = darkerGold;
        colors[ImGuiCol_TitleBg] = rtuaGold;
        colors[ImGuiCol_TitleBgActive] = darkerGold;
        colors[ImGuiCol_TitleBgCollapsed] = darkestGold;
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.25f, 0.25f, 0.5f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 0.171f, 0.f, 0.9f);
        colors[ImGuiCol_Button] = rtuaBlack;
        colors[ImGuiCol_ButtonHovered] = darkerGold;
        colors[ImGuiCol_ButtonActive] = darkestGold;
        colors[ImGuiCol_Header] = rtuaLightBlack;
        colors[ImGuiCol_HeaderHovered] = rtuaGold;
        colors[ImGuiCol_HeaderActive] = darkerGold;
        colors[ImGuiCol_Separator] = darkerGold;
        colors[ImGuiCol_SeparatorHovered] = ImVec4(1.f, 0.171f, 0.f, 1.f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(1.f, 0.1f, 0.f, 1.f);
        colors[ImGuiCol_ResizeGrip] = darkerGold;
        colors[ImGuiCol_ResizeGripHovered] = darkestGold;
        colors[ImGuiCol_ResizeGripActive] = basicallyRed;
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