#include "AZB_DLSS.h"
//===============================================================================
// desc: This is the main class for DLSS integration and functionality. Using the namespace design to match the existing convention in the engine
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "Utility.h"
#include "CommandContext.h"	// To allow for creation and execution of DLSS, we need the global command context and queue

// Define externals to ensure no redefinition occurs elsewhere
namespace DLSS
{
	ID3D12Device* m_pD3DDevice = nullptr;
	NVSDK_NGX_Handle* m_DLSS_FeatureHandle = nullptr;
	NVSDK_NGX_Parameter* m_DLSS_Parameters = nullptr;
	std::array<OptimalSettings, 5> m_DLSS_Modes = {};

	uint32_t m_NumResolutions = 0;
	std::vector<std::pair<std::string, Resolution>> m_Resolutions;

	Resolution m_MaxNativeResolution = {};
	Resolution m_CurrentNativeResolution = {};

	uint8_t m_CurrentQualityMode = 1;

	const wchar_t* m_AppDataPath = L"./../../DLSS_Data/";

	bool m_bIsNGXSupported = false;
	bool m_DLSS_Enabled = false;
	bool m_bNeedsReleasing = false;
	bool m_bPipelineUpdate = false;
	bool m_bPipelineReset = false;
}

void DLSS::QueryFeatureRequirements(IDXGIAdapter* Adapter)
{
	// Contains information common to all NGX Features - required for Feature discovery, Initialization and Logging
	NVSDK_NGX_FeatureDiscoveryInfo FeatureDiscoveryInfo;

	// We don't have an NVIDIA project or app ID so we need to go through a couple extra setup steps before we can fill out our feature discovery struct
	NVSDK_NGX_ProjectIdDescription projectDesc = { "RTUA", NVSDK_NGX_ENGINE_TYPE_CUSTOM };
	NVSDK_NGX_Application_Identifier appID = { NVSDK_NGX_Application_Identifier_Type_Application_Id,  projectDesc };

	// Another couple setup structs, used to find fallback paths for DLSS DLL and more
	wchar_t const* dlssPath = L"./../../ThirdParty/DLSS/lib/dev/";

	NVSDK_NGX_PathListInfo pathListInfo = { &dlssPath, 1 };
	NVSDK_NGX_FeatureCommonInfo ftInfo = { pathListInfo };

	// Setup feature discovery struct as per NVIDIA docs
	FeatureDiscoveryInfo.SDKVersion = NVSDK_NGX_Version_API;
	FeatureDiscoveryInfo.FeatureID = NVSDK_NGX_Feature_SuperSampling;	// This is DLSS!
	FeatureDiscoveryInfo.Identifier = appID;

	FeatureDiscoveryInfo.ApplicationDataPath = m_AppDataPath;
	FeatureDiscoveryInfo.FeatureInfo = &ftInfo;

	// This is a pointer to a NVSDK_NGX_FeatureRequirement structure. Check the values returned in OutSupported if NVSDK_NGX_Result_Success is returned
	NVSDK_NGX_FeatureRequirement OutSupported;

	// Query hardware to see if DLSS is supported (RTX Cards only)
	// N.B. This helper detects NGX feature support for only static system conditions and the feature may still be determined to be unsupported at runtime when 
	// using other SDK entry points (i.e. not enough device memory may be available)
	NVSDK_NGX_Result ret = NVSDK_NGX_D3D12_GetFeatureRequirements(Adapter, &FeatureDiscoveryInfo, &OutSupported);

	// Assert that ret != fail
	if (NVSDK_NGX_SUCCEED(ret))
		// Set flag to indicate this device is NGX capable, ready to init and create features!
		m_bIsNGXSupported = true;
	else
		// Either we're not on an RTX card OR the dll file could be missing. 
		Utility::Print("\nNVIDIA DLSS not supported - have you got the right hardware and software?\n\n");
}

