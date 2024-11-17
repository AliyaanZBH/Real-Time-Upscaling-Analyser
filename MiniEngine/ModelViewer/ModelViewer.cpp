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

#include "GameCore.h"
#include "CameraController.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "TemporalEffects.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "PostEffects.h"
#include "SSAO.h"
#include "FXAA.h"
#include "SystemTime.h"
#include "TextRenderer.h"
#include "ParticleEffectManager.h"
#include "GameInput.h"
#include "SponzaRenderer.h"
#include "glTF.h"
#include "Renderer.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ShadowCamera.h"
#include "Display.h"

//===============================================================================
// desc: This is the  "GameApp", where the app specific settings are created, e.g. models to load, rendering stages to complete.
//       Heavily modified at this point.
// modified: Aliyaan Zulfiqar
//===============================================================================

/*
   Change Log:
   [AZB] 13/11/24: Added a check to apply post-processing to DLSS_OutputBuffer and not the main color buffer when DLSS enabled!

*/


//
// [AZB]: Custom includes and macro mods
//

// [AZB]: Container file for code modifications and other helper tools. Contains the global "AZB_MOD" macro.
#include "AZB_Utils.h"




// [AZB]: These will only be included if the global modificiation macro is defined as true (=1)
#if AZB_MOD
#include "AZB_DLSS.h"

#include "TextureConvert.h"     // For converting HDRI PNGs to DDS
#endif

//#define LEGACY_RENDERER

using namespace GameCore;
using namespace Math;
using namespace Graphics;
using namespace std;

using Renderer::MeshSorter;

class ModelViewer : public GameCore::IGameApp
{
public:

    ModelViewer( void ) {}

    virtual void Startup( void ) override;
    virtual void Cleanup( void ) override;

    virtual void Update( float deltaT ) override;
    virtual void RenderScene( void ) override;

private:

    Camera m_Camera;
    unique_ptr<CameraController> m_CameraController;

    D3D12_VIEWPORT m_MainViewport;
    D3D12_RECT m_MainScissor;

    ModelInstance m_ModelInst;
    ShadowCamera m_SunShadowCamera;
};

CREATE_APPLICATION( ModelViewer )

