//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "AZB_GUI.h"
#include "AZB_DLSS.h"

#include <Utility.h>

// For performance metrics
#include "EngineProfiling.h"
#include "Display.h"			// To allow ImGui to trigger swapchain resize and handle global resolution controls!
#include "BufferManager.h"		// For debug rendering buffers
#include "CommandContext.h"		// For transitioning resources
#include <TemporalEffects.h>	// For jitter
#include "Renderer.h"			
#include "SamplerManager.h"		// For mipBias overriding and control!
#include <ModelLoader.h>

//===============================================================================
// Wrapped in macro so that the source still compiles when macro is disabled
#if AZB_MOD

//============================================================================
//
// 
// GUI Style Helper Implementation
// 
// 
//============================================================================

#pragma region GUI Style Helper Implementation

const void GUI_Style::QuarterLineBreak() { ImGui::Dummy(ImVec2(0.0f, 5.0f)); }
const void GUI_Style::HalfLineBreak() { ImGui::Dummy(ImVec2(0.0f, 10.0f)); }
const void GUI_Style::SingleLineBreak() { ImGui::Dummy(ImVec2(0.0f, 20.0f)); }
const void GUI_Style::DoubleLineBreak() { ImGui::Dummy(ImVec2(0.0f, 40.0f)); }

const void GUI_Style::SingleTabSpace() { ImGui::SameLine(0.0f, 20.0f); }
const void GUI_Style::DoubleTabSpace() { ImGui::SameLine(0.0f, 40.0f); }
const void GUI_Style::TripleTabSpace() { ImGui::SameLine(0.0f, 60.0f); }

const void GUI_Style::Separator()
{
	// Add space between previous item
	DoubleLineBreak();
	// Draw separator
	ImGui::Separator();
	// Add space for next item
	DoubleLineBreak();
}

