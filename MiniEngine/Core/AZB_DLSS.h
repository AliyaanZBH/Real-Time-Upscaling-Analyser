#pragma once
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "nvsdk_ngx.h"

namespace DLSS
{

	void Init(ID3D12Device* device, IDXGIAdapter* Adapter);
	void Terminate();

	//NVSDK_NGX_Handle*		m_DLSS_FeatureHandle	= nullptr;
	//NVSDK_NGX_Parameter*	m_DLSS_Parameters		= nullptr;
};