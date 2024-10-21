#include "pch.h"
#include "AZB_DLSS.h"

void DLSS::Init(IDXGIAdapter* Adapter)
{
	// Contains information common to all NGX Features - required for Feature discovery, Initialization and Logging
	NVSDK_NGX_FeatureDiscoveryInfo FeatureDiscoveryInfo;

	// We don't have an NVIDIA project or app ID so we need to go through a couple extra setup steps before we can fill out our feature discovery struct
	NVSDK_NGX_ProjectIdDescription projectDesc = { "RTUA", NVSDK_NGX_ENGINE_TYPE_CUSTOM };
	NVSDK_NGX_Application_Identifier appID = { NVSDK_NGX_Application_Identifier_Type_Application_Id,  projectDesc };

	// Another couple setup structs, used to find fallback paths for DLSS DLL and more
	wchar_t const* path = L"../\../\ThirdParty/\DLSS/\lib/\dev";
	NVSDK_NGX_PathListInfo pathListInfo = { &path, 1};
	NVSDK_NGX_FeatureCommonInfo ftInfo = { pathListInfo };

	// Setup feature discovery struct as per NVIDIA docs
	FeatureDiscoveryInfo.SDKVersion = NVSDK_NGX_Version_API;
	FeatureDiscoveryInfo.FeatureID = NVSDK_NGX_Feature_SuperSampling;	// This is DLSS!
	FeatureDiscoveryInfo.Identifier = appID;

	FeatureDiscoveryInfo.ApplicationDataPath = L"C:/\ProgramData/\RTUA/\DLSS_Data";
	FeatureDiscoveryInfo.FeatureInfo = &ftInfo;


	// This is a pointer to a NVSDK_NGX_FeatureRequirement structure. Check the values returned in OutSupported if NVSDK_NGX_Result_Success is returned
	NVSDK_NGX_FeatureRequirement OutSupported;


	// Query hardware to see if DLSS is supported (RTX Cards only)
	NVSDK_NGX_D3D11_GetFeatureRequirements(Adapter, &FeatureDiscoveryInfo, &OutSupported);

	OutputDebugStringW(L"DLSS Query Pause");

	//NVSDK_NGX_D3D12_Init();
}
