#pragma once
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "nvsdk_ngx.h"
#include "nvsdk_ngx_helpers.h"
#include "AZB_Utils.h"

#include <array>

namespace DLSS
{
	// This struct represents the optimal settings for each DLSS "mode" - such as their render target resolution other data.
	struct OptimalSettings
	{
		// Resolution values for lower resolution output buffer that DLSS will render to. E.g. when native resolution is set to 1080p, these may be set to 1280x720.
		unsigned int m_RenderWidth = 0;
		unsigned int m_RenderHeight = 0;

		// Maps to DLSS modes e.g. 1 = Balanced which I think is a good default setting for now
		const int m_PerfQualityValue = 1;
	};

	// Data required by DLSS to create the feature
	struct CreationRequirements
	{
		ID3D12GraphicsCommandList* m_pCmdList;
		unsigned int m_InCreationNodeMask = 1;					// These only matter for Multi GPU (default 1)
		unsigned int m_InVisibilityNodeMask = 1;				// These only matter for Multi GPU (default 1)
		NVSDK_NGX_DLSS_Create_Params m_DlSSCreateParams;
	};

	struct ExecutionRequirements
	{
		ID3D12GraphicsCommandList* m_pCmdList;
		NVSDK_NGX_D3D12_DLSS_Eval_Params m_DlSSEvalParams;	// Actual necessary date (e.g. Motion Vectors) are contained in here
	};

	// Query for NGX capabilities - ensuring that DLSS is possible on this device!
	void QueryFeatureRequirements(IDXGIAdapter* Adapter);

	// Initialise NDX
	void Init(ID3D12Device* device);

	// Query optimal resolution settings for a given DLSS Mode
	void QueryOptimalSettings(const int targetWidth, const int targetHeight, OptimalSettings& settings);

	// Pre-Query for all performance quality levels and store them. Useful for instantly switching at run-time, we can avoid running queries repeatedly
	void PreQueryAllSettings(const int targetWidth, const int targetHeight);

	// Create DLSS feature using the optimal settings
	void Create(CreationRequirements& reqs);

	// Execute the upscale step
	void Execute(ExecutionRequirements& params);

	// Turn DLSS on or off
	void ToggleDLSS(bool toggle);

	void SetD3DDevice(ID3D12Device* device);

	// Cleanly shutdown and release all resources
	void Terminate();

	//
	// Namespace members
	//  

	// Handle to DLSS feature
	extern NVSDK_NGX_Handle* m_DLSS_FeatureHandle;

	// Handle to D3D
	extern ID3D12Device* m_pD3DDevice;
	
	// Parameters for hardware capabilities and other NGX functionality
	extern NVSDK_NGX_Parameter*	m_DLSS_Parameters;

	// A container that represents DLSS "modes" and their settings
	// Array indices map to DLSS modes e.g. 0 = MaxPerformance etc
	extern std::array<OptimalSettings, 5> m_DLSS_Modes;

	// Flag that is determined by feauture capability query
	extern bool m_bIsNGXSupported;

	// Place to store debug logs from NGX
	extern const wchar_t* m_AppDataPath;

	// Flag to allow for runtime toggling of DLSS
	extern bool m_DLSS_Enabled;

	// Track the current native resolution
	extern Resolution m_CurrentNativeResolution;
};