#pragma once
//===============================================================================
// desc: A helper namespace to generate per-pixel motion vectors! The existing implementation in MiniEngine only generates camera velocity
// auth: Aliyaan Zulfiqar
//===============================================================================

class ComputeContext;

namespace MotionVectors
{
	// These functions mimick how other MiniEngine namespaces initialise and terminate, so I am replicating these for code parity sake.
	void Initialize(void);
	void Shutdown(void);

	void GeneratePerPixelMotionVectors(ComputeContext& Context);

}