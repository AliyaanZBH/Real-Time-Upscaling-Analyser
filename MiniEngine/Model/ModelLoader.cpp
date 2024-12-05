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
//             Chuck Walbourn (ATG)
//
// This code depends on DirectXTex
//

#include "ModelLoader.h"
#include "Renderer.h"
#include "Model.h"
#include "glTF.h"
#include "ModelH3D.h"
#include "TextureManager.h"
#include "TextureConvert.h"
#include "GraphicsCommon.h"

#include <fstream>
#include <unordered_map>


using namespace Renderer;
using namespace Graphics;

//===============================================================================
// desc: This is where models are loaded in and textures get converted to DDS. 
// mod: Aliyaan Zulfiqar
//===============================================================================

#if AZB_MOD
#include "AZB_BistroRenderer.h"
#include "AZB_DLSS.h"
#endif

//#if AZB_MOD
//namespace Renderer
//{
//    std::unordered_map<uint32_t, uint32_t> g_SamplerPermutations;
//// [AZB]: Original permutations map, local to the modelLoader file
//#else
// [AZB]: This maps addressModes (a combination of sampler settings) to offsets in our sampler heap, which shaders (and DLSS) will access at runtime
std::unordered_map<uint32_t, uint32_t> g_SamplerPermutations;


#if AZB_MOD
namespace Renderer
{
    // [AZB]: Maps materials to addressModes to allow us to update samplers at runtime!
    std::unordered_map<uint16_t, uint32_t> m_MaterialAddressModes;  // uint16_t is the materialCBV from the Mesh structure, which uniquely identifies the material. uint32_t stores the addressModes for that mat.
}
#endif
//#endif

D3D12_CPU_DESCRIPTOR_HANDLE GetSampler(uint32_t addressModes)
{
    SamplerDesc samplerDesc;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(addressModes & 0x3);
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE(addressModes >> 2);

    //[AZB]: Experimenting with setting mip bias here
   //samplerDesc.MipLODBias = -2.f;
   //samplerDesc.MinLOD = -2.f;

    return samplerDesc.CreateDescriptor();
}

