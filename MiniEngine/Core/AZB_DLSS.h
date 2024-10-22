#pragma once
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "nvsdk_ngx.h"
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

	// Initialise NDX and query for DLSS capability
	void Init(ID3D12Device* device, IDXGIAdapter* Adapter);

	// Query optimal resolution settings for a given DLSS Mode
	void QueryOptimalSettings(const int targetWidth, const int targetHeight, OptimalSettings& settings);

	// Pre-Query for all performance quality levels and store them. Useful for instantly switching at run-time, we can avoid running queries repeatedly
	void PreQueryAllSettings(const int targetWidth, const int targetHeight);

	// Cleanly shutdown and release all resources
	void Terminate();

	//
	// Namespace members
	// 
	 
	//NVSDK_NGX_Handle*		m_DLSS_FeatureHandle	= nullptr;

	// Handle to D3D
	extern ID3D12Device*		m_pD3DDevice;
	
	// Parameters for hardware capabilities and other NGX functionality
	extern NVSDK_NGX_Parameter*	m_DLSS_Parameters;

	// A container that represents DLSS "modes" and their settings
	// Array indices map to DLSS modes e.g. 0 = MaxPerformance etc
	extern std::array<OptimalSettings, 5> m_DLSS_Modes;


};