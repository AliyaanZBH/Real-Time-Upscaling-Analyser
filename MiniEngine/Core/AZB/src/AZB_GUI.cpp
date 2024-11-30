//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "AZB_GUI.h"
#include "AZB_DLSS.h"

#include <Utility.h>

// For performance metrics
#include "EngineProfiling.h"
#include "Display.h"		// To allow ImGui to trigger swapchain resize and handle global resolution controls!
#include "BufferManager.h"	// For debug rendering buffers
#include "CommandContext.h"	// For transitioning resources
#include <TemporalEffects.h>

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

	// Begin integrating into mini engine

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescriptor = {};
	descriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDescriptor.NumDescriptors = eGBuffers::NUM_BUFFERS + 1;						// Increase and reserve spot for more textures! The first spot is used for built-in font hence the + 1
	descriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ASSERT_SUCCEEDED(pDevice->CreateDescriptorHeap(&descriptorHeapDescriptor, IID_PPV_ARGS(&m_pSrvDescriptorHeap)));

	// Set ImGui up with his heap
	ImGui_ImplDX12_Init(pDevice, numFramesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM, m_pSrvDescriptorHeap,
		m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// Store device for heap allocations!
	m_pD3DDevice = pDevice;
	
	// Set app to use our custom style!
	// TODO: Remember that because Dear ImGui is a submodule, I need to fork it and commit stuff there in order for the changes to be reflected in Git!!!!
	//
	//ImGui::RTUAStyle(&ImGui::GetStyle());
	// 
	
	// TMP - added style in header of this file
	SetStyle();

	// Set newWidth at the start so that it doesn't start at 0 and avoid DLSS crashing!
	m_NewWidth = Graphics::g_DisplayWidth;
	m_NewHeight = Graphics::g_DisplayHeight;

	// Also set up our window vars!
	m_MainWindowSize = { Graphics::g_DisplayWidth / 4.f, Graphics::g_DisplayHeight / 2.f };
	m_MainWindowPos = { (Graphics::g_DisplayWidth - m_MainWindowSize.x) - 10.f, 0.f };		// Additional offset as it doesn't sit quite flush without it

	// Store handles to GBuffers so that we can display them!
	m_GBuffers[eGBuffers::SCENE_COLOR] = Graphics::g_SceneColorBuffer.GetSRV();
	m_GBuffers[eGBuffers::SCENE_DEPTH] = Graphics::g_LinearDepth[TemporalEffects::GetFrameIndex()].GetSRV();
	m_GBuffers[eGBuffers::MOTION_VECTORS] = Graphics::g_DecodedVelocityBuffer.GetSRV();
	m_GBuffers[eGBuffers::VISUAL_MOTION_VECTORS] = Graphics::g_MotionVectorVisualisationBuffer.GetSRV();
}

