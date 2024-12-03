#pragma once
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "nvsdk_ngx.h"
#include "nvsdk_ngx_helpers.h"
#include "AZB_Utils.h"

#include <array>
#include <vector>
#include <string>

namespace DLSS
{
	// This struct represents the optimal settings for each DLSS "mode" - such as their render target resolution other data.
	struct OptimalSettings
	{
		// Resolution values for lower resolution output buffer that DLSS will render to. E.g. when native resolution is set to 1080p, these may be set to 1280x720.
		unsigned int m_RenderWidth = 0;
		unsigned int m_RenderHeight = 0;

		// Maps to DLSS modes e.g. 1 = Balanced which I think is a good default setting for now. To be changed when implementing multiple modes
		int m_PerfQualityValue = 1;
	};

	// Data required by DLSS to create the feature
	struct CreationRequirements
	{
		ID3D12GraphicsCommandList* m_pCmdList;
		unsigned int m_InCreationNodeMask = 1;					// These only matter for Multi GPU (default 1)
		unsigned int m_InVisibilityNodeMask = 1;				// These only matter for Multi GPU (default 1)
		NVSDK_NGX_DLSS_Create_Params m_DlSSCreateParams = {};
	};

	struct ExecutionRequirements
	{
		ID3D12GraphicsCommandList* m_pCmdList;
		NVSDK_NGX_D3D12_DLSS_Eval_Params m_DlSSEvalParams;	// Actual necessary data (e.g. Motion Vectors) are contained in here
	};

	// Query for NGX capabilities - ensuring that DLSS is possible on this device!
	void QueryFeatureRequirements(IDXGIAdapter* Adapter);

	// Initialise NDX
	void Init(ID3D12Device* device);

	// Query optimal resolution settings for a given DLSS Mode
	void QueryOptimalSettings(const uint32_t targetWidth, const uint32_t targetHeight, OptimalSettings& settings);

	// Pre-Query for all performance quality levels and store them. Useful for instantly switching at run-time, we can avoid running queries repeatedly
	void PreQueryAllSettings(const uint32_t targetWidth, const uint32_t targetHeight);

	// Create DLSS feature using the optimal settings
	void Create(CreationRequirements& reqs);

	// Execute the upscale step
	void Execute(ExecutionRequirements& params);

	// Dispose of the DLSS feature - use when changing native resolution
	void Release();

	// Turn DLSS on and off, aswell as change quality mode
	void UpdateDLSS(bool toggle, bool updateMode, Resolution currentResolution);

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

	//
	// Resolution handling
	// 

	extern uint32_t m_NumResolutions;
	extern 	std::vector<std::pair<std::string, Resolution>> m_Resolutions;

	// Keep note of the maximum possible native resolution
	extern Resolution m_MaxNativeResolution;
	// Track the current native resolution
	extern Resolution m_CurrentNativeResolution;
	// Track the current internal rendering resolution
	extern Resolution m_CurrentInternalResolution;

	// Track which DLSS mode is currently selectd
	extern uint8_t m_CurrentQualityMode;

	// Place to store debug logs from NGX
	extern const wchar_t* m_AppDataPath;
	
	// Flag that is determined by feauture capability query
	extern bool m_bIsNGXSupported;

	// Flag to allow for runtime toggling of DLSS
	extern bool m_bDLSS_Enabled;

	// Flag to track when native resolution has changed and DLSS needs re-creating
	extern bool m_bNeedsReleasing;

	// Flag to track when the pipeline needs to update
	extern bool m_bPipelineUpdate;

	// Flag to track when the pipeline needs to reset back to native!
	extern bool m_bPipelineReset;
};