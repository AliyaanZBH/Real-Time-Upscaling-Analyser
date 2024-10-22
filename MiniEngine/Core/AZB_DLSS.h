#pragma once
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "nvsdk_ngx.h"

namespace DLSS
{
	// Initialise NDX and query for DLSS capability
	void Init(ID3D12Device* device, IDXGIAdapter* Adapter);

	// Query optimal resolution settings
	void QueryOptimalSettings(const int targetWidth, const int targetHeight);

	// Cleanly shutdown and release all resources
	void Terminate();

	//NVSDK_NGX_Handle*		m_DLSS_FeatureHandle	= nullptr;
	extern ID3D12Device*		m_pD3DDevice;
	extern NVSDK_NGX_Parameter*	m_DLSS_Parameters;
};