const void GUI_Style::CenterNextTextItem(const char* text) { ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2.f); }
const void GUI_Style::RightAlignNextTextItem(const char* text) { ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x)); }
const void GUI_Style::RightAlignSameLine(const char* text) { ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(text).x + ImGui::GetTextLineHeightWithSpacing())); }
const void GUI_Style::MakeNextItemFitText(const char* text) { ImGui::PushItemWidth(ImGui::CalcTextSize(text).x + ImGui::GetTextLineHeightWithSpacing()); }
const void GUI_Style::CenterNextCombo(const char* text) { ImGui::SetCursorPosX(((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2.f) - ImGui::GetFrameHeight()); }

const void GUI_Style::SectionTitle(const char* titleText)
{
	CenterNextTextItem(titleText);
	ImGui::Text(titleText);
}

const void GUI_Style::WrappedBullet(const char* bulletText)
{
	ImGui::Bullet();
	SingleTabSpace();
	ImGui::TextWrapped(bulletText);
}

const void GUI_Style::HighlightTextItem(const char* itemText, bool center, bool wrapped, float spacing, float thickness)
{
	if (center)
		CenterNextTextItem(itemText);
	if (!wrapped)
		ImGui::Text(itemText);
	else
		ImGui::TextWrapped(itemText);
	// Calculate position to draw rect of last item
	ImVec2 firstItemPosMin = ImGui::GetItemRectMin();
	ImVec2 firstItemPosMax = ImGui::GetItemRectMax();

	// Add an offset to create some space around the item we are highlighting
	ImVec2 firstRectPosMin = ImVec2(firstItemPosMin.x - spacing, firstItemPosMin.y - spacing);
	ImVec2 firstRectPosMax = ImVec2(firstItemPosMax.x + spacing, firstItemPosMax.y + spacing);

	// Submit the highlight rectangle to ImGui's draw list!
	ImGui::GetWindowDrawList()->AddRect(firstRectPosMin, firstRectPosMax, ImColor(ThemeColours::m_HighlightColour), 0, ImDrawFlags_None, thickness);
}

const void GUI_Style::HelpMarker(const char* desc)
{
	ImGui::TextColored(ThemeColours::m_RtuaGold, "(?)");
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

const void GUI_Style::TutorialPageButtons(uint8_t& pageNum)
{
	const char* btnText = "Previous";
	MakeNextItemFitText(btnText);
	ImGui::SetItemDefaultFocus();

	// Return to previous page
	if (ImGui::Button(btnText))
	{
		--pageNum;
	}

	btnText = "Next";
	RightAlignSameLine(btnText);
	MakeNextItemFitText(btnText);


	if (ImGui::Button(btnText))
	{
		// Advance to next page
		++pageNum;
	}

}

//
// Custom Style Setter
// 

void GUI_Style::SetStyle()
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

#pragma endregion

//============================================================================
//
// 
// Main GUI Implementation
// 
// 
//============================================================================

#pragma region Main Gui Implementation

// Using namespace here to keep code easily readable
using namespace GUI_Style;

//
// Define namespace members here - matching MiniEngine's design
//
namespace RTUA_GUI
{
	//
	// See header file for comments on each member here
	//

	ID3D12DescriptorHeap* m_pSrvDescriptorHeap = nullptr;
	ID3D12Device* m_pD3DDevice = nullptr;
	const Model* m_pScene = nullptr;

	uint32_t m_NewWidth = 0u;
	uint32_t m_NewHeight = 0u;
	Resolution m_TitleBarSize{};


	bool m_bReady = false;
	bool m_bEnablePostFX = true;
	bool m_bEnableMotionVisualisation = true;

	bool m_bShowStartupModal = true;
	bool m_bFullscreen = false;
	bool m_bDisplayModeChangePending = false;


	// This needs to change if the enum changes
	std::string m_RenderModeNames[eRenderingMode::NUM_RENDER_MODES] =
	{
		"Native"            ,
		"Bilinear Upscale"  ,
		"DLSS "
	};
	eRenderingMode m_CurrentRenderingMode = eRenderingMode::NATIVE;
	eRenderingMode m_PreviousRenderingMode = eRenderingMode::NATIVE;

	float m_ScalingFactor = 0.f;
	Resolution m_BilinearInputRes = { 640, 480 };
	bool m_bResolutionChangePending = false;
	bool m_bCommonStateChangePending = false;
	bool m_bOverrideLodBias = false;
	float m_ForcedLodBias = 0.f;
	float m_OriginalLodBias = 0.f;

	bool m_bDLSSUpdatePending = false;
	bool m_bToggleDLSS = false;
	bool m_bUpdateDLSSMode = false;

	bool m_ShowHardwareMetrics = false;
	bool m_ShowFrameRate = false;


	ImVec2 m_MainWindowSize{};
	ImVec2 m_MainWindowPos{};

	ImVec2 m_BufferWindowSize{};
	ImVec2 m_MetricWindowSize{};

	ImVec2 m_BufferWindowPos{};       // Set at far corner
	ImVec2 m_HwTimingWindowPos{};     // Set directly next to the main window at the top
	ImVec2 m_FrameRateWindowPos{};    // Set underneath the other one

	uint8_t m_Page = 1;

	// If the enum changes, you need to change this!
	std::string m_BufferNames[eGBuffers::NUM_BUFFERS] =
	{
		"Main Color"            ,
		"Depth"                 ,
		"Motion Vectors Raw"    ,
		"MV Visualisation"
	};

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NUM_BUFFERS> m_GBuffers{};

}

#pragma region Main Gui Loop

void RTUA_GUI::Init(void* Hwnd, ID3D12Device* pDevice, int numFramesInFlight, const DXGI_FORMAT& renderTargetFormat)
{
	// Create ImGui context and get IO to set flags
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// Setup ImPlot here too
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// Enable keyboard & gamepad controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
	
	// Enable docking and multiple viewports!
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Disable ini file so that window settings don't save, I want the app to start in the same configuration every time!
	io.IniFilename = NULL;
	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(Hwnd);

	// MiniEngine uses dynamic heap allocation, sharing this with ImGui will probably cause more headaches than is needed, so create a dedicated heap for ImGui
	// This will simplify resource management and allow for better scalability as the UI continues to grow in complexity and polish

	// Begin integrating into mini engine

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescriptor = {};
	descriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDescriptor.NumDescriptors = eGBuffers::NUM_BUFFERS + 1;						// Increase and reserve spot for more textures! The first spot is used for built-in font hence the + 1
	descriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ASSERT_SUCCEEDED(pDevice->CreateDescriptorHeap(&descriptorHeapDescriptor, IID_PPV_ARGS(&m_pSrvDescriptorHeap)));

	// Set ImGui up with his heap
	ImGui_ImplDX12_Init(pDevice, numFramesInFlight, renderTargetFormat, m_pSrvDescriptorHeap,
		m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// Store device for heap allocations!
	m_pD3DDevice = pDevice;
	
	// Set app to use our custom style!
	SetStyle();

	// Set newWidth at the start so that it doesn't start at 0 and avoid DLSS crashing!
	m_NewWidth = Graphics::g_NativeWidth;
	m_NewHeight = Graphics::g_NativeHeight;

	// Also set up our window vars!
	m_MainWindowSize = { m_NewWidth * 0.25f, m_NewHeight * 0.5f };
	m_MainWindowPos = { (m_NewWidth - m_MainWindowSize.x), 0.f };

	// Store handles to GBuffers so that we can display them!
	m_GBuffers[eGBuffers::SCENE_COLOR] = Graphics::g_SceneColorBuffer.GetSRV();
	m_GBuffers[eGBuffers::SCENE_DEPTH] = Graphics::g_LinearDepth[TemporalEffects::GetFrameIndex()].GetSRV();
	m_GBuffers[eGBuffers::MOTION_VECTORS] = Graphics::g_DecodedVelocityBuffer.GetSRV();
	m_GBuffers[eGBuffers::VISUAL_MOTION_VECTORS] = Graphics::g_MotionVectorVisualisationBuffer.GetSRV();


	// Set swapchain to start in fullscreen!
		
	// wb for Windows Bool - have to use this when querying the swapchain!
	BOOL wbFullscreen = FALSE;
	Display::GetSwapchain()->GetFullscreenState(&wbFullscreen, nullptr);

	m_bFullscreen = wbFullscreen;
	HRESULT hr = Display::GetSwapchain()->SetFullscreenState(!wbFullscreen, nullptr);
	if (SUCCEEDED(hr))
	{
		Display::GetSwapchain()->GetFullscreenState(&wbFullscreen, nullptr);
		m_bFullscreen = wbFullscreen;
		DEBUGPRINT("Switched to %s mode", m_bFullscreen ? "Fullscreen" : "Windowed");
	}

	// Set init flag to true so rest of windows app can behave appropriately!
	m_bReady = true;

}

void RTUA_GUI::Run(CommandContext& Context)
{
	// Start ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

		// Display Modal if it is the first time!
	if (m_bShowStartupModal)
		StartupModal();
	else
		// Display the our lovely main Window!
	{

		ImGui::SetNextWindowPos(m_MainWindowPos, ImGuiCond_Appearing, kTopLeftPivot);
		ImGui::SetNextWindowSize(m_MainWindowSize, ImGuiCond_Appearing);
		// Allow window to resize, but only vertically. Restrict horizontal size to a fixed width
		ImGui::SetNextWindowSizeConstraints(ImVec2(m_MainWindowSize.x, 0), ImVec2(m_MainWindowSize.x, FLT_MAX));

		if (!ImGui::Begin("RTUA", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			// Early out if the window is collapsed, as an optimization.
			ImGui::End();
			return;
		}

		// Begin with the lovely formatted title section
		MainWindowTitle();

		// Display Render Resolution information!
		ResolutionDisplay();

		// Display main rendermode area next!
		RenderModeSelection();
		// This one will sit in the same area as the rendermode selection
		GraphicsSettings(Context);

		PerformanceMetrics();

		// Debug sections with more variables to tweak!
#if AZB_DBG
		ResolutionSettingsDebug();
		DLSSSettings();
#endif
		
		ImGui::End();
	}
}

void RTUA_GUI::UpdateGraphics()
{
	// Check if the common graphics state (samplers etc.) needs updating first
	if (m_bCommonStateChangePending)
	{
		if (m_bOverrideLodBias)
		{
			if (DLSS::m_bDLSS_Enabled)
			{
				Renderer::UpdateSamplers(m_pScene, DLSS::m_CurrentInternalResolution, true, m_ForcedLodBias);
			}
			else
			{
				Renderer::UpdateSamplers(m_pScene, DLSS::m_CurrentNativeResolution, true, m_ForcedLodBias);
			}
		}
		else
		{
			if (DLSS::m_bDLSS_Enabled)
			{
				Renderer::UpdateSamplers(m_pScene, DLSS::m_CurrentInternalResolution);
			}
			else
			{
				// This will fire off when turning the override off.
				// Add an extra step when in native rendering mode to reset back to 0, instead of -1
				Renderer::UpdateSamplers(m_pScene, DLSS::m_CurrentNativeResolution, true, 0.0f);
			}
		}

		// Reset flag
		m_bCommonStateChangePending = false;
	}

	// Check if display mode has changed - this will happen when alt+tabbing or alt+enter
	if (m_bDisplayModeChangePending)
	{
		// wb for Windows Bool - have to use this when querying the swapchain!
		BOOL wbFullscreen = FALSE;
		Display::GetSwapchain()->GetFullscreenState(&wbFullscreen, nullptr);

		// This will actually attempt to toggle fullscreen state
		//HRESULT hr = Display::GetSwapchain()->SetFullscreenState(!wbFullscreen, nullptr);

		// This version sets it explicitly based on controlling the bool externally
		HRESULT hr = Display::GetSwapchain()->SetFullscreenState(m_bFullscreen, nullptr);
		if (SUCCEEDED(hr))
		{
			DEBUGPRINT("Switched to %s mode", m_bFullscreen ? "Fullscreen" : "Windowed");

			// Update new width and height to fullscreen values if we have entered fullscreen
			if (m_bFullscreen)
			{
				// Set to max by default
				m_NewWidth = DLSS::m_MaxNativeResolution.m_Width;
				m_NewHeight = DLSS::m_MaxNativeResolution.m_Height;
			}
			else
			{
				// When returning to windowed, set it to default smaller window size of 720p!
				m_NewWidth = 1280u;
				m_NewHeight = 720u;
			}

			// Set pipeline update flag
			m_bResolutionChangePending = true;
			// Also move GUI window to the new top corner!
			m_MainWindowPos.x = ((float)m_NewWidth - m_MainWindowSize.x) - 5.f;
			ImGui::SetWindowPos("RTUA", m_MainWindowPos);

			// If we are in a different rendering mode, reset those values
			if (m_CurrentRenderingMode == eRenderingMode::BILINEAR_UPSCALE)
				// Set bilinear back to the new native, effectively disabling scaling as the factor returns to 1
				m_BilinearInputRes = { m_NewWidth, m_NewHeight };

			if (m_CurrentRenderingMode == eRenderingMode::DLSS)
			{
				// For DLSS, just disable it and return to native
				m_CurrentRenderingMode = eRenderingMode::NATIVE;
				DLSS::m_bNeedsReleasing = true;
				m_bToggleDLSS = false;
				m_bDLSSUpdatePending = true;
			}
		}
		else
		{
			DEBUGPRINT("\nFailed to toggle fullscreen mode.\n");
		}

		// Reset flag
		m_bDisplayModeChangePending = false;
	}

	// See if resolution has been changed for any given display mode
	if (m_bResolutionChangePending)
	{
		// Regardless of specific change, DLSS will need to be recreated, and it cannot handle being on while a resize occurs!
		// Before it can be recreated, disable the flag to ensure it doesn't try to run while out of date!
		if (DLSS::m_bDLSS_Enabled)
		{
			m_bToggleDLSS = false;
			m_bDLSSUpdatePending = true;
		}

		if (!m_bFullscreen)
		{
			// This version scales the window to match the resolution - it also returns the size of the windows titlebar so that ImGui can generate a more user-friendly resolution for the user!
			m_TitleBarSize = Display::SetWindowedResolution(m_NewWidth, m_NewHeight);

			// Update window size and reset position as it may not fit into the new window size
			m_MainWindowSize.y = (float)m_NewHeight * 0.5f;
			m_MainWindowPos.x = ((float)m_NewWidth - m_MainWindowSize.x) - 10.f;
			ImGui::SetWindowSize("RTUA", m_MainWindowSize);
			ImGui::SetWindowPos("RTUA", m_MainWindowPos);
		}
		else
		{
			// This version maintains fullscreen size based on what was queried at app startup and stretches the resolution to the display size
			// We still need to update the pipeline to render at the lower resolution
			Display::SetPipelineResolution(false, m_NewWidth, m_NewHeight);

			// Clean up swapchain when this happens!
		}

		// Reset flag for next time
		m_bResolutionChangePending = false;
		// Also let DLSS know that the resolution has changed!
		DLSS::m_bNeedsReleasing = true;
	}

	// Check if anything needs to change with DLSS
	if (m_bDLSSUpdatePending)
	{
		// If we're fullscreen, we need to reset back to full res
		if (m_bFullscreen)
		{
			// If we're in bilinear, reset to that value instead for comparison
			if (m_CurrentRenderingMode == eRenderingMode::BILINEAR_UPSCALE)
			{
				m_NewWidth = m_BilinearInputRes.m_Width;
				m_NewHeight = m_BilinearInputRes.m_Height;
			}
			else
			{
				m_NewWidth = Graphics::g_DisplayWidth;
				m_NewHeight = Graphics::g_DisplayHeight;
			}
		}

		DLSS::UpdateDLSS(m_bToggleDLSS, m_bUpdateDLSSMode, { m_NewWidth, m_NewHeight });
		// DLSS may need the pipeline to update as a result of the changes made. Check the internal flag and then update as appropriate
		if (DLSS::m_bPipelineUpdate)
		{
			Display::SetPipelineResolution(true, DLSS::m_DLSS_Modes[DLSS::m_CurrentQualityMode].m_RenderWidth, DLSS::m_DLSS_Modes[DLSS::m_CurrentQualityMode].m_RenderHeight);
			// Reset flag
			DLSS::m_bPipelineUpdate = false;
		}

		// This is set to true when disabling DLSS
		if (DLSS::m_bPipelineReset)
		{
			Display::SetPipelineResolution(false, m_NewWidth, m_NewHeight);
			DLSS::m_bPipelineReset = false;
		}
		m_bDLSSUpdatePending = false;
		m_bUpdateDLSSMode = false;

		// Enable this new flag to update mips next frame!
		m_bCommonStateChangePending = true;
	}
}

void RTUA_GUI::Terminate()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	ImPlot::DestroyContext();

	// Last step - ensure the swapchain is not in fullscreen as otherwise the rest of the pipeline can't terminate correctly
	HRESULT hr = Display::GetSwapchain()->SetFullscreenState(FALSE, nullptr);
	if (SUCCEEDED(hr))
	{
		DEBUGPRINT("Switched to Windowed mode at shutdown!");
	}
}

#pragma endregion

//
//	Smaller functions that break down individual sections of the GUI. Makes it much more readable and modular!
//

#pragma region GUI Section Helpers

void RTUA_GUI::StartupModal()
{
	ImGui::OpenPopup("Welcome!");
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowSize(m_MainWindowSize, ImGuiCond_Appearing);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, kCenterPivot);

	// Allow window to resize, but only vertically. Restrict horizontal size to a fixed width
	ImGui::SetNextWindowSizeConstraints(ImVec2(m_MainWindowSize.x, 0), ImVec2(m_MainWindowSize.x, FLT_MAX));

	ImGui::BeginPopupModal("Welcome!", NULL, ImGuiChildFlags_AlwaysAutoResize);
	{
		DoubleLineBreak();

		// Render content in "pages". ImGui doesn't support this natively but we can create this effect pretty easily!
		switch (m_Page)
		{
			// First page
			case 1:
			{


				SectionTitle("Welcome to Real-Time Upscaling Analyser!");
				Separator();

				ImGui::TextWrapped("This is a tool developed as part of a study into upscaling techniques within the field of real-time rendering.");
				SingleLineBreak();
				ImGui::TextWrapped("Important information or places of interest within the GUI will be highlighted like so:");
				DoubleLineBreak();

				HighlightTextItem("Use the Arrow Keys to navigate the GUI!");
				SingleLineBreak();
				HighlightTextItem("Press Enter or Spacebar to interact with elements!");

				DoubleLineBreak();
				ImGui::TextWrapped("If this is your first time, please start the tutorial to see other controls and learn how the GUI functions.");
				SingleLineBreak();
				ImGui::TextWrapped("If you already know what you're doing, feel free to skip the tutorial and get started.");
				
				DoubleLineBreak();
	
				// Temp var to store the text that will go inside a button
				const char* btnText = "Start Tutorial";
				// This makes it so the next item we define (in this case a button) will have the right size
				MakeNextItemFitText(btnText);
				// Center the button within the modal
				CenterNextTextItem(btnText);

				ImGui::SetItemDefaultFocus();

				// Advance to next page
				if (ImGui::Button(btnText))
				{
					++m_Page;
				}

				SingleLineBreak();

				btnText = "Skip";
				MakeNextItemFitText(btnText);
				CenterNextTextItem(btnText);
				if (ImGui::Button(btnText))
				{
					m_bShowStartupModal = false;
					ImGui::CloseCurrentPopup();
				}

				SingleLineBreak();

				break;
			}

			case 2:
			{
				SectionTitle("Scene Controls");
				Separator();

				ImGui::TextWrapped("You have a few different ways to move throughout the scene.");
				SingleLineBreak();
				HighlightTextItem("WASD to move, Mouse for camera");
				SingleLineBreak();
				ImGui::TextWrapped("You can also change how fast you move with WASD so you can slow down for evaluation.");
				SingleLineBreak();
				HighlightTextItem("LSHIFT to change movement speed");
				DoubleLineBreak();

				TutorialPageButtons(m_Page);

				break;
			}
			
			case 3:
			{

				SectionTitle("GUI Controls");
				Separator();

				ImGui::TextWrapped("Occasionally you will need the mouse to interact with certain elements like tooltips or graphs.");
				SingleLineBreak();
				ImGui::TextWrapped("You can also move any GUI windows around wherever you like by dragging with the mouse.");
				SingleLineBreak();
				HighlightTextItem("LCTRL+M to toggle mouse input between the GUI and the scene!");
				SingleLineBreak();
				ImGui::TextWrapped("Use the command highlighted above to toggle between inputs when necessary.");
				SingleLineBreak();
				ImGui::TextWrapped("You can also navigate between windows and move them without having to switch input mode. Note; the following commands will work on the main window, but not in this tutorial section");
				SingleLineBreak();
				HighlightTextItem("Hold LCTRL and press TAB once to bring up GUI window select.");
				SingleLineBreak();
				ImGui::TextWrapped("Make sure you keep LCTRL held to keep this window open. From here, you can:");
				SingleLineBreak();
				HighlightTextItem("Press TAB again to cycle between open windows.");
				SingleLineBreak();
				HighlightTextItem("Use Arrow Keys while to move selected window position.");
				DoubleLineBreak();

				TutorialPageButtons(m_Page);

				break;
			}

			// Final page
			case 4:
			{
				SectionTitle("Evaluation Guidance");
				Separator();

				ImGui::TextWrapped("Native rendering should be straightforward to evaluate, however you may find yourself struggling to compare the two upscale methods. The biggest factor in your evaluation should be input resolution.");
				SingleLineBreak();
				HighlightTextItem("To best compare upscaling, use the exact same input resolution.");
				SingleLineBreak();
				ImGui::TextWrapped("When swapping between Bilinear or DLSS upscaling, the input resolution you select will be saved. However, not all DLSS input resolutions exist as inputs for Bilinear. As a result, a scaling factor has been provided to help your evaluation.");
				SingleLineBreak();
				HighlightTextItem("The scaling factor is your next most important comparison point.");
				SingleLineBreak();
				ImGui::TextWrapped("The scaling factor is a simple value that represents the percentage of native rendering we are currently rendering at. 1.0 means that no scaling is taking place and, if you prefer, that we are rendering at 100 percent native. 0.666 means that we are rendering at two-thirds or 66 percent of native resolution and then scaling up.");
				SingleLineBreak();
				ImGui::TextWrapped("Additionally, LOD or Mip bias has a great effect on texture resolution when upscaling. DLSS automatically calculates the optimal bias, but you are free to override this and see the effects in real-time.");
				SingleLineBreak();
				ImGui::TextWrapped("Lastly, upscaling can have varying effects depending on the type of surface you are looking at, the distance and angle from which you view it and much more.");
				SingleLineBreak();
				HighlightTextItem("Try and test against many surfaces in as many ways as possible.");
				DoubleLineBreak();

				TutorialPageButtons(m_Page);

				break;
			}

			case 5:
			{

				SectionTitle("Final Words");
				Separator();

				ImGui::TextWrapped("If you need to read these instructions again, you can find a button to re-open this popup at any time.");
				HalfLineBreak();
				ImGui::TextWrapped("There are also helpful tooltips across the main application, please interact with these!");
				SingleLineBreak();
				HighlightTextItem("Helper tooltips will look like this: (?) ");
				SingleLineBreak();
				ImGui::TextWrapped("And most importantly, let your curiosity drive you. You may come away from this experience with an increased sensitivity and appreciation for rendering quality.");
				DoubleLineBreak();


				// Customised version of the TutorialPageButtons() function for the final page. 

				const char* btnText = "Previous";
				MakeNextItemFitText(btnText);
				ImGui::SetItemDefaultFocus();
				if (ImGui::Button(btnText))
				{
					--m_Page;
				}


				btnText = "Begin Analysing!";
				RightAlignSameLine(btnText);
				MakeNextItemFitText(btnText);

				// Finally allow the user to close the popup
				if (ImGui::Button(btnText))
				{
					m_bShowStartupModal = false;
					ImGui::CloseCurrentPopup();
				}

				break;

			}
		}
 
		ImGui::EndPopup();
	}
}

void RTUA_GUI::MainWindowTitle()
{
	DoubleLineBreak();

	ImGui::TextWrapped("From this main window you can:");
	SingleLineBreak();

	// Use my wrapper to do a bunch of bullet points with text wrapping! Also self indents by 1 level!
	WrappedBullet("Swap between render modes.");
	SingleLineBreak();
	WrappedBullet("Tweak settings relating to the implementation of the each rendering technique");
	SingleLineBreak();
	WrappedBullet("View performance metrics for the current rendering mode");


	Separator();

	//
	// DEBUG
	//

#if AZB_DBG
	
	// Try and draw some shapes
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	static float size = 36.0f;
	static float thickness = 3.0f;
	static ImVec4 colf = ImVec4(1.0f, 0.4f, 0.4f, 1.f);

	ImGui::DragFloat("Size", &size, 0.2f, 2.0f, 100.0f, "%.0f");
	ImGui::DragFloat("Thickness", &thickness, 0.05f, 1.0f, 8.0f, "%.02f");
	ImGui::ColorEdit4("Color", &colf.x);

	const ImVec2 p = ImGui::GetCursorScreenPos();
	float x = p.x + 4.0f;
	float y = p.y + 4.0f;
	const ImU32 col = ImColor(colf);
	const float spacing = 10.0f;
	const ImDrawFlags corners_tl_br = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight;
	const float rounding = size / 5.0f;

	draw_list->AddRect(ImVec2(x, y), ImVec2(x + size, y + size), col, rounding, corners_tl_br, thickness);         x += size + spacing;  // Square with two rounded corners
	draw_list->AddTriangle(ImVec2(x + size * 0.5f, y), ImVec2(x + size, y + size - 0.5f), ImVec2(x, y + size - 0.5f), col, thickness); x += size + spacing;  // Triangle
	//y += size + spacing;

	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + size, y + size), col, 10.0f, corners_tl_br);              x += size + spacing;  // Square with two rounded corners
	draw_list->AddTriangleFilled(ImVec2(x + size * 0.5f, y), ImVec2(x + size, y + size - 0.5f), ImVec2(x, y + size - 0.5f), col);  x += size + spacing;  // Triangle

	DoubleLineBreak();
	DoubleLineBreak();
	Separator();

	static float highlightOffset = 0.f;
	ImGui::DragFloat("Highlight Spacing", &highlightOffset, 0.05f, -1.f, 10.0f, "%.02f");

	static bool bEnableHighlighting;
	ImGui::Checkbox("Toggle Highlighting", &bEnableHighlighting);

	DoubleLineBreak();
	
	ImGui::Text("Text to try highlighting edges!");

	// Calculate position to draw rect of last item
	ImVec2 firstItemPosMin = ImGui::GetItemRectMin();
	ImVec2 firstItemPosMax = ImGui::GetItemRectMax();

	ImVec2 firstRectPosMin = ImVec2(firstItemPosMin.x - highlightOffset, firstItemPosMin.y - highlightOffset);
	ImVec2 firstRectPosMax = ImVec2(firstItemPosMax.x + highlightOffset, firstItemPosMax.y + highlightOffset);


	if (bEnableHighlighting)
		draw_list->AddRect(firstRectPosMin, firstRectPosMax, col,0, ImDrawFlags_None, thickness);     // Regular square highlight 
	DoubleLineBreak();

	// Split channels so that text doesn't get covered by rect - rect gets covered by text!
	draw_list->ChannelsSplit(2);
	// Set next item (in this case text) to be drawn onto first channel. This represents the top layer / foreground
	draw_list->ChannelsSetCurrent(1);
	ImGui::Text("Text to try highlighting fully!");

	// Recalculate rect pos
	ImVec2 secondItemPosMin = ImGui::GetItemRectMin();
	ImVec2 secondItemPosMax = ImGui::GetItemRectMax();
	ImVec2 secondRectPosMin = ImVec2(secondItemPosMin.x - highlightOffset, secondItemPosMin.y - highlightOffset);
	ImVec2 secondRectPosMax = ImVec2(secondItemPosMax.x + highlightOffset, secondItemPosMax.y + highlightOffset);

	// Set next item (in this case the highlight rect) to be drawn onto 0th channel. This represents the lowest layer / background
	draw_list->ChannelsSetCurrent(0);
	if (bEnableHighlighting)
		draw_list->AddRectFilled(secondRectPosMin, secondRectPosMax, col);     // Filled square highlight 

	// Merge layers!
	DoubleLineBreak();
	draw_list->ChannelsMerge();

	ImGui::Button("Button to try highlighting!");
	//
	// END DEBUG
	//

	DoubleLineBreak();
	Separator();
#endif
}

