//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "AZB_GUI.h"

#include <d3d12.h>
#include <Utility.h>

// For performance metrics
#include "EngineProfiling.h"
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
	ImGui::RTUAStyle(&ImGui::GetStyle());
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
