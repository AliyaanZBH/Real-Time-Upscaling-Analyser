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
}

void GUI::Run()
{
	// Start ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Taken from sample code
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);
		

		// Frame data from MiniEngine profiler!
		static std::vector<float> cpuTimes, gpuTimes, frameTimes;
		cpuTimes.push_back(EngineProfiling::GetCPUTime());		// CPU time per frame
		gpuTimes.push_back(EngineProfiling::GetGPUTime());		// GPU time per frame
		frameTimes.push_back(EngineProfiling::GetFrameRate());  // Framerate

		// Limit buffer size
		if (cpuTimes.size() > 2000) cpuTimes.erase(cpuTimes.begin());
		if (gpuTimes.size() > 2000) gpuTimes.erase(gpuTimes.begin());
		if (frameTimes.size() > 2000) frameTimes.erase(frameTimes.begin());

		// Plot the data
		if (ImPlot::BeginPlot("Hardware Timings"))
		{
			// Setup axis, x then y. This will be Frame,Ms. Use autofit for now, will mess around with these later
			ImPlot::SetupAxes("Frame", "MS", ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_::ImPlotAxisFlags_AutoFit);
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

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
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