void GUI::Run(CommandContext& Context)
{
	// Start ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

		// Display Modal if it is the first time!
	if (m_bShowStartupModal)
		StartupModal();
	else
		// Display the rest of our lovely Window!
	{

		ImGui::SetNextWindowPos(m_MainWindowPos, ImGuiCond_Appearing, kTopLeftPivot);
		ImGui::SetNextWindowSize(m_MainWindowSize, ImGuiCond_Appearing);

		if (!ImGui::Begin("RTUA"))
		{
			// Early out if the window is collapsed, as an optimization.
			ImGui::End();
			return;
		}

		// Begin with the lovely formatted title section
		MainWindowTitle();


		// Display resolution settings next!
		ResolutionSettings();


		if (ImGui::CollapsingHeader("DLSS Settings"))
		{

			// Only show the next section if DLSS is supported!
			if (DLSS::m_bIsNGXSupported)
			{



				//static bool useDLSS = false;
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

					for (int n = 0; n < std::size(modes); n++)
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
			ImGui::Checkbox("Enable PostFX", &m_bEnablePostFX);

		}


		if (ImGui::CollapsingHeader("Graphics Settings"))
		{

		

			static bool showMV = false;
			ImGui::Checkbox("Show GBuffers", &showMV);

			if (showMV)
			{
				//g_PerPixelMotionBuffer
								
				GraphicsContext& imguiGBufferContext = Context.GetGraphicsContext();


				imguiGBufferContext.TransitionResource(Graphics::g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				imguiGBufferContext.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				imguiGBufferContext.TransitionResource(Graphics::g_DecodedVelocityBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				imguiGBufferContext.TransitionResource(Graphics::g_MotionVectorVisualisationBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

				//
				// Update ImGui descriptor heap by adding GBuffer texture
				//
				
				// Open in new window
				ImGui::Begin("GBuffer Rendering Test",0, ImGuiWindowFlags_AlwaysAutoResize);

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

void GUI::UpdateGraphics()
{
	// See if resolution has been changed
	if (m_bResolutionChangePending)
	{
		// Regardless of specific change, DLSS will need to be recreated, and it cannot handle being on while a resize occurs!
		// Before it can be recreated, disable the flag to ensure it doesn't try to run while out of date!
		if (DLSS::m_DLSS_Enabled)
		{
			m_bToggleDLSS = false;
			m_bDLSSUpdatePending = true;
		}

		if (!m_bFullscreen)
			// This version scales the window to match the resolution - it also returns the size of the windows titlebar so that ImGui can generate a more user-friendly resolution for the user!
			m_TitleBarSize = Display::SetWindowedResolution(m_NewWidth, m_NewHeight);
		else
		{
			// This version maintains fullscreen size based on what was queried at app startup and stretches the resolution to the display size
			// We still need to update the pipeline to render at the lower resolution
			Display::SetPipelineResolution(false, m_NewWidth, m_NewHeight);
		}

		// Reset flag for next time
		m_bResolutionChangePending = false;
		// Also let DLSS know that the resolution has changed!
		DLSS::m_bNeedsReleasing = true;

		// Update window size and reset position!!
		//m_MainWindowSize = { (float)m_NewWidth / 4.f, (float)m_NewHeight / 2.f };
		m_MainWindowSize.y = (float)m_NewHeight / 2.f;
		m_MainWindowPos.x = ( (float)m_NewWidth - m_MainWindowSize.x ) - 10.f;
		ImGui::SetWindowSize("RTUA", m_MainWindowSize);
		ImGui::SetWindowPos("RTUA", m_MainWindowPos);
	}

	if (m_bDLSSUpdatePending)
	{
		// If we're fullscreen, we need to reset back to full res
		if (m_bFullscreen)
		{
			m_NewWidth = Graphics::g_DisplayWidth;
			m_NewHeight = Graphics::g_DisplayHeight;
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
	}
}

void GUI::Terminate()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	ImPlot::DestroyContext();
}

void GUI::StartupModal()
{
	ImGui::OpenPopup("Welcome!");
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, kCenterPivot);
	ImGui::BeginPopupModal("Welcome!", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	{
		DoubleLineBreak();

		SectionTitle("Welcome to Real-Time Upscaling Analyser!");

		Separator();

		ImGui::TextWrapped("If this is your first time using the app, or you'd like a refresher, please view the tutorial before using the tool.");
		SingleLineBreak();
		ImGui::TextWrapped("If you would like to jump straight into the tool, please feel free to do so!");
		DoubleLineBreak();

		// Highlight this baby!
		ImGui::Text("LCTRL+M to toggle input between the GUI and the scene!");

		// Calculate position to draw rect of last item
		ImVec2 firstItemPosMin = ImGui::GetItemRectMin();
		ImVec2 firstItemPosMax = ImGui::GetItemRectMax();

		float offset = 5.f;
		ImVec2 firstRectPosMin = ImVec2(firstItemPosMin.x - offset, firstItemPosMin.y - offset);
		ImVec2 firstRectPosMax = ImVec2(firstItemPosMax.x + offset, firstItemPosMax.y + offset);


		ImGui::GetWindowDrawList()->AddRect(firstRectPosMin, firstRectPosMax, ImColor(ThemeColours::m_HighlightColour), 0, ImDrawFlags_None, 3.f);

		DoubleLineBreak();

		// Temp var to store the text that will go inside a button
		const char* btnText = "Start Tutorial";
		// This makes it so the next item we define (in this case a button) will have the right size
		MakeNextItemFitText(btnText);
		// Center the button within the modal
		CenterNextTextItem(btnText);

		if (ImGui::Button(btnText))
			ImGui::CloseCurrentPopup();

		ImGui::SetItemDefaultFocus();

		SingleLineBreak();

		btnText = "Free Roam";
		MakeNextItemFitText(btnText);
		CenterNextTextItem(btnText);
		if (ImGui::Button(btnText))
		{
			m_bShowStartupModal = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

}

void GUI::MainWindowTitle()
{
	DoubleLineBreak();

	SectionTitle("Welcome to Real-Time Upscaling Analyser!");


	Separator();

	ImGui::TextWrapped("From this main window you can:");
	SingleLineBreak();

	// Use my wrapper to do a bunch of bullet points with text wrapping! Also self indents by 1 level!
	WrappedBullet("View performance metrics and debug data about the current upscaler");
	SingleLineBreak();
	WrappedBullet("Tweak settings relating to the implementation of the current upscaler");
	SingleLineBreak();
	WrappedBullet("Change the currently implemented upscaler or disable it entirely!");


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

void GUI::ResolutionSettings()
{
	std::string comboValue;
	// In order to make it clearer to users, create a variable combo label
	std::string comboLabel;

	if (ImGui::CollapsingHeader("Resolution Settings"))
	{
		static int item_current_idx = DLSS::m_NumResolutions - 1;

		// Check window display mode. 
		// When Fullscreen, our display height will be fixed (naturally), so we want to convey native height to the user
		// When Windowed, display height has variable, and can even be resized manually.
		// This nlock takes that into account and ensures that the dropdown is always as helpful as possible
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
			// This will actually attempt to go fullscreen
			HRESULT hr = Display::GetSwapchain()->SetFullscreenState(!wbFullscreen, nullptr);
			if (SUCCEEDED(hr))
			{
				DEBUGPRINT("Switched to %s mode", m_bFullscreen ? "Fullscreen" : "Windowed");

				// Update new width and height to fullscreen values if we have entered fullscreen
				if (m_bFullscreen)
				{
					m_NewWidth = DLSS::m_MaxNativeResolution.m_Width;
					m_NewHeight = DLSS::m_MaxNativeResolution.m_Height;
				}
				m_bResolutionChangePending = true;
				}
			else
			{
				DEBUGPRINT("\nFailed to toggle fullscreen mode.\n");
			}
			}
		}
}