ExpVar g_SunLightIntensity("Viewer/Lighting/Sun Light Intensity", 4.0f, 0.0f, 16.0f, 0.1f);
NumVar g_SunOrientation("Viewer/Lighting/Sun Orientation", -0.5f, -100.0f, 100.0f, 0.1f );
NumVar g_SunInclination("Viewer/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f );

void ChangeIBLSet(EngineVar::ActionType);
void ChangeIBLBias(EngineVar::ActionType);

DynamicEnumVar g_IBLSet("Viewer/Lighting/Environment", ChangeIBLSet);
std::vector<std::pair<TextureRef, TextureRef>> g_IBLTextures;

#if AZB_MOD
// [AZB]: Default IBL bias makes Bistro appear in pure Chrome!
NumVar g_IBLBias("Viewer/Lighting/Gloss Reduction", 8.0f, 0.0f, 16.0f, 1.0f, ChangeIBLBias);
#else
NumVar g_IBLBias("Viewer/Lighting/Gloss Reduction", 2.0f, 0.0f, 10.0f, 1.0f, ChangeIBLBias);
#endif
void ChangeIBLSet(EngineVar::ActionType)
{
    int setIdx = g_IBLSet - 1;
    if (setIdx < 0)
    {
        Renderer::SetIBLTextures(nullptr, nullptr);
    }
    else
    {
        auto texturePair = g_IBLTextures[setIdx];
        Renderer::SetIBLTextures(texturePair.first, texturePair.second);
    }
}

void ChangeIBLBias(EngineVar::ActionType)
{
    Renderer::SetIBLBias(g_IBLBias);
}

#include <direct.h> // for _getcwd() to check data root path

#if AZB_MOD
// [AZB]: Modified method that detects PNGs and HDRs and converts to DDS!
void LoadIBLTextures()
{
    char CWD[256];
    _getcwd(CWD, 256);

    Utility::Printf("Loading IBL environment maps\n");

    WIN32_FIND_DATA ffd;
    WIN32_FIND_DATA ffdPNGs;
    WIN32_FIND_DATA ffdHDRs;
    HANDLE hFind = FindFirstFile(L"Textures/*_diffuseIBL.dds", &ffd);
    // [AZB]: Added step to find any PNGS aswell, and pass them on to convert func
    HANDLE hFindPNGs = FindFirstFile(L"Textures/*_diffuseIBL.png", &ffdPNGs);
    HANDLE hFindHDRs = FindFirstFile(L"Textures/*.hdr", &ffdHDRs);

    g_IBLSet.AddEnum(L"None");

    // [AZB]: Loop through PNGs and convert!
    if (hFindPNGs != INVALID_HANDLE_VALUE) do
    {
        if (ffdPNGs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;


        // [AZB]: Get filename as wstring
        std::wstring diffuseFile = ffdPNGs.cFileName;
        // [AZB]: Concatenate with path for convertion function to use
        std::wstring pathToFile = L"Textures/" + diffuseFile;

        // [AZB]: Convert to DDS
        CompileTextureOnDemand(pathToFile, 0);

        // [AZB]: Repeat for specular map, resizing original string
        diffuseFile.resize(diffuseFile.rfind(L"_diffuseIBL.png"));
        std::wstring specularFile = diffuseFile + L"_specularIBL.png";
        pathToFile = L"Textures/" + specularFile;

        CompileTextureOnDemand(pathToFile, 0);


       // [AZB]: Look for any more pairs of PNG HDRI files!
    } while (FindNextFile(hFindPNGs, &ffdPNGs) != 0);


    // [AZB]: Repeat for .HDRs!
    if (hFindHDRs != INVALID_HANDLE_VALUE) do
    {
        if (ffdHDRs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;


        // [AZB]: Get filename as wstring
        std::wstring hdrFile = ffdHDRs.cFileName;

        // [AZB]: Convert to DDS
        CompileTextureOnDemand(L"Textures/" + hdrFile, 0);

        // [AZB]: These HDR's end up as single textures, so load them into the set here
        TextureRef hdrTex = TextureManager::LoadDDSFromFile(L"Textures/" + hdrFile);
        TextureRef emptyTex = TextureManager::LoadDDSFromFile(L"empty");
        g_IBLSet.AddEnum(hdrFile);
        g_IBLTextures.push_back(std::make_pair(hdrTex, nullptr));

        // [AZB]: Look for any more pairs of PNG HDRI files!
    } while (FindNextFile(hFindHDRs, &ffdHDRs) != 0);


    // [AZB]: Once all conversions have been completed, continue with regular loading loop and allow the newly generated DDS files to be loaded!

    // [AZB]: This bit loops through all DDS files in the folder and will eventually find our PNG, so ensure it's converted before here!
    if (hFind != INVALID_HANDLE_VALUE) do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::wstring diffuseFile = ffd.cFileName;
        std::wstring baseFile = diffuseFile;
        
        baseFile.resize(baseFile.rfind(L"_diffuseIBL.dds"));
        std::wstring specularFile = baseFile + L"_specularIBL.dds";

        TextureRef diffuseTex = TextureManager::LoadDDSFromFile(L"Textures/" + diffuseFile);
        if (diffuseTex.IsValid())
        {
            TextureRef specularTex = TextureManager::LoadDDSFromFile(L"Textures/" + specularFile);
            if (specularTex.IsValid())
            {
                g_IBLSet.AddEnum(baseFile);
                g_IBLTextures.push_back(std::make_pair(diffuseTex, specularTex));
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    Utility::Printf("Found %u IBL environment map sets\n", g_IBLTextures.size());

    if (g_IBLTextures.size() > 0)
        g_IBLSet.Increment();

    // [AZB]: Set IBL glossiness bias as alot of custom models come in looking like pure Chrome due to overtuned specular maps
    Renderer::SetIBLBias(g_IBLBias);
}

#else

// [AZB]: Original Method
void LoadIBLTextures()
{
    char CWD[256];
    _getcwd(CWD, 256);

    Utility::Printf("Loading IBL environment maps\n");

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(L"Textures/*_diffuseIBL.dds", &ffd);

    g_IBLSet.AddEnum(L"None");

    if (hFind != INVALID_HANDLE_VALUE) do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

       std::wstring diffuseFile = ffd.cFileName;
       std::wstring baseFile = diffuseFile; 
       baseFile.resize(baseFile.rfind(L"_diffuseIBL.dds"));
       std::wstring specularFile = baseFile + L"_specularIBL.dds";

       TextureRef diffuseTex = TextureManager::LoadDDSFromFile(L"Textures/" + diffuseFile);
       if (diffuseTex.IsValid())
       {
           TextureRef specularTex = TextureManager::LoadDDSFromFile(L"Textures/" + specularFile);
           if (specularTex.IsValid())
           {
               g_IBLSet.AddEnum(baseFile);
               g_IBLTextures.push_back(std::make_pair(diffuseTex, specularTex));
           }
       }
    }
    while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    Utility::Printf("Found %u IBL environment map sets\n", g_IBLTextures.size());

    if (g_IBLTextures.size() > 0)
        g_IBLSet.Increment();
}
#endif

void ModelViewer::Startup( void )
{
#if AZB_MOD
    // [AZB] Disable these for maximum image clarity!
    MotionBlur::Enable = false;
    TemporalEffects::EnableTAA = false;
    // [AZB]: Disable this so that we can get real frame-times!
    //s_EnableVSync = false;
#else
    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = true;
#endif
    FXAA::Enable = false;
    PostEffects::EnableHDR = true;
    PostEffects::EnableAdaptation = true;
    SSAO::Enable = true;

    Renderer::Initialize();

    LoadIBLTextures();

    std::wstring gltfFileName;

    bool forceRebuild = false;
    uint32_t rebuildValue;
    if (CommandLineArgs::GetInteger(L"rebuild", rebuildValue))
        forceRebuild = rebuildValue != 0;

    if (CommandLineArgs::GetString(L"model", gltfFileName) == false)
    {
#ifdef LEGACY_RENDERER
        Sponza::Startup(m_Camera);
#else
        m_ModelInst = Renderer::LoadModel(L"Sponza/PBR/sponza2.gltf", forceRebuild);
        m_ModelInst.Resize(100.0f * m_ModelInst.GetRadius());
        OrientedBox obb = m_ModelInst.GetBoundingBox();
        float modelRadius = Length(obb.GetDimensions()) * 0.5f;
        const Vector3 eye = obb.GetCenter() + Vector3(modelRadius * 0.5f, 0.0f, 0.0f);
        m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
#endif
    }
    else
    {
        //[AZB]: This block will load our Bistro scene!... but only one of them!
        m_ModelInst = Renderer::LoadModel(gltfFileName, forceRebuild);
        m_ModelInst.LoopAllAnimations();
        //m_ModelInst.Resize(10.0f);
        m_ModelInst.Resize(100.0f * m_ModelInst.GetRadius());
        OrientedBox obb = m_ModelInst.GetBoundingBox();
        float modelRadius = Length(obb.GetDimensions()) * 0.5f;
        const Vector3 eye = obb.GetCenter() + Vector3(modelRadius * 0.5f, 0.0f, modelRadius * 0.5f);
        m_Camera.SetEyeAtUp(eye, Vector3(kZero), Vector3(kYUnitVector));
        float newFov = m_Camera.GetFOV() * 2.5f;
        m_Camera.SetFOV(newFov);

        MotionBlur::Enable = false;
    }

#if AZB_MOD
    m_Camera.SetZRange(0.01f, 40000.0f);
    
    // [AZB]: FPS camera has weeeird culling!
    m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));
    //m_CameraController.reset(new OrbitCamera(m_Camera, m_ModelInst.GetBoundingSphere(), Vector3(kYUnitVector)));

#else
    m_Camera.SetZRange(1.0f, 10000.0f);
    if (gltfFileName.size() == 0)
        m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));
    else
        m_CameraController.reset(new OrbitCamera(m_Camera, m_ModelInst.GetBoundingSphere(), Vector3(kYUnitVector)));
#endif
}

