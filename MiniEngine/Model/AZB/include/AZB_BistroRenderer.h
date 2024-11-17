#pragma once
//===============================================================================
// desc: This is modelled after the SponzaRenderer namespace, allowing for Bistro to be rendered in a non-HDR environment using baked lights!
// auth: Aliyaan Zulfiqar
//===============================================================================
#include <d3d12.h>
#include "AZB_Utils.h"
//===============================================================================

class GraphicsContext;
class ShadowCamera;
class ModelInstance;
class ExpVar;

namespace Math
{
    class Camera;
    class Vector3;
}

namespace Bistro
{
    void Startup(Math::Camera& camera, ModelInstance& model);
    void Cleanup(void);

    void RenderScene(
        GraphicsContext& gfxContext,
        const Math::Camera& camera,
        const D3D12_VIEWPORT& viewport,
        const D3D12_RECT& scissor,
        bool skipDiffusePass = false,
        bool skipShadowMap = false);

    const ModelInstance& GetModel();

    extern Math::Vector3 m_SunDirection;
    extern ShadowCamera m_SunShadow;
    extern ExpVar m_AmbientIntensity;
    extern ExpVar m_SunLightIntensity;

}