void RTUA_GUI::ResolutionDisplay()
{
	// In order to make it clearer to users, create labels
	const char* labelText;
	std::string labelValue;


	labelText = "Display Resolution";
	CenterNextTextItem(labelText);
	ImGui::Text(labelText);

	ImGui::SameLine();
	HelpMarker("This is the current display size of the app. This won't change in fullscreen as the window won't change size.");

	labelValue = std::to_string(DLSS::m_MaxNativeResolution.m_Width) + "x" + std::to_string(DLSS::m_MaxNativeResolution.m_Height);
	labelText = labelValue.c_str();
	CenterNextTextItem(labelText);
	ImGui::Text(labelText);

	SingleLineBreak();

	labelText = "Native Resolution";
	CenterNextTextItem(labelText);
	ImGui::Text(labelText);
	ImGui::SameLine();
	HelpMarker("This is the current resolution of internal rendering buffers in the app. This will update as you interact with upscaling techniques.\n\nTry finding where DLSS modes (e.g. Quality) and Bilinear inputs (e.g. 1280x800) match for the best comparison points!");
	labelValue = std::to_string(Graphics::g_NativeWidth) + "x" + std::to_string(Graphics::g_NativeHeight);
	labelText = labelValue.c_str();
	CenterNextTextItem(labelText);
	ImGui::Text(labelText);

	SingleLineBreak();

	labelText = "Current Scale Factor";
	CenterNextTextItem(labelText);
	ImGui::Text(labelText);
	ImGui::SameLine();
	HelpMarker("This is the magnitude of scaling being applied. When this is 100%, that means that no scaling is taking place.\n0.666 means that we are rendering at a scale of 66% native\n\nTry finding where DLSS modes and Bilinear inputs match for the best comparison points!");
	m_ScalingFactor = static_cast<float>(Graphics::g_NativeHeight) / static_cast<float>(DLSS::m_MaxNativeResolution.m_Height);	// Calculated by using render height / display height
	labelValue = std::to_string(m_ScalingFactor);
	labelText = labelValue.c_str();
	CenterNextTextItem(labelText);
	ImGui::Text(labelText);

	SingleLineBreak();

	// Create a button to relaunch tutorial model
	labelText = "Help";
	CenterNextTextItem(labelText);

	if (ImGui::Button(labelText))
	{
		m_bShowStartupModal = true;
		// Reset page count so it starts at the beginning
		m_Page = 1;
	}

	ImGui::SameLine();
	HelpMarker("This will restart the tutorial so you can refresh your knowledge of inputs.");

	Separator();

	m_NewWidth = Graphics::g_NativeWidth;
	m_NewHeight = Graphics::g_NativeHeight;
}