void ModelViewer::Cleanup( void )
{
    m_ModelInst = nullptr;

    g_IBLTextures.clear();

#ifdef LEGACY_RENDERER
    Sponza::Cleanup();
#endif

    Renderer::Shutdown();
}

namespace Graphics
{
    extern EnumVar DebugZoom;
}

void ModelViewer::Update( float deltaT )
{
    ScopedTimer _prof(L"Update State");

    if (GameInput::IsFirstPressed(GameInput::kLShoulder))
        DebugZoom.Decrement();
    else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
        DebugZoom.Increment();

    m_CameraController->Update(deltaT);

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");

    m_ModelInst.Update(gfxContext, deltaT);

    gfxContext.Finish();

    // We use viewport offsets to jitter sample positions from frame to frame (for TAA.)
    // D3D has a design quirk with fractional offsets such that the implicit scissor
    // region of a viewport is floor(TopLeftXY) and floor(TopLeftXY + WidthHeight), so
    // having a negative fractional top left, e.g. (-0.25, -0.25) would also shift the
    // BottomRight corner up by a whole integer.  One solution is to pad your viewport
    // dimensions with an extra pixel.  My solution is to only use positive fractional offsets,
    // but that means that the average sample position is +0.5, which I use when I disable
    // temporal AA.
    TemporalEffects::GetJitterOffset(m_MainViewport.TopLeftX, m_MainViewport.TopLeftY);

    m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
    m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
    m_MainViewport.MinDepth = 0.0f;
    m_MainViewport.MaxDepth = 1.0f;

    m_MainScissor.left = 0;
    m_MainScissor.top = 0;
    m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
    m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();
}