void DLSS::Init(ID3D12Device* device)
{
	NVSDK_NGX_Result ret;

	// Set up device pointer as we may need it later e.g. for shutdown
	m_pD3DDevice = device;
	// Check our flag based on the earlier query!
	if (m_bIsNGXSupported)
	{
		// Init NGX!
		ret = NVSDK_NGX_D3D12_Init(12345678910112021, m_AppDataPath, m_pD3DDevice);
		if (NVSDK_NGX_FAILED(ret))
			Utility::Print("\nNGX Failed to init, check D3D device!\n\n");
	}
	else
		return;

	// Secondary runtime check specifically for DLSS after device and NGX init
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

void DLSS::QueryOptimalSettings(const uint32_t targetWidth, const uint32_t targetHeight, OptimalSettings& settings)
{
	// Early return for non DLSS-Capable hardware
	if (!m_bIsNGXSupported)
		return;

	// These values need a valid memory address in order for the query to work, even though the values are ultimately unused!
	
	// Dynamic Maximum Render Size Width	- Unused due to no dynamic rendering currently implemented
	unsigned int maxDynamicW = 0;
	// Dynamic Maximum Render Size Height	- ^
	unsigned int maxDynamicH = 0;
	// Dynamic Minimum Render Size Width	- ^ 
	unsigned int minDynamicW = 0;
	// Dynamic Minimum Render Size Height	- ^ 
	unsigned int minDynamicH = 0;
	// Sharpness							- Deprecated value, but still needs passing through as a fallback
	float sharpness = 0;

	NVSDK_NGX_Result ret = NGX_DLSS_GET_OPTIMAL_SETTINGS(
		m_DLSS_Parameters,
		targetWidth, targetHeight,
		static_cast<NVSDK_NGX_PerfQuality_Value>(settings.m_PerfQualityValue),
		&settings.m_RenderWidth, &settings.m_RenderHeight,
		&maxDynamicW,
		&maxDynamicH,
		&minDynamicW,
		&minDynamicH,
		&sharpness
		);

	if (settings.m_RenderWidth == 0 || settings.m_RenderHeight == 0)
	{
		// Mode hasn't been found so inform user
		Utility::Print("\nThis PerfQuality mode has not been made available yet.\n\n");
		Utility::Print("\nPlease request another PerfQuality mode.\n\n");
	}

	// Store target resolution for later use
	m_CurrentNativeResolution = { targetWidth, targetHeight };
}

void DLSS::PreQueryAllSettings(const uint32_t targetWidth, const uint32_t targetHeight)
{
	// See QueryOptimalSettings for more explanation on these values
	unsigned int maxDW = 0;
	unsigned int maxDH = 0;
	unsigned int minDW = 0;
	unsigned int minDH = 0;
	float sharpness = 0;

	// PerfQualityValue [0 MaxPerformance, 1 Balance, 2 MaxQuality, 3 UltraPerformance, 4 UltraQuality]
	// DLAA sits here at 5, but NVIDIA recommend exposing it as a different UI option later.
	// Check all settings by looping through the integer values of the enum
	for (int perfQualityValue = 0; perfQualityValue < 5; ++perfQualityValue)
	{
		NVSDK_NGX_Result ret = NGX_DLSS_GET_OPTIMAL_SETTINGS(
			m_DLSS_Parameters,
			targetWidth, targetHeight,
			static_cast<NVSDK_NGX_PerfQuality_Value>(perfQualityValue),
			&m_DLSS_Modes[perfQualityValue].m_RenderWidth, &m_DLSS_Modes[perfQualityValue].m_RenderHeight,
			&maxDW, &maxDH, &minDW, &minDH, &sharpness
		);

		// All the modes start with a default value of 1 - which corresponds to Balanced. Update this to match the correct one
		m_DLSS_Modes[perfQualityValue].m_PerfQualityValue = perfQualityValue;
	}

	// Store target resolution for later use
	m_CurrentNativeResolution = { targetWidth, targetHeight };
}

void DLSS::Create(CreationRequirements& reqs)
{
	// Early return for non DLSS-Capable hardware
	if (!m_bIsNGXSupported)
		return;

	// [AZB]: Set additional feature flags!
	// [AZB]: Even though we may not render to HDR, our color buffer is infact in HDR format, so set the appropriate flag!
	reqs.m_DlSSCreateParams.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_DepthInverted | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes/*| NVSDK_NGX_DLSS_Feature_Flags_IsHDR*/;

	NVSDK_NGX_Result ret = NGX_D3D12_CREATE_DLSS_EXT(reqs.m_pCmdList, 1, 1, &m_DLSS_FeatureHandle, m_DLSS_Parameters, &reqs.m_DlSSCreateParams);
	if (NVSDK_NGX_SUCCEED(ret))
	{
		Utility::Print("\nDLSS created for the current resolution succesfully!\n\n");
	}
	else
		Utility::Print("\nDLSS could not be created - something is not integrated correctly within the rendering pipeline\n\n");

}

void DLSS::Execute(ExecutionRequirements& params)
{
	// Early return for non DLSS-Capable hardware
	if (!m_bIsNGXSupported)
		return;

	// TMP: DEBUGGGIN MVS
	//params.m_DlSSEvalParams.InMVScaleX = 50.5f;
	//params.m_DlSSEvalParams.InMVScaleY = 50.5f;

	NVSDK_NGX_Result ret = NGX_D3D12_EVALUATE_DLSS_EXT(params.m_pCmdList, m_DLSS_FeatureHandle, m_DLSS_Parameters, &params.m_DlSSEvalParams);
	if (NVSDK_NGX_SUCCEED(ret))
		Utility::Print("\nDLSS executed!!\nCheck that the final image looks right!\n\n");
	else
		Utility::Print("\nDLSS could not be evaluated - something is not integrated correctly within the rendering pipeline\n\n");
}

void DLSS::Release()
{
	NVSDK_NGX_D3D12_ReleaseFeature(m_DLSS_FeatureHandle);
}

void DLSS::UpdateDLSS(bool toggle, bool updateMode, Resolution currentResolution)
{
	// Flip our flag
	m_DLSS_Enabled = toggle;

	// If we are going from disabled to enabled, we need to recreate the feature if the target resolution has changed!
	// If the mode has changed, we also need to recreate DLSS with this value
	if (m_DLSS_Enabled || updateMode)
	{
		
		// Check if feature has already been created, release if so
		if(m_DLSS_FeatureHandle != nullptr)
		{
			Release();
			// Reset flag
			m_bNeedsReleasing = false;

			// Begin creating the feature and for execution next frame
			ComputeContext& dlssContext = ComputeContext::Begin(L"DLSS Enable");

			// Query for recommended settings
			PreQueryAllSettings(currentResolution.m_Width, currentResolution.m_Height);

			// Fill in requirements struct ready for the feature creation
			DLSS::CreationRequirements reqs;
			reqs.m_pCmdList = dlssContext.GetCommandList();

			NVSDK_NGX_Feature_Create_Params dlssParams = { m_DLSS_Modes[m_CurrentQualityMode].m_RenderWidth, m_DLSS_Modes[m_CurrentQualityMode].m_RenderHeight,
														   currentResolution.m_Width, currentResolution.m_Height, static_cast<NVSDK_NGX_PerfQuality_Value>(m_CurrentQualityMode) };

			// Let the create function itself handle additional feature flags
			reqs.m_DlSSCreateParams = NVSDK_NGX_DLSS_Create_Params{ dlssParams, NVSDK_NGX_DLSS_Feature_Flags_None };
			DLSS::Create(reqs);

			// Set flag to update the pipeline so that DLSS can execute next frame at correct resolution! See TemporalEffects.cpp, ResolveImage() to see implementation
			m_bPipelineUpdate = true;

			// Close context
			dlssContext.Finish();
		}
		// If DLSS has already been created (e.g. in Resize() event) DLSS has already been created for the correct output resolution
	}
	else
	{
		// If DLSS is being disabled, we don't necessarily need to release the full feature, but we do need to reset the pipeline still!
		m_bPipelineReset = true;
		m_bNeedsReleasing = true;
	}

	// Update current resolution
	m_CurrentNativeResolution = currentResolution;

}

void DLSS::SetD3DDevice(ID3D12Device* device)
{
	m_pD3DDevice = device;
}

void DLSS::Terminate()
{
	// Destroy parameter map
	NVSDK_NGX_D3D12_DestroyParameters(m_DLSS_Parameters);
	m_DLSS_Parameters = nullptr;

	// Shutdown NGX Cleanly
	NVSDK_NGX_D3D12_Shutdown1(m_pD3DDevice);
}
