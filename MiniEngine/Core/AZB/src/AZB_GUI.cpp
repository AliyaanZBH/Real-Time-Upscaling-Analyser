//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "AZB_GUI.h"
#include "AZB_DLSS.h"

#include <d3d12.h>
#include <Utility.h>
#include <array>

// For performance metrics
#include "EngineProfiling.h"
// To allow ImGui to trigger swapchain resize and handle global resolution controls!
#include "Display.h"

// TODO: Improve this tmp
constexpr int kResolutionArraySize = 8;

//===============================================================================

void GUI::Init(void* Hwnd, ID3D12Device* pDevice, int numFramesInFlight, const DXGI_FORMAT& renderTargetFormat)
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

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescriptor = {};
	descriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDescriptor.NumDescriptors = 1;
	descriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ASSERT_SUCCEEDED(pDevice->CreateDescriptorHeap(&descriptorHeapDescriptor, IID_PPV_ARGS(&m_pSrvDescriptorHeap)));

	// Set ImGui up with his heap
	ImGui_ImplDX12_Init(pDevice, numFramesInFlight, renderTargetFormat, m_pSrvDescriptorHeap,
		m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// Set app to use our custom style!
	// TODO: Remember that because Dear ImGui is a submodule, I need to fork it and commit stuff there in order for the changes to be reflected in Git!!!!
	//
	//ImGui::RTUAStyle(&ImGui::GetStyle());
	// 
	// TMP - added style in header of this file
	SetStyle();
}

void GUI::Run()
{
	// Start ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// The window size will have to adjust based on resolution changes
	//ImVec2 kMainWindowStartSize = { Graphics::g_DisplayWidth / 4.f, Graphics::g_DisplayHeight / 1.5f };

	ImGui::SetNextWindowPos(kMainWindowStartPos, ImGuiCond_FirstUseEver, kTopLeftPivot);
	ImGui::SetNextWindowSize(kMainWindowStartSize, ImGuiCond_FirstUseEver);

	if(!ImGui::Begin("RTUA"))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	// Display our lovely formatted title
	MainWindowTitle();

	if (ImGui::CollapsingHeader("Resolution Settings")) 
	{
		static int item_current_idx = DLSS::m_NumResolutions - 1;
		const char* combo_preview_value = DLSS::m_Resolutions[item_current_idx].first.c_str();

		// Check if the window has been resized manually - set the dropdown to custom if so
		// This checks that the newHeight has been set and that the actual current display has changed (via the flag)
		// Then, check that the size does not match!
		if (!m_bResolutionChangePending && (Graphics::g_DisplayWidth != m_NewWidth || Graphics::g_DisplayHeight != m_NewHeight))
			combo_preview_value = "Custom";

		if(ImGui::BeginCombo("Output Resolution", combo_preview_value))
		{
			for (int n = 0; n < DLSS::m_NumResolutions; n++)
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
			HRESULT hr = Display::GetSwapchain()->SetFullscreenState(!wbFullscreen, nullptr);
			if (SUCCEEDED(hr))
			{
				DEBUGPRINT("Switched to %s mode", m_bFullscreen ? "Fullscreen" : "Windowed");
			}
			else
			{
				DEBUGPRINT("\nFailed to toggle fullscreen mode.\n");
			}
		}
	}

	if (ImGui::CollapsingHeader("DLSS Settings"))
	{

		static bool useDLSS = false;
		static int dlssMode = 1; // 0: Performance, 1: Balanced, 2: Quality, etc.
		const char* modes[] = { "Performance", "Balanced", "Quality", "Ultra Performance"};

		if (ImGui::BeginCombo("Mode", modes[dlssMode]))
		{

			for (int n = 0; n < std::size(modes); n++)
			{
				const bool is_selected = (dlssMode == n);
				if (ImGui::Selectable(modes[n], is_selected))
				{
					dlssMode = n;
					// Update current mode
					DLSS::m_CurrentQualityMode = n;
					m_bUpdateDLSSMode = true;
				}

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();

		}

		if (ImGui::Checkbox("Enable DLSS", &useDLSS))
		{
			m_bToggleDLSS = useDLSS;
		}
	}

	if (ImGui::CollapsingHeader("Performance Metrics"))
	{
		// Frame data from MiniEngine profiler!
		static std::vector<float> cpuTimes, gpuTimes, frameTimes;

		// These functions will not exist when the mod is disabled - even though this function is never called when mod is disabled, compiler will fuss.
#if AZB_MOD
		cpuTimes.push_back(EngineProfiling::GetCPUTime());		// CPU time per frame
		gpuTimes.push_back(EngineProfiling::GetGPUTime());		// GPU time per frame
		frameTimes.push_back(EngineProfiling::GetFrameRate());  // Framerate
#endif

		// Limit buffer size
		if (cpuTimes.size() > 1000) cpuTimes.erase(cpuTimes.begin());
		if (gpuTimes.size() > 1000) gpuTimes.erase(gpuTimes.begin());
		if (frameTimes.size() > 1000) frameTimes.erase(frameTimes.begin());

		// Plot the data
		if (ImPlot::BeginPlot("Hardware Timings"))
		{
			// Setup axis, x then y. This will be Frame,Ms. Use autofit for now, will mess around with these later
			ImPlot::SetupAxes("Frame", "Speed(ms)", ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit);
			ImPlot::PlotLine("CPU Time", cpuTimes.data(), cpuTimes.size());
			ImPlot::PlotLine("GPU Time", gpuTimes.data(), gpuTimes.size());
			ImPlot::EndPlot();
		}

		if (ImPlot::BeginPlot("Frame Rate"))
		{
			ImPlot::SetupAxes("Count", "FPS", ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit);
			ImPlot::PlotLine("Frame Rate", frameTimes.data(), frameTimes.size());
			ImPlot::EndPlot();
		}
	}
	ImGui::End();
}

void GUI::UpdateGraphics()
{
	// See if resolution has been changed
	if (m_bResolutionChangePending)
	{
		if (!m_bFullscreen)
			// This version scales the window to match the resolution
			Display::SetWindowedResolution(m_NewWidth, m_NewHeight);
		else
		{
			// This version maintains fullscreen size based on what was queried at app startup and stretches the resolution to the display size
			Display::Resize(DLSS::m_MaxNativeResolution.m_Width, DLSS::m_MaxNativeResolution.m_Height);
			// We still need to update the pipeline to render at the lower resolution
			Display::SetPipelineResolution(false, m_NewWidth, m_NewHeight, true);
		}

		// Reset flag for next time
		m_bResolutionChangePending = false;
	}

	DLSS::UpdateDLSS(m_bToggleDLSS, m_bUpdateDLSSMode);
}

void GUI::Terminate()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	ImPlot::DestroyContext();
}

void GUI::MainWindowTitle()
{
	DoubleLineBreak();

	SectionTitle("Welcome to Real-Time Upscaling Analyser!");


	Separator();

	ImGui::TextWrapped("From this main window you can:");
	SingleLineBreak();
	SingleTabSpace();
	ImGui::BulletText("View performance metrics and debug data about the current upscaler");
	SingleLineBreak();
	SingleTabSpace();
	ImGui::BulletText("Tweak settings relating to the implementation of the current upscaler");
	SingleLineBreak();
	SingleTabSpace();
	ImGui::BulletText("Change the currently implemented upscaler or disable it entirely!");


	Separator();
}
