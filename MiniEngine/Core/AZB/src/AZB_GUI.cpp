//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "AZB_GUI.h"

#include <d3d12.h>
#include <Utility.h>
#include <array>

// For performance metrics
#include "EngineProfiling.h"
#include "AZB_DLSS.h"

// To allow ImGui to trigger swapchain resize!
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

	{
		int renderWidth = 1920;
		int renderHeight = 1080;
		int outputWidth = 1920;
		int outputHeight = 1080;

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
			// Create pre-made resolutions - App should start at 1080p
			static std::array<std::pair<std::string, Resolution>, kResolutionArraySize> resolutionPresets{  std::pair<std::string, Resolution>("360p", Resolution{ 640, 360 }),
																						 std::pair<std::string, Resolution>("540p", Resolution{ 960, 540 }),
																						 std::pair<std::string, Resolution>("720p", Resolution{ 1280, 720 }),
																						 std::pair<std::string, Resolution>("768p", Resolution{ 1366, 768 }),
																						 std::pair<std::string, Resolution>("900p", Resolution{ 1600, 900 }),
																						 std::pair<std::string, Resolution>("1050p", Resolution{ 1680, 1050 }),
																						 std::pair<std::string, Resolution>("1080p", Resolution{ 1920, 1080 }),
																						 std::pair<std::string, Resolution>("Custom", Resolution{ 1920, 1080 })	// Leave it as max for now

			};


			static int item_current_idx = kResolutionArraySize - 1;
			const char* combo_preview_value = resolutionPresets[item_current_idx].first.c_str();

			if(ImGui::BeginCombo("Output Resolution", combo_preview_value))
			{
				for (int n = 0; n < kResolutionArraySize; n++)
				{
					const bool is_selected = (item_current_idx == n);
					if (ImGui::Selectable(resolutionPresets[n].first.c_str(), is_selected))
					{
						item_current_idx = n;
						// TODO:
						// Select resolution and resize swapchain, which should in turn requery DLSS!
						Display::SetResolution(resolutionPresets[n].second.m_Width, resolutionPresets[n].second.m_Height);

						// This version maintains the display - something interesting worth investigating!
						//Display::Resize(resolutionPresets[n].second.m_Width, resolutionPresets[n].second.m_Height);
					}

					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			
			// Check if the window has been resized manually - set the dropdown to custom if so
			if (Graphics::g_DisplayHeight != resolutionPresets[item_current_idx].second.m_Height)
				item_current_idx = kResolutionArraySize - 1;

			// Apply changes
			//if (renderWidth != currentRenderWidth || renderHeight != currentRenderHeight ||
			//	outputWidth != currentOutputWidth || outputHeight != currentOutputHeight) {
			//	currentRenderWidth = renderWidth;
			//	currentRenderHeight = renderHeight;
			//	currentOutputWidth = outputWidth;
			//	currentOutputHeight = outputHeight;
			//	ReconfigureDLSS(currentRenderWidth, currentRenderHeight, currentOutputWidth, currentOutputHeight);
			//}


			// Add a checkbox to control fullscreen
			static bool bFullscreen = false;
			// wb for Windows Bool - have to use this when querying the swapchain!
			BOOL wbFullscreen = FALSE;
			Display::GetSwapchain()->GetFullscreenState(&wbFullscreen, nullptr);

			bFullscreen = wbFullscreen;

			if (ImGui::Checkbox("Enable fullscreen mode", &bFullscreen))
			{
				HRESULT hr = Display::GetSwapchain()->SetFullscreenState(!wbFullscreen, nullptr);
				if (SUCCEEDED(hr))
				{
					bFullscreen = wbFullscreen;
					DEBUGPRINT("Switched to %s mode", bFullscreen ? "Fullscreen" : "Windowed");
				}
				else
				{
					DEBUGPRINT("\nFailed to toggle fullscreen mode.\n");
				}
			}
		}

		if (ImGui::CollapsingHeader("DLSS Settings")) {

			static bool useDLSS = false;

			if (ImGui::Checkbox("Enable DLSS", &useDLSS))
			{
				DLSS::ToggleDLSS(useDLSS); 
			}

			static int dlssMode = 1; // 0: Performance, 1: Balanced, 2: Quality, etc.
			const char* modes[] = { "Performance", "Balanced", "Quality", "Ultra Performance", "Ultra Quality" };

			if(ImGui::BeginCombo("Mode", modes[dlssMode]))
			{

				for (int n = 0; n < std::size(modes); n++)
				{
					const bool is_selected = (dlssMode == n);
					if (ImGui::Selectable(modes[n], is_selected))
					{
						dlssMode = n;
						// TODO:
						// Select setting and requery DLSS
					}

					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();

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
