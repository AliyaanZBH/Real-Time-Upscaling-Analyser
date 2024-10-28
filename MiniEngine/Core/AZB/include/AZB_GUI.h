#pragma once
//===============================================================================
// desc: A UI superclass that utilises ImGui to create a functional UI for education and performance profiling
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "implot.h"
//===============================================================================

class GUI
{
public:
	GUI() {}

	// Get ImGui started!
	void Init(void* Hwnd, ID3D12Device* pDevice, int numFramesInFlight, const DXGI_FORMAT& renderTargetFormat);

	// Run ImGui render loop!
	void Run();

	// Shutdown ImGui safely
	void Terminate();

	// A handle to render ImGui within MiniEngine
	ID3D12DescriptorHeap* m_pSrvDescriptorHeap = nullptr;

private:


	//
	//	Functions to break up smaller sections of the UI
	//

	void MainWindowTitle();

	//
	// Constants to help with the UI
	//

	// Situate window on the left side, just underneath the existing profiling
	const ImVec2 kMainWindowStartPos = { 0.f, 200.f };

	// How big is the window at program start?
	const ImVec2 kMainWindowStartSize = { 1180.f, 720.f };

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
	const void SectionTitle(const char* titleText) {
		CenterNextTextItem(titleText);
		ImGui::Text(titleText);
	}
};