void RTUA_GUI::RenderModeSelection()
{
	static int mode_current_idx = m_CurrentRenderingMode;

	const char* comboLabel = "Rendering Mode";
	const char* comboPreviewValue = m_RenderModeNames[m_CurrentRenderingMode].c_str();


	// Write custom centered label
	CenterNextTextItem(comboLabel);
	ImGui::TextColored(ThemeColours::m_RtuaGold, comboLabel);

	// Create combo to select rendering mode - Center it as it's the main event!
	CenterNextCombo(comboPreviewValue);

	// Using cool method to remove label from combo!
	if (ImGui::BeginCombo("##RenderCombo", comboPreviewValue, ImGuiComboFlags_WidthFitPreview))
	{
		for (int n = 0; n < eRenderingMode::NUM_RENDER_MODES ; n++)
		{
			const bool is_selected = (mode_current_idx == n);
			if (ImGui::Selectable(m_RenderModeNames[n].c_str(), is_selected))
			{
				mode_current_idx = n;
				// Update rendering modes
				m_PreviousRenderingMode = m_CurrentRenderingMode;
				m_CurrentRenderingMode = (eRenderingMode)n;


				// Now execute appropriate behaviour based on rendering mode, but only if it's changed!
				if (m_CurrentRenderingMode != m_PreviousRenderingMode)
				{
					// Regardless of where we've come from or are going to - reset the LOD override if it's enabled
					m_bOverrideLodBias = false;
					switch (m_CurrentRenderingMode)
					{
						case eRenderingMode::NATIVE:
						{
							// Check if we were using DLSS previously - we need to reset the pipeline and toggle it off
							if (m_PreviousRenderingMode == eRenderingMode::DLSS)
							{
								// Set flags appropriately
								m_bToggleDLSS = false;
								m_bDLSSUpdatePending = true;
							}

							// Check if the resolution is at native, if it's been changed from another mode, reset it!
							if (Graphics::g_NativeWidth != DLSS::m_MaxNativeResolution.m_Width && Graphics::g_NativeHeight != DLSS::m_MaxNativeResolution.m_Height)
							{
								m_NewWidth = DLSS::m_MaxNativeResolution.m_Width;
								m_NewHeight = DLSS::m_MaxNativeResolution.m_Height;
								// Set flag to true so that we can update the pipeline next frame! This will result in DLSS needing to be recreated also
								// This takes place in UpdateGraphics()
								m_bResolutionChangePending = true;
								

							}
							break;
						}

						case eRenderingMode::BILINEAR_UPSCALE:
						{
							// Reset DLSS just like we did with Native
							if (m_PreviousRenderingMode == eRenderingMode::DLSS)
							{
								m_bToggleDLSS = false;
								m_bDLSSUpdatePending = true;
							}

							// Check if the internal resolution is the same as last time, if it's been changed from another mode, reset it!
							if (Graphics::g_NativeWidth != m_BilinearInputRes.m_Width && Graphics::g_NativeHeight != m_BilinearInputRes.m_Height)
							{
								m_NewWidth = m_BilinearInputRes.m_Width;
								m_NewHeight = m_BilinearInputRes.m_Height;
								// Set flag to true so that we can update the pipeline next frame! This will result in DLSS needing to be recreated also
								// This takes place in UpdateGraphics()
								m_bResolutionChangePending = true;
							}
							break;
						}

						case eRenderingMode::DLSS:
						{
							// Only do the next section if DLSS is supported!
							if (DLSS::m_bIsNGXSupported)
							{
								// Removed checkbox and added an automatic toggle when this mode is selected!
								m_bToggleDLSS = true;
								m_bDLSSUpdatePending = true;

							}
							break;
						}
					}
				}
			}

			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();

	}

	// After the state changes are done, check what mode we are in and add those extra options in, if they're needed!
	switch (m_CurrentRenderingMode)
	{
		case eRenderingMode::NATIVE:
		{
			break;
		}
		case eRenderingMode::BILINEAR_UPSCALE:
		{

			// Choose internal resolution for bilinear upscaling
			std::string comboValue = std::to_string(m_BilinearInputRes.m_Width) + "x" + std::to_string(m_BilinearInputRes.m_Height);
			const char* res_combo_preview_value = comboValue.c_str();
			//static int res_current_idx = DLSS::m_NumResolutions - 1;
			static int res_current_idx = 0;


			// Also center this child combo
			comboLabel = "Internal Resolution";
			CenterNextTextItem(comboLabel);
			ImGui::TextColored(ThemeColours::m_DarkerGold, comboLabel);
			
			// Display a helpful tooltip
			ImGui::SameLine();
			HelpMarker("This is an input native resolution that will then get upscaled to the final display resolution\n\nWhen this resolution matches the display resolution (displayed above), no scaling will take place.");

			CenterNextCombo(res_combo_preview_value);
			if (ImGui::BeginCombo("##Internal Resolution Combo", res_combo_preview_value, ImGuiComboFlags_WidthFitPreview))
			{
				for (uint16_t n = 0; n < DLSS::m_NumResolutions; n++)
				{
					const bool is_selected = (res_current_idx == n);
					if (ImGui::Selectable(DLSS::m_Resolutions[n].first.c_str(), is_selected))
					{
						res_current_idx = n;
						m_bResolutionChangePending = true;
						// Also update what the values should be
						m_NewWidth = DLSS::m_Resolutions[n].second.m_Width;
						m_NewHeight = DLSS::m_Resolutions[n].second.m_Height;
						m_BilinearInputRes = { m_NewWidth, m_NewHeight };
					}

					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			break;
		}

		case eRenderingMode::DLSS:
		{
			// Only show the next section if DLSS is supported!
			if (DLSS::m_bIsNGXSupported)
			{
				static int dlssMode = 1; // 0: Performance, 1: Balanced, 2: Quality, etc.
				const char* modes[] = { "Performance", "Balanced", "Quality", "Ultra Performance" };

				// Also center this child combo
				comboLabel = "Super Resolution Mode";
				CenterNextTextItem(comboLabel);
				ImGui::TextColored(ThemeColours::m_DarkerGold, comboLabel);

				ImGui::SameLine();
				HelpMarker("These modes are the official names given by NVIDIA but they simply represent an input resolution to upscale from, just like Bilinear Upscaling");

				CenterNextCombo(modes[dlssMode]);
				if (ImGui::BeginCombo("##DLSS Mode", modes[dlssMode], ImGuiComboFlags_WidthFitPreview))
				{
					for (uint8_t n = 0; n < std::size(modes); n++)
					{
						const bool is_selected = (dlssMode == n);
						if (ImGui::Selectable(modes[n], is_selected))
						{
							dlssMode = n;
							// Update current mode
							DLSS::m_CurrentQualityMode = n;
							// Set flags
							DLSS::m_bNeedsReleasing = true;
							m_bDLSSUpdatePending = true;
							m_bUpdateDLSSMode = true;
							// Also reset LOD Bias override
							m_bOverrideLodBias = false;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				// Let user know that DLSS isn't supported
				const char* centerText = "DLSS is not supported by your hardware! Sorry!";
				CenterNextTextItem(centerText);
				ImGui::TextColored({ 1.f,0.f,0.f,1.f }, centerText);
			}
			break;
		}
	}
}

void RTUA_GUI::GraphicsSettings(CommandContext& Context)
{
	QuarterLineBreak();

	ImGui::TextColored(ThemeColours::m_RtuaGold, "Graphics Settings");
	ImGui::Text("Default LODBias : %.2f", m_OriginalLodBias);
	ImGui::SameLine();
	HelpMarker("This value affects the resolution at which textures are sampled. DLSS will automatically adjust this value when it is active. ");
	QuarterLineBreak();

	if (ImGui::Checkbox("Override LODBias", &m_bOverrideLodBias))
		m_bCommonStateChangePending = true;

	if (m_bOverrideLodBias)
	{
		ImGui::SameLine(); if (ImGui::DragFloat("New LOD Bias", &m_ForcedLodBias, 0.01f, -3.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput))
			m_bCommonStateChangePending = true;
	}
	else
	{
		ImGui::SameLine();
		HelpMarker("Enable this to feed in a custom bias and see the effects it has for yourself.");
		m_OriginalLodBias = DLSS::m_LodBias;
	}

	ImGui::Checkbox("Enable PostFX", &m_bEnablePostFX);
	SingleLineBreak();

#if AZB_DBG
	static bool showBuffers = false;
	ImGui::Checkbox("Show GBuffers", &showBuffers);

	if (showBuffers)
	{
		GraphicsContext& imguiGBufferContext = Context.GetGraphicsContext();

		imguiGBufferContext.TransitionResource(Graphics::g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		imguiGBufferContext.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		imguiGBufferContext.TransitionResource(Graphics::g_DecodedVelocityBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		imguiGBufferContext.TransitionResource(Graphics::g_MotionVectorVisualisationBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		//
		// Update ImGui descriptor heap by adding GBuffer texture
		//

		// Open in new window
		m_BufferWindowSize = { 1024 , 840.f };
		m_BufferWindowPos = { 0.f, (float)DLSS::m_MaxNativeResolution.m_Height };
		
		ImGui::SetNextWindowSizeConstraints(ImVec2(m_BufferWindowSize.x, m_BufferWindowSize.y), ImVec2(m_BufferWindowSize.x * 4, FLT_MAX));

		ImGui::SetNextWindowSize(m_BufferWindowSize, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(m_BufferWindowPos, ImGuiCond_Appearing, kBottomLeftPivot);  // Set it at top corner

		ImGui::Begin("GBuffers");

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("GBuffers", tab_bar_flags))
		{

			for (UINT i = 0; i < eGBuffers::NUM_BUFFERS; ++i)
			{
				// Create a tab for each buffer
				if (ImGui::BeginTabItem(m_BufferNames[i].c_str()))
				{
					// Get handle to the start of this heap
					D3D12_CPU_DESCRIPTOR_HANDLE newCPUHandle = m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

					// Calculate the size of each descriptor
					UINT descriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

					// Font is loaded in at slot 0, so we want to load our textures starting from slot 1
					int descriptorIndex = 1 + i;

					// Use this offset for each descriptor
					newCPUHandle.ptr += descriptorIndex * descriptorSize;

					// Copy our existing SRV into the new descriptor heap!
					m_pD3DDevice->CopyDescriptorsSimple(1, newCPUHandle, m_GBuffers[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

					// ImGui recommends explicitly using the GPU handle to our texture, but MiniEngine doesn't support returning that
					// However, in copying the Descriptor to the correct slot on the CPU side, the GPU should now also be at that same index, and we simply need to update
					//		the ptr from the start of the new heap!

					// Repeat for GPU handle
					D3D12_GPU_DESCRIPTOR_HANDLE newGPUHandle = m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
					newGPUHandle.ptr += (descriptorIndex * descriptorSize);
					
					// Render our lovely buffer!
					SingleLineBreak();
					ImGui::Image((ImTextureID)newGPUHandle.ptr, ImVec2(960, 720));
					ImGui::EndTabItem();
				}
			}

			ImGui::EndTabBar();
			ImGui::End();
		}
	}
#endif
}

void RTUA_GUI::PerformanceMetrics()
{
	// Show checkboxes that open windows!
	ImGui::TextColored(ThemeColours::m_RtuaGold, "Peformance Metrics");
	ImGui::Checkbox("Hardware Frame Times", &m_ShowHardwareMetrics);
	ImGui::Checkbox("Frame Rate (FPS)", &m_ShowFrameRate);


	// Open windows when bools are true! 
	if (m_ShowHardwareMetrics)
	{
		// Re-calculate window sizes for the current resolution!
		m_MetricWindowSize = { m_MainWindowSize.x, m_MainWindowSize.y * 0.666f };
		m_HwTimingWindowPos = { m_MainWindowPos.x - m_MainWindowSize.x, m_MainWindowPos.y }; // Set it directly next to the main window at the top

		ImGui::SetNextWindowSize(m_MetricWindowSize, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(m_HwTimingWindowPos, ImGuiCond_Appearing);

		ImGui::Begin("Hardware Metrics");

		// Continue with rest of window

		// Frame data from MiniEngine profiler!
		static std::vector<float> cpuTimes, gpuTimes;

		cpuTimes.push_back(EngineProfiling::GetCPUTime());		// CPU time per frame
		gpuTimes.push_back(EngineProfiling::GetGPUTime());		// GPU time per frame

		// Limit buffer sizes
		if (cpuTimes.size() > 1000) cpuTimes.erase(cpuTimes.begin());
		if (gpuTimes.size() > 1000) gpuTimes.erase(gpuTimes.begin());

		// Plot the data
		if (ImPlot::BeginPlot("Hardware Timings (MS)"))
		{
			// Setup axis, x then y. This will be Frame,Ms. Use autofit for now, will mess around with these later
			ImPlot::SetupAxes("Frame", "Speed(ms)", ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit);
			ImPlot::PlotLine("CPU Time", cpuTimes.data(), (int)cpuTimes.size());
			ImPlot::PlotLine("GPU Time", gpuTimes.data(), (int)gpuTimes.size());
			ImPlot::EndPlot();
		}

		ImGui::End();
	}

	// Repeat for FPS!
	if (m_ShowFrameRate)
	{
		m_MetricWindowSize = { m_MainWindowSize.x, m_MainWindowSize.y * 0.666f };
		m_FrameRateWindowPos = { m_MainWindowPos.x - m_MainWindowSize.x, m_MainWindowPos.y + m_MetricWindowSize.y };	// Next to main window start pos, and just below where the hardware timings would have gone

		ImGui::SetNextWindowSize(m_MetricWindowSize, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(m_FrameRateWindowPos, ImGuiCond_Appearing);

		ImGui::Begin("Frame Rate");

		static std::vector<float> frameTimes;

		frameTimes.push_back(EngineProfiling::GetFrameRate());
		if (frameTimes.size() > 1000) frameTimes.erase(frameTimes.begin());

		if (ImPlot::BeginPlot("Frame Rate"))
		{
			ImPlot::SetupAxes("Count", "FPS", ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit);
			ImPlot::PlotLine("Frame Rate", frameTimes.data(), (int)frameTimes.size());
			ImPlot::EndPlot();
		}

		ImGui::End();
	}
}

void RTUA_GUI::ResolutionSettingsDebug()
{
	std::string comboValue;
	// In order to make it clearer to users and devs, create a variable combo label
	std::string comboLabel;

	if (ImGui::CollapsingHeader("Resolution Settings"))
	{
		static int item_current_idx = DLSS::m_NumResolutions - 1;

		// Check window display mode. 
		// When Fullscreen, our display height will be fixed (naturally), so we want to convey native height to the user
		// When Windowed, display height has variable, and can even be resized manually.
		// This block takes that into account and ensures that the dropdown is always as helpful as possible
		if (m_bFullscreen)
		{
			comboLabel = "Native Resolution";
			comboValue = std::to_string(Graphics::g_NativeWidth) + "x" + std::to_string(Graphics::g_NativeHeight);
			// Also update these - the display could have changed as a result of window resizing!
			m_NewWidth = Graphics::g_NativeWidth;
			m_NewHeight = Graphics::g_NativeHeight;

		}
		else
		{
			comboLabel = "Display Resolution";
			comboValue = std::to_string(m_NewWidth) + "x" + std::to_string(m_NewHeight);
			// Also update these - the display could have changed as a result of window resizing! Also account for windows title bars! These will vary based on aspect ratio and scaling
			m_NewWidth = Graphics::g_DisplayWidth + m_TitleBarSize.m_Width;
			m_NewHeight = Graphics::g_DisplayHeight + m_TitleBarSize.m_Height;	// Display height also includes Windows Title Bars, so account for this in our GUI!

		}

		const char* combo_preview_value = comboValue.c_str();

		if (ImGui::BeginCombo(comboLabel.c_str(), combo_preview_value))
		{
			for (uint16_t n = 0; n < DLSS::m_NumResolutions; n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(DLSS::m_Resolutions[n].first.c_str(), is_selected))
				{
					item_current_idx = n;
					// Set flag to true so that we can update the pipeline next frame! This will result in DLSS needing to be recreated also
					// This takes place in UpdateGraphics()
					m_bResolutionChangePending = true;
					// Also update what the values should be
					m_NewWidth = DLSS::m_Resolutions[n].second.m_Width;
					m_NewHeight = DLSS::m_Resolutions[n].second.m_Height;
				}

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}



		// wb for Windows Bool - have to use this when querying the swapchain!
		BOOL wbFullscreen = FALSE;
		Display::GetSwapchain()->GetFullscreenState(&wbFullscreen, nullptr);

		m_bFullscreen = wbFullscreen;

		// Add a checkbox to control fullscreen
		if (ImGui::Checkbox("Enable fullscreen mode", &m_bFullscreen))
		{
			// Flip flag to signal a display mode change at the start of next frame.
			m_bDisplayModeChangePending = true;
		}
	}
}

void RTUA_GUI::DLSSSettings()
{
	if (ImGui::CollapsingHeader("DLSS Settings"))
	{
		// Only show the next section if DLSS is supported!
		if (DLSS::m_bIsNGXSupported)
		{
			static int dlssMode = 1; // 0: Performance, 1: Balanced, 2: Quality, etc.
			const char* modes[] = { "Performance", "Balanced", "Quality", "Ultra Performance" };




			// Main selection for user to play with!
			if (ImGui::Checkbox("Enable DLSS", &m_bToggleDLSS))
			{
				m_bDLSSUpdatePending = true;
			}

			// Wrap mode selection in disabled blcok - only want to edit this when DLSS is ON
			if (!m_bToggleDLSS)
				ImGui::BeginDisabled(true);
			if (ImGui::BeginCombo("Mode", modes[dlssMode]))
			{

				for (uint8_t n = 0; n < std::size(modes); n++)
				{
					const bool is_selected = (dlssMode == n);
					if (ImGui::Selectable(modes[n], is_selected))
					{
						dlssMode = n;
						// Update current mode
						DLSS::m_CurrentQualityMode = n;
						// Set flags
						DLSS::m_bNeedsReleasing = true;
						m_bDLSSUpdatePending = true;
						m_bUpdateDLSSMode = true;
					}

					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();

			}

			if (!m_bToggleDLSS)
				ImGui::EndDisabled();

		}
		else
		{
			// Let user know that DLSS isn't supported
			const char* centerText = "DLSS is not supported by your hardware! Sorry!";
			CenterNextTextItem(centerText);
			ImGui::TextColored({ 1.f,0.f,0.f,1.f }, centerText);
		}

	}

}

void RTUA_GUI::GraphicsSettingsDebug(CommandContext& Context)
{
	if (ImGui::CollapsingHeader("Graphics Settings"))
	{
		if (ImGui::Checkbox("Override LODBias", &m_bOverrideLodBias))
			m_bCommonStateChangePending = true;
		
		if (m_bOverrideLodBias)
		{
			if(ImGui::DragFloat("LODBias (-3.0 ~ 1.0)", &m_ForcedLodBias, 0.01f, -3.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput))
				m_bCommonStateChangePending = true;
		}
		else
		{
			ImGui::Text("Default LODBias : %.2f", Graphics::m_DefaultLodBias);
		}

		ImGui::Checkbox("Enable PostFX", &m_bEnablePostFX);


		static bool showMV = false;
		ImGui::Checkbox("Show GBuffers", &showMV);

		if (showMV)
		{
			GraphicsContext& imguiGBufferContext = Context.GetGraphicsContext();

			imguiGBufferContext.TransitionResource(Graphics::g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			imguiGBufferContext.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			imguiGBufferContext.TransitionResource(Graphics::g_DecodedVelocityBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			imguiGBufferContext.TransitionResource(Graphics::g_MotionVectorVisualisationBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			//
			// Update ImGui descriptor heap by adding GBuffer texture
			//

			// Open in new window
			ImGui::Begin("GBuffer Rendering Test", 0, ImGuiWindowFlags_AlwaysAutoResize);

			for (UINT i = 0; i < eGBuffers::NUM_BUFFERS; ++i)
			{
				// Get handle to the start of this heap
				D3D12_CPU_DESCRIPTOR_HANDLE newCPUHandle = m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

				// Calculate the size of each descriptor
				UINT descriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				// Font is loaded in at slot 0, so we want to load our textures starting from slot 1
				int descriptorIndex = 1 + i;

				// Use this offfset for each descriptor
				newCPUHandle.ptr += descriptorIndex * descriptorSize;

				// Copy our existing SRV into the new descriptor heap!
				m_pD3DDevice->CopyDescriptorsSimple(1, newCPUHandle, m_GBuffers[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				// ImGui recommends explicitly using the GPU handle to our texture, but MiniEngine doesn't support returning that
				// However, in copying the Descriptor to the correct slot on the CPU side, the GPU should now also be at that same index, and we simply need to update
				//		the ptr from the start of the new heap!

				// Repeat for GPU handle
				D3D12_GPU_DESCRIPTOR_HANDLE newGPUHandle = m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
				newGPUHandle.ptr += (descriptorIndex * descriptorSize);

				ImGui::Text("CPU handle = %p", newCPUHandle.ptr);
				ImGui::Text("GPU handle = %p", newGPUHandle.ptr);
				DoubleLineBreak();
				ImGui::Text("Buffer: %s", m_BufferNames[i].c_str());
				// Render our lovely buffer!
				ImGui::Image((ImTextureID)newGPUHandle.ptr, ImVec2(400, 300));

				DoubleLineBreak();
			}
			ImGui::End();
		}
	}

}

#pragma endregion

#pragma endregion

#endif
