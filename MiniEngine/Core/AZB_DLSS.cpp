#include "AZB_DLSS.h"
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "Utility.h"

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


	NVSDK_NGX_PathListInfo pathListInfo = { &dlssPath, 1};
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

	if (ret == NVSDK_NGX_Result::NVSDK_NGX_Result_Success)
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

	NVSDK_NGX_Parameter* Params = nullptr;
	NVSDK_NGX_Result Result = NVSDK_NGX_D3D12_GetCapabilityParameters(&Params);

	NVSDK_NGX_Result resultUpdatedDriver = Params->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
	NVSDK_NGX_Result resultMinDriverVersionMajor = Params->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverVersionMajor);
	NVSDK_NGX_Result resultMinDriverVersionMinor = Params->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverVersionMinor);

	if (NVSDK_NGX_SUCCEED(resultUpdatedDriver))
	{
		if (needsUpdatedDriver)
		{
			// NVIDIA DLSS cannot be loaded due to outdated driver.
			if (NVSDK_NGX_SUCCEED(resultMinDriverVersionMajor) && NVSDK_NGX_SUCCEED(resultMinDriverVersionMinor))
			{
				// Min Driver Version required: minDriverVersionMajor.minDriverVersionMinor
				Utility::Printf("DLSS could not be loaded due to outdated driver, please upgrade to version %d", minDriverVersionMajor);
			}
			// Fallback to default AA solution (TAA etc)
			Utility::Print("Falling back to default AA solution");

		}
		else
		{
			// Driver update is not required
		}
	}

	// Secondary hardware query
	NVSDK_NGX_Result ResultDlssSupported = Params->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &DLSS_Supported);
	if (NVSDK_NGX_FAILED(ResultDlssSupported) || !DLSS_Supported)
	{
		Utility::Print("NVIDIA DLSS not available on this hardware");
	}

	// Sometimes DLSS is denied due to API permissions
	ResultDlssSupported = Params->Get(NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, &DLSS_Supported);
	if (NVSDK_NGX_FAILED(ResultDlssSupported) || !DLSS_Supported)
	{
		Utility::Print("NVIDIA DLSS is denied for this solution");
	}

	// Destroy parameter map
	NVSDK_NGX_D3D12_DestroyParameters(Params);
	Params = nullptr;


}

void DLSS::Terminate()
{
	NVSDK_NGX_D3D12_Shutdown();
}
