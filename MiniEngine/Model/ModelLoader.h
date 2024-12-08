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
// Author:  James Stanard
//

#pragma once

#include "Model.h"
#include "Animation.h"
#include "ConstantBuffers.h"
#include "../Core/Math/BoundingSphere.h"
#include "../Core/Math/BoundingBox.h"

#include <cstdint>
#include <vector>

namespace glTF { class Asset; struct Mesh; }

#define CURRENT_MINI_FILE_VERSION 13

//===============================================================================
// desc: This is where models are loaded in and textures get converted to DDS. Additionaly where samplers are created and will need updating in order to meet DLSS mipBias requirement!
//       
// mod: Aliyaan Zulfiqar
//===============================================================================

//
// [AZB]: Custom includes and macro mods
//

// [AZB]: Container file for code modifications and other helper tools. Contains the global "AZB_MOD" macro.
#include "AZB_Utils.h"

#if AZB_MOD
#include "unordered_map"
#endif

namespace Renderer
{
    using namespace Math;

    // Unaligned mirror of MaterialConstants
    struct MaterialConstantData
    {
        float baseColorFactor[4]; // default=[1,1,1,1]
        float emissiveFactor[3]; // default=[0,0,0]
        float normalTextureScale; // default=1
        float metallicFactor; // default=1
        float roughnessFactor; // default=1
        uint32_t flags;
    };

    // Used at load time to construct descriptor tables
    struct MaterialTextureData
    {
        uint16_t stringIdx[kNumTextures];
        uint32_t addressModes;
    };

    // All of the information that needs to be written to a .mini data file
    struct ModelData
    {
        BoundingSphere m_BoundingSphere;
        AxisAlignedBox m_BoundingBox;
        std::vector<byte> m_GeometryData;
        std::vector<byte> m_AnimationKeyFrameData;
        std::vector<AnimationCurve> m_AnimationCurves;
        std::vector<AnimationSet> m_Animations;
        std::vector<uint16_t> m_JointIndices;
        std::vector<Matrix4> m_JointIBMs;
        std::vector<MaterialTextureData> m_MaterialTextures;
        std::vector<MaterialConstantData> m_MaterialConstants;
        std::vector<Mesh*> m_Meshes;
        std::vector<GraphNode> m_SceneGraph;
        std::vector<std::string> m_TextureNames;
        std::vector<uint8_t> m_TextureOptions;
    };

    struct FileHeader
    {
        char     id[4];   // "MINI"
        uint32_t version; // CURRENT_MINI_FILE_VERSION
        uint32_t numNodes;
        uint32_t numMeshes;
        uint32_t numMaterials;
        uint32_t meshDataSize;
        uint32_t numTextures;
        uint32_t stringTableSize;
        uint32_t geometrySize;
        uint32_t keyFrameDataSize;      // Animation data
        uint32_t numAnimationCurves;
        uint32_t numAnimations;
        uint32_t numJoints;     // All joints for all skins
        float    boundingSphere[4];
        float    minPos[3];
        float    maxPos[3];
    };

    void CompileMesh(
        std::vector<Mesh*>& meshList,
        std::vector<byte>& bufferMemory,
        glTF::Mesh& srcMesh,
        uint32_t matrixIdx,
        const Matrix4& localToObject,
        Math::BoundingSphere& boundingSphere,
        Math::AxisAlignedBox& boundingBox
    );

    bool BuildModel( ModelData& model, const glTF::Asset& asset, int sceneIdx = -1 );
    bool SaveModel( const std::wstring& filePath, const ModelData& model );
    
    std::shared_ptr<Model> LoadModel( const std::wstring& filePath, bool forceRebuild = false );

#if AZB_MOD
    // [AZB]: This maps addressModes (a combination of sampler settings) to offsets in our sampler heap, which shaders (and DLSS) will access at runtime
   // extern std::unordered_map<uint32_t, uint32_t> g_SamplerPermutations;

    // [AZB]: Function that dynamically updates mipBias in our samplers according to the formula supplied by NVIDIA in the DLSS docs
    void UpdateSamplers(const Model* scene, Resolution inputResolution, bool bOverride = false, float overrideLodBias = 0.f);

    // [AZB]: Maps materials to addressModes to allow us to update samplers at runtime!
    extern std::unordered_map<uint16_t, uint32_t> m_MaterialAddressModes;  // uint16_t is the materialCBV from the Mesh structure, which uniquely identifies the material. uint32_t stores the addressModes for that mat.

#endif
}