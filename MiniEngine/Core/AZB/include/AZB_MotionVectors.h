#pragma once
//===============================================================================
// desc: A helper namespace to generate per-pixel motion vectors! The existing implementation in MiniEngine only generates camera velocity
// auth: Aliyaan Zulfiqar
//===============================================================================
#include "EngineTuning.h"

class CommandContext;
namespace Math { class Camera; }

namespace MotionVectors
{
	// These functions mimick how other MiniEngine namespaces initialise and terminate, so I am replicating these for code parity sake.
	void Initialize(void);
	void Shutdown(void);

	// This function takes the screen-space motion vectors created by MiniEngine and decodes them into a format we can use!
	void DecodeMotionVectors(CommandContext& BaseContext);

	// This function actually generates world space, per pixel motion vectors!
	void GeneratePerPixelMotionVectors(CommandContext& BaseContext, const Math::Camera& camera);

	// This function will help visualise motion vectors
	void Render(CommandContext& BaseContext);
}