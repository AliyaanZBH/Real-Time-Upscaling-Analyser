#include "AZB_DLSS.h"
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "Utility.h"
#include "nvsdk_ngx_helpers.h"

namespace DLSS {
	NVSDK_NGX_Parameter* m_DLSS_Parameters = nullptr;
}

void DLSS::Init(ID3D12Device* device, IDXGIAdapter* Adapter)
{
	// Contains information common to all NGX Features - required for Feature discovery, Initialization and Logging
	NVSDK_NGX_FeatureDiscoveryInfo FeatureDiscoveryInfo;

	// We don't have an NVIDIA project or app ID so we need to go through a couple extra setup steps before we can fill out our feature discovery struct
	NVSDK_NGX_ProjectIdDescription projectDesc = { "RTUA", NVSDK_NGX_ENGINE_TYPE_CUSTOM };
	NVSDK_NGX_Application_Identifier appID = { NVSDK_NGX_Application_Identifier_Type_Application_Id,  projectDesc };

	// Another couple setup structs, used to find fallback paths for DLSS DLL and more
	wchar_t const* dlssPath = L"../\../\ThirdParty/\DLSS/\lib/\dev";
	const wchar_t* appDataPath = L"C:/\ProgramData/\RTUA/\DLSS_Data";


	NVSDK_NGX_PathListInfo pathListInfo = { &dlssPath, 1 };
	NVSDK_NGX_FeatureCommonInfo ftInfo = { pathListInfo };

	// Setup feature discovery struct as per NVIDIA docs
	FeatureDiscoveryInfo.SDKVersion = NVSDK_NGX_Version_API;
	FeatureDiscoveryInfo.FeatureID = NVSDK_NGX_Feature_SuperSampling;	// This is DLSS!
	FeatureDiscoveryInfo.Identifier = appID;

	FeatureDiscoveryInfo.ApplicationDataPath = appDataPath;
	FeatureDiscoveryInfo.FeatureInfo = &ftInfo;


	// This is a pointer to a NVSDK_NGX_FeatureRequirement structure. Check the values returned in OutSupported if NVSDK_NGX_Result_Success is returned
	NVSDK_NGX_FeatureRequirement OutSupported;

	// Query hardware to see if DLSS is supported (RTX Cards only)
	// N.B. This helper detects NGX feature support for only static system conditions and the feature may still be determined to be unsupported at runtime when 
	// using other SDK entry points (i.e. not enough device memory may be available)
	NVSDK_NGX_Result ret = NVSDK_NGX_D3D12_GetFeatureRequirements(Adapter, &FeatureDiscoveryInfo, &OutSupported);

	// Assert that ret != fail
	if (NVSDK_NGX_SUCCEED(ret))
		// Init NGX!
		NVSDK_NGX_D3D12_Init(12345678910112021, appDataPath, device);
	else
		return;

	// Secondary runtime check after device and NGX init
	// Successful initialization of the NGX SDK instance indicates that the target system is capable of running NGX features. However, each feature can have additional dependencies

	int DLSS_Supported = 0;
	int needsUpdatedDriver = 0;
	unsigned int minDriverVersionMajor = 0;
	unsigned int minDriverVersionMinor = 0;

	// Important step! Fill in our member parameters so that we can refer back to them and use them throughout the lifetime of the app
	NVSDK_NGX_Result Result = NVSDK_NGX_D3D12_GetCapabilityParameters(&m_DLSS_Parameters);

	// Secondary hardware query
	NVSDK_NGX_Result ResultDlssSupported = m_DLSS_Parameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &DLSS_Supported);
	if (NVSDK_NGX_FAILED(ResultDlssSupported) || !DLSS_Supported)
		Utility::Print("\nNVIDIA DLSS not available on this hardware\n\n");


	// Check if the software is at the minimum version for running DLSS, and if it isn't find out what version is needed to help the user get the right one
	NVSDK_NGX_Result resultUpdatedDriver = m_DLSS_Parameters->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
	NVSDK_NGX_Result resultMinDriverVersionMajor = m_DLSS_Parameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverVersionMajor);
	NVSDK_NGX_Result resultMinDriverVersionMinor = m_DLSS_Parameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverVersionMinor);

	// Check our queries and assist the user as appropriate
	if (NVSDK_NGX_SUCCEED(resultUpdatedDriver))
	{
		if (needsUpdatedDriver)
		{
			// If the end user needs to update the driver, check that our queries for the necessary driver version succeeded, then feed this info to the user
			if (NVSDK_NGX_SUCCEED(resultMinDriverVersionMajor) && NVSDK_NGX_SUCCEED(resultMinDriverVersionMinor))
				// Min Driver Version required: minDriverVersionMajor.minDriverVersionMinor
				Utility::Printf("\nDLSS could not be loaded due to outdated driver, please upgrade to version %d.%d\n\n", minDriverVersionMajor, minDriverVersionMinor);
			else
				// If we can't find the minimum driver required, print a generic update message atleast
				Utility::Print("\nDLSS could not be loaded due to outdated driver, please upgrade to latest stable version\\nn");
		}
		// Else, a driver update is not required and we're good to go!
	}

	// Sometimes DLSS is denied due to API permissions
	ResultDlssSupported = m_DLSS_Parameters->Get(NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, &DLSS_Supported);
	if (NVSDK_NGX_FAILED(ResultDlssSupported) || !DLSS_Supported)
		Utility::Print("\nNVIDIA DLSS is denied for this solution\n\n");
	else
		// Final message to indicate success!
		Utility::Print("\nNVIDIA DLSS is supported!\n\n");

}

void DLSS::QueryOptimalSettings(const int targetWidth, const int targetHeight)
{
	unsigned int optimalRenderWidth = 0;
	unsigned int optimalRenderHeight = 0;

	// PerfQualityValue [0 MaxPerformance, 1 Balance, 2 MaxQuality, 3 UltraPerformance, 4 UltraQuality]
	// Check all settings by loopiung through the integer values of the enum
	for (int perfQuality = 0; perfQuality <= 4; ++perfQuality)
	{
		NVSDK_NGX_Result DLSSMode = NGX_DLSS_GET_OPTIMAL_SETTINGS(
			m_DLSS_Parameters,
			targetWidth, targetHeight,
			static_cast<NVSDK_NGX_PerfQuality_Value>(perfQuality),
			&optimalRenderWidth, &optimalRenderHeight,
			0,																// Dynamic Maximum Render Size Width	- Unused due to no dynamic rendering currently implemented
			0,																// Dynamic Maximum Render Size Height	- ^
			0,																// Dynamic Minimum Render Size Width	- ^ 
			0,																// Dynamic Minimum Render Size Height	- ^ 
			0																// Sharpness							- Deprecated value, but may need enabling as a fallback 
			);

		if (optimalRenderWidth == 0 || optimalRenderHeight == 0)
		{
			// This PerfQuality mode has not been made available yet.
			// Please request another PerfQuality mode.

		}
		else
		{
			// Use DLSS for this combination
			// - Create feature with RecommendedOptimalRenderWidth, RecommendedOptimalRenderHeight
			// - Render to (RenderWidth, RenderHeight) between Min and Max inclusive
			// - Call DLSS to upscale to (TargetWidth, TargetHeight)
		}
	}
}

void DLSS::Terminate()
{
	// Destroy parameter map
	NVSDK_NGX_D3D12_DestroyParameters(m_DLSS_Parameters);
	m_DLSS_Parameters = nullptr;

	// Shutdown NGX Cleanly
	NVSDK_NGX_D3D12_Shutdown();
}