void ModelViewer::RenderScene( void )
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();
    const D3D12_VIEWPORT& viewport = m_MainViewport;
    const D3D12_RECT& scissor = m_MainScissor;

    ParticleEffectManager::Update(gfxContext.GetComputeContext(), Graphics::GetFrameTime());

    if (m_ModelInst.IsNull())
    {
#ifdef LEGACY_RENDERER
        Sponza::RenderScene(gfxContext, m_Camera, viewport, scissor);
#endif
    }
    else
    {
        // Update global constants
        float costheta = cosf(g_SunOrientation);
        float sintheta = sinf(g_SunOrientation);
        float cosphi = cosf(g_SunInclination * 3.14159f * 0.5f);
        float sinphi = sinf(g_SunInclination * 3.14159f * 0.5f);

        Vector3 SunDirection = Normalize(Vector3( costheta * cosphi, sinphi, sintheta * cosphi ));
        Vector3 ShadowBounds = Vector3(m_ModelInst.GetRadius());
        //m_SunShadowCamera.UpdateMatrix(-SunDirection, m_ModelInst.GetCenter(), ShadowBounds,
        m_SunShadowCamera.UpdateMatrix(-SunDirection, Vector3(0, -500.0f, 0), Vector3(5000, 3000, 3000),
            (uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16);

        GlobalConstants globals;
        globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
        globals.SunShadowMatrix = m_SunShadowCamera.GetShadowMatrix();
        globals.CameraPos = m_Camera.GetPosition();
        globals.SunDirection = SunDirection;
        globals.SunIntensity = Vector3(Scalar(g_SunLightIntensity));

        // Begin rendering depth
        gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        gfxContext.ClearDepth(g_SceneDepthBuffer);

        MeshSorter sorter(MeshSorter::kDefault);
		sorter.SetCamera(m_Camera);
		sorter.SetViewport(viewport);
		sorter.SetScissor(scissor);
		sorter.SetDepthStencilTarget(g_SceneDepthBuffer);
		sorter.AddRenderTarget(g_SceneColorBuffer);

        m_ModelInst.Render(sorter);

        sorter.Sort();

        {
            ScopedTimer _prof(L"Depth Pre-Pass", gfxContext);
            sorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
        }

        SSAO::Render(gfxContext, m_Camera);

        if (!SSAO::DebugDraw)
        {
            ScopedTimer _outerprof(L"Main Render", gfxContext);

            {
                ScopedTimer _prof(L"Sun Shadow Map", gfxContext);

                MeshSorter shadowSorter(MeshSorter::kShadows);
				shadowSorter.SetCamera(m_SunShadowCamera);
				shadowSorter.SetDepthStencilTarget(g_ShadowBuffer);

                m_ModelInst.Render(shadowSorter);

                shadowSorter.Sort();
                shadowSorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
            }

            gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
            gfxContext.ClearColor(g_SceneColorBuffer);

            {
                ScopedTimer _prof(L"Render Color", gfxContext);

                gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
                gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV_DepthReadOnly());
                gfxContext.SetViewportAndScissor(viewport, scissor);

                sorter.RenderMeshes(MeshSorter::kOpaque, gfxContext, globals);
            }

            Renderer::DrawSkybox(gfxContext, m_Camera, viewport, scissor);

            sorter.RenderMeshes(MeshSorter::kTransparent, gfxContext, globals);
        }
    }

    // Some systems generate a per-pixel velocity buffer to better track dynamic and skinned meshes.  Everything
    // is static in our scene, so we generate velocity from camera motion and the depth buffer.  A velocity buffer
    // is necessary for all temporal effects (and motion blur).
    MotionBlur::GenerateCameraVelocityBuffer(gfxContext, m_Camera, true);

#if AZB_MOD
    // [AZB]: TMP: Particle rendering assumes that the depth buffer and output buffer are the same size. Experimenting with moving it to before DLSS so that the particles can get upscaled too!
    ParticleEffectManager::Render(gfxContext, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer,  g_LinearDepth[FrameIndex]);
#endif
    // [AZB]: This is where DLSS gets executed!
    TemporalEffects::ResolveImage(gfxContext);

#if AZB_MOD
    // [AZB]: The main sceneColorBuffer is effectively disposed of once DLSS executes, so run post effects on that buffer instead!
   // if (DLSS::m_DLSS_Enabled)
   //     //ParticleEffectManager::Render(gfxContext, m_Camera, g_DLSSOutputBuffer, g_SceneDepthBuffer, g_LinearDepth[FrameIndex]);
   // else
        // AZB]: DLSS can still be disabled, so default to original method when this happens
       // ParticleEffectManager::Render(gfxContext, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer,  g_LinearDepth[FrameIndex]);

#else
    ParticleEffectManager::Render(gfxContext, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer,  g_LinearDepth[FrameIndex]);
#endif
    // Until I work out how to couple these two, it's "either-or".
    if (DepthOfField::Enable)
        DepthOfField::Render(gfxContext, m_Camera.GetNearClip(), m_Camera.GetFarClip());
    else
        MotionBlur::RenderObjectBlur(gfxContext, g_VelocityBuffer);

    gfxContext.Finish();
}