void LoadMaterials(Model& model,
    const std::vector<MaterialTextureData>& materialTextures,
    const std::vector<std::wstring>& textureNames,
    const std::vector<uint8_t>& textureOptions,
    const std::wstring& basePath)
{
    static_assert((_alignof(MaterialConstants) & 255) == 0, "CBVs need 256 byte alignment");

    // Load textures
    const uint32_t numTextures = (uint32_t)textureNames.size();
    model.textures.resize(numTextures);
    for (size_t ti = 0; ti < numTextures; ++ti)
    {
        std::wstring originalFile = basePath + textureNames[ti];
        CompileTextureOnDemand(originalFile, textureOptions[ti]);

        std::wstring ddsFile = Utility::RemoveExtension(originalFile) + L".dds";
        model.textures[ti] = TextureManager::LoadDDSFromFile(ddsFile);
    }

    // Generate descriptor tables and record offsets for each material
    const uint32_t numMaterials = (uint32_t)materialTextures.size();
    std::vector<uint32_t> tableOffsets(numMaterials);

    for (uint32_t matIdx = 0; matIdx < numMaterials; ++matIdx)
    {
        const MaterialTextureData& srcMat = materialTextures[matIdx];
        DescriptorHandle TextureHandles = Renderer::s_TextureHeap.Alloc(kNumTextures);
        uint32_t SRVDescriptorTable = Renderer::s_TextureHeap.GetOffsetOfHandle(TextureHandles);

        uint32_t DestCount = kNumTextures;
        uint32_t SourceCounts[kNumTextures] = { 1, 1, 1, 1, 1 };

        D3D12_CPU_DESCRIPTOR_HANDLE DefaultTextures[kNumTextures] =
        {
            GetDefaultTexture(kWhiteOpaque2D),
            GetDefaultTexture(kWhiteOpaque2D),
            GetDefaultTexture(kWhiteOpaque2D),
            GetDefaultTexture(kBlackTransparent2D),
            GetDefaultTexture(kDefaultNormalMap)
        };

        D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[kNumTextures];
        for (uint32_t j = 0; j < kNumTextures; ++j)
        {
            if (srcMat.stringIdx[j] == 0xffff)
                SourceTextures[j] = DefaultTextures[j];
            else
                SourceTextures[j] = model.textures[srcMat.stringIdx[j]].GetSRV();
        }

        g_Device->CopyDescriptors(1, &TextureHandles, &DestCount,
            DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // See if this combination of samplers has been used before.  If not, allocate more from the heap
        // and copy in the descriptors.
        uint32_t addressModes = srcMat.addressModes;

        // [AZB]: Store the addressModes so that we can use them when updating the samplers later!
#if AZB_MOD
        Renderer::m_MaterialAddressModes[matIdx] = addressModes;
#endif
        auto samplerMapLookup = g_SamplerPermutations.find(addressModes);

        if (samplerMapLookup == g_SamplerPermutations.end())
        {
            DescriptorHandle SamplerHandles = Renderer::s_SamplerHeap.Alloc(kNumTextures);
            uint32_t SamplerDescriptorTable = Renderer::s_SamplerHeap.GetOffsetOfHandle(SamplerHandles);
            g_SamplerPermutations[addressModes] = SamplerDescriptorTable;
            tableOffsets[matIdx] = SRVDescriptorTable | SamplerDescriptorTable << 16;

            D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[kNumTextures];
            for (uint32_t j = 0; j < kNumTextures; ++j)
            {
                SourceSamplers[j] = GetSampler(addressModes & 0xF);
                addressModes >>= 4;
            }
            g_Device->CopyDescriptors(1, &SamplerHandles, &DestCount,
                DestCount, SourceSamplers, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }
        else
        {
            tableOffsets[matIdx] = SRVDescriptorTable | samplerMapLookup->second << 16;
        }
    }

    // Update table offsets for each mesh
    uint8_t* meshPtr = model.m_MeshData.get();
    for (uint32_t i = 0; i < model.m_NumMeshes; ++i)
    {
        Mesh& mesh = *(Mesh*)meshPtr;
        uint32_t offsetPair = tableOffsets[mesh.materialCBV];
        mesh.srvTable = offsetPair & 0xFFFF;
        mesh.samplerTable = offsetPair >> 16;
        mesh.pso = Renderer::GetPSO(mesh.psoFlags);
        meshPtr += sizeof(Mesh) + (mesh.numDraws - 1) * sizeof(Mesh::Draw);
    }
}

std::shared_ptr<Model> Renderer::LoadModel(const std::wstring& filePath, bool forceRebuild)
{
    const std::wstring miniFileName = Utility::RemoveExtension(filePath) + L".mini";
    const std::wstring fileName = Utility::RemoveBasePath(filePath);

    struct _stat64 sourceFileStat;
    struct _stat64 miniFileStat;
    std::ifstream inFile;
    FileHeader header;

    bool sourceFileMissing = _wstat64(filePath.c_str(), &sourceFileStat) == -1;
    bool miniFileMissing = _wstat64(miniFileName.c_str(), &miniFileStat) == -1;

    if (sourceFileMissing)
        forceRebuild = false;

    if (sourceFileMissing && miniFileMissing)
    {
        Utility::Printf("Error: Could not find %ws\n", fileName.c_str());
        return nullptr;
    }

    bool needBuild = forceRebuild;

    // Check if .mini file exists and it is newer than source file
    if (miniFileMissing || !sourceFileMissing && sourceFileStat.st_mtime > miniFileStat.st_mtime)
        needBuild = true;

    // Check if it's an older version of .mini
    if (!needBuild)
    {
        inFile = std::ifstream(miniFileName, std::ios::in | std::ios::binary);
        inFile.read((char*)&header, sizeof(FileHeader));
        if (strncmp(header.id, "MINI", 4) != 0 || header.version != CURRENT_MINI_FILE_VERSION)
        {
            Utility::Printf("Model version deprecated.  Rebuilding %ws...\n", fileName.c_str());
            needBuild = true;
            inFile.close();
        }
    }

    if (needBuild)
    {
        if (sourceFileMissing)
        {
            Utility::Printf("Error: Could not find %ws\n", fileName.c_str());
            return nullptr;
        }

        ModelData modelData;

        const std::wstring fileExt = Utility::ToLower(Utility::GetFileExtension(filePath));

        if (fileExt == L"gltf" || fileExt == L"glb")
        {
            glTF::Asset asset(filePath);
            if (!BuildModel(modelData, asset))
                return nullptr;
        }
        else if (fileExt == L"h3d")
        {
            ModelH3D modelh3d;
            const std::wstring basePath = Utility::GetBasePath(filePath);
            if (!modelh3d.Load(filePath) || !modelh3d.BuildModel(modelData, basePath))
                return nullptr;
        }
        else
        {
            Utility::Printf(L"Unsupported model file extension: %ws\n", fileExt.c_str());
            return nullptr;
        }

        if (!SaveModel(miniFileName, modelData))
            return nullptr;

        inFile = std::ifstream(miniFileName, std::ios::in | std::ios::binary);
        inFile.read((char*)&header, sizeof(FileHeader));
    }

    if (!inFile)
        return nullptr;

    ASSERT(strncmp(header.id, "MINI", 4) == 0 && header.version == CURRENT_MINI_FILE_VERSION);

    std::wstring basePath = Utility::GetBasePath(filePath);

    std::shared_ptr<Model> model(new Model);

    model->m_NumNodes = header.numNodes;
    model->m_SceneGraph.reset(new GraphNode[header.numNodes]);
    model->m_NumMeshes = header.numMeshes;
    model->m_MeshData.reset(new uint8_t[header.meshDataSize]);

	if (header.geometrySize > 0)
	{
		UploadBuffer modelData;
		modelData.Create(L"Model Data Upload", header.geometrySize);
		inFile.read((char*)modelData.Map(), header.geometrySize);
		modelData.Unmap();
		model->m_DataBuffer.Create(L"Model Data", header.geometrySize, 1, modelData);
	}

    inFile.read((char*)model->m_SceneGraph.get(), header.numNodes * sizeof(GraphNode));
    inFile.read((char*)model->m_MeshData.get(), header.meshDataSize);

	if (header.numMaterials > 0)
	{
		UploadBuffer materialConstants;
		materialConstants.Create(L"Material Constant Upload", header.numMaterials * sizeof(MaterialConstants));
		MaterialConstants* materialCBV = (MaterialConstants*)materialConstants.Map();
		for (uint32_t i = 0; i < header.numMaterials; ++i)
		{
			inFile.read((char*)materialCBV, sizeof(MaterialConstantData));
			materialCBV++;
		}
		materialConstants.Unmap();
		model->m_MaterialConstants.Create(L"Material Constants", header.numMaterials, sizeof(MaterialConstants), materialConstants);
	}

    // Read material texture and sampler properties so we can load the material
    std::vector<MaterialTextureData> materialTextures(header.numMaterials);
    inFile.read((char*)materialTextures.data(), header.numMaterials * sizeof(MaterialTextureData));

    std::vector<std::wstring> textureNames(header.numTextures);
    for (uint32_t i = 0; i < header.numTextures; ++i)
    {
        std::string utf8TextureName;
        std::getline(inFile, utf8TextureName, '\0');
        textureNames[i] = Utility::UTF8ToWideString(utf8TextureName);
        // [AZB]: Store texture names for use in SDR renderer, to help determine which textures are cutouts and transparent.
#if AZB_MOD
        Bistro::m_TextureNames.push_back(utf8TextureName);
#endif
    }



    std::vector<uint8_t> textureOptions(header.numTextures);
    inFile.read((char*)textureOptions.data(), header.numTextures *sizeof(uint8_t));

    LoadMaterials(*model, materialTextures, textureNames, textureOptions, basePath);

    model->m_BoundingSphere = BoundingSphere(*(XMFLOAT4*)header.boundingSphere);
    model->m_BoundingBox = AxisAlignedBox(Vector3(*(XMFLOAT3*)header.minPos), Vector3(*(XMFLOAT3*)header.maxPos));

    // Load animation data
    model->m_NumAnimations = header.numAnimations;

    if (header.numAnimations > 0)
    {
        ASSERT(header.keyFrameDataSize > 0 && header.numAnimationCurves > 0);
        model->m_KeyFrameData.reset(new uint8_t[header.keyFrameDataSize]);
        inFile.read((char*)model->m_KeyFrameData.get(), header.keyFrameDataSize);
        model->m_CurveData.reset(new AnimationCurve[header.numAnimationCurves]);
        inFile.read((char*)model->m_CurveData.get(), header.numAnimationCurves * sizeof(AnimationCurve));
        model->m_Animations.reset(new AnimationSet[header.numAnimations]);
        inFile.read((char*)model->m_Animations.get(), header.numAnimations * sizeof(AnimationSet));
    }

    model->m_NumJoints = header.numJoints;

    if (header.numJoints > 0)
    {
        model->m_JointIndices.reset(new uint16_t[header.numJoints]);
        inFile.read((char*)model->m_JointIndices.get(), header.numJoints * sizeof(uint16_t));
        model->m_JointIBMs.reset(new Matrix4[header.numJoints]);
        inFile.read((char*)model->m_JointIBMs.get(), header.numJoints * sizeof(Matrix4));
    }

    return model;
}

#if AZB_MOD
void Renderer::UpdateSamplers(const Model* scene, Resolution inputResolution, bool bOverride, float overrideLodBias)
{
    // [AZB]: First, calculate the LOD bias
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

    // [AZB]: Update bias in DLSS namespace
    DLSS::m_LodBias = lodBias;

    // [AZB]: Iterate through all the address modes stored in the g_SamplerPermutations map
    for (auto& perm : g_SamplerPermutations)
    {
        // [AZB]: Perm.first is the addressMode, we need to fetch the corresponding descriptors
        uint32_t addressModes = perm.first;

        // [AZB]: Retrieve the original sampler descriptor handle from the heap using the saved table offset
        DescriptorHandle samplerHandles = Renderer::s_SamplerHeap[perm.second];

        // [AZB]: Create the array of source samplers to be updated
        D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[kNumTextures];
        uint32_t addressModeCopy = addressModes;

        // [AZB]: Iterate over the textures (e.g. kNumTextures textures) and create new samplers for each
        for (uint32_t j = 0; j < kNumTextures; ++j)
        {
            // [AZB]: Create a new sampler descriptor with the updated mipBias
            SamplerDesc updatedSamplerDesc = SamplerDesc();
            updatedSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(addressModeCopy & 0x3);         // U address mode - These all default to wrap according to GetSampler() and LoadMaterials()
            updatedSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE((addressModeCopy >> 2) & 0x3);  // V address mode - These all default to wrap according to GetSampler() and LoadMaterials()
            updatedSamplerDesc.MaxAnisotropy = 16;  // Keep fixed anisotropy
            updatedSamplerDesc.MipLODBias = lodBias;  // Set the mip bias dynamically

            // [AZB]: Create the new sampler handle using the updated descriptor
            SourceSamplers[j] = updatedSamplerDesc.CreateDescriptor();

            // [AZB]: Move to the next address mode component (4 bits for each of U, V, W)
            addressModeCopy >>= 4;
        }

        // [AZB]: Copy the new samplers into the original slot in the heap
        uint32_t DestCount = kNumTextures;

        g_Device->CopyDescriptors(1, &samplerHandles, &DestCount,
            DestCount, SourceSamplers, &DestCount,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }
}
#endif
