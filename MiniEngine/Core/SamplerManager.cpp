//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):  James Stanard
//             Alex Nankervis
//

#include "pch.h"
#include "SamplerManager.h"
#include "GraphicsCore.h"
#include "Hash.h"
#include <map>

using namespace std;
using namespace Graphics;

//===============================================================================
// desc: This is a helper class for samplers within the engine. DLSS requires that mip-map bias is set to a value lower than 0, ensuring that textures are sampled at display resolution instead of the downscaled render resolution!
//       See NVIDIA Docs section 3.5 for more info!
// modified: Aliyaan Zulfiqar
//===============================================================================

#if AZB_MOD
#include "AZB_DLSS.h"
#include <cmath>    // For log2
namespace SamplerManager
{
    map< size_t, D3D12_CPU_DESCRIPTOR_HANDLE > s_SamplerCache;
    void ReinitialiseSamplerCache(Resolution inputResolution, bool bOverride, float overrideLodBias)
    {
        float lodBias = 0.f;

        // [AZB]: Use the input resolution of DLSS
        float texLodXDimension = inputResolution.m_Width;

        // [AZB]: Use the formula of the DLSS programming guide for the LOD Bias...
        lodBias = std::log2f(texLodXDimension / DLSS::m_MaxNativeResolution.m_Width) - 1.0f;

        // [AZB]: ... but leave the opportunity to override it in the UI...
        if (bOverride)
        {
            lodBias = overrideLodBias;
        }

        for (auto& samplerPair : s_SamplerCache)
        {
            SamplerDesc samplerDesc;
            memcpy(&samplerDesc, &samplerPair.first, sizeof(SamplerDesc));

            samplerDesc.MipLODBias = lodBias;

            D3D12_CPU_DESCRIPTOR_HANDLE handle = samplerPair.second;
            samplerDesc.CreateDescriptor(handle);

            // Log updated sampler details (optional)
            Utility::Printf("Updated sampler with MipLODBias = %.2f", lodBias);
        }
    }
#else
// [AZB]: Original empty namespace
namespace
{
    map< size_t, D3D12_CPU_DESCRIPTOR_HANDLE > s_SamplerCache;
}
#endif
}

D3D12_CPU_DESCRIPTOR_HANDLE SamplerDesc::CreateDescriptor()
{
    size_t hashValue = Utility::HashState(this);
    auto iter = SamplerManager::s_SamplerCache.find(hashValue);
    if (iter != SamplerManager::s_SamplerCache.end())
    {
        return iter->second;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    g_Device->CreateSampler(this, Handle);
    return Handle;
}

void SamplerDesc::CreateDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    ASSERT(Handle.ptr != 0 && Handle.ptr != -1);
    g_Device->CreateSampler(this, Handle);
}
