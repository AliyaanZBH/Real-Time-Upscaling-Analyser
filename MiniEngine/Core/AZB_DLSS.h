#pragma once
//===============================================================================
// desc: This is the main class for DLSS integration and functionality
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "nvsdk_ngx.h"


void Init()
{
	// Query hardware to see if DLSS is supported (RTX Cards only)
	NVSDK_NGX_Result NVSDK_NGX_D3D11_GetFeatureRequirements(
		IDXGIAdapter * Adapter,
		// Contains information common to all NGX Features - required for Feature discovery, Initialization and Logging
		const NVSDK_NGX_FeatureDiscoveryInfo * FeatureDiscoveryInfo,
		// This is a pointer to a NVSDK_NGX_FeatureRequirement structure. Check the values returned in OutSupported if NVSDK_NGX_Result_Success is returned
		NVSDK_NGX_FeatureRequirement * OutSupported);				 
}