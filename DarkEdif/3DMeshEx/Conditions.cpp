#include "Common.hpp"

bool Extension::ImmediateCondition()
{
	return true;
}


bool Extension::IsMeshLoaded()
{
	return renderCtx.IsMeshLoaded();
}


bool Extension::UsingPointFiltering()
{
	return renderCtx.meshProps.filtering == MeshEx::MeshFiltering_Point;
}

bool Extension::UsingBilinearFiltering()
{
	return renderCtx.meshProps.filtering == MeshEx::MeshFiltering_Bilinear;
}


bool Extension::UsingWrapUV()
{
	return renderCtx.meshProps.addressMode == MeshEx::MeshAddressing_Wrap;
}

bool Extension::UsingClampUV()
{
	return renderCtx.meshProps.addressMode == MeshEx::MeshAddressing_Clamp;
}


bool Extension::IsWireframe()
{
	return renderCtx.meshProps.wireframe;
}


bool Extension::UsingBackFace()
{
	return renderCtx.meshProps.cull == MeshEx::MeshCull_Back;
}

bool Extension::UsingFrontFace()
{
	return renderCtx.meshProps.cull == MeshEx::MeshCull_Front;
}

bool Extension::UsingNoneFace()
{
	return renderCtx.meshProps.cull == MeshEx::MeshCull_None;
}


bool Extension::IsAnimLooping()
{
	return renderCtx.animProps.loop;
}

bool Extension::UsingAlwaysInterp()
{
	return renderCtx.animProps.frameInterp == MeshEx::FrameInterpolation_Always;
}

bool Extension::UsingSometimesInterp()
{
	return renderCtx.animProps.frameInterp == MeshEx::FrameInterpolation_Sometimes;
}

bool Extension::UsingNeverInterp()
{
	return renderCtx.animProps.frameInterp == MeshEx::FrameInterpolation_Never;
}


bool Extension::UsingPerspective()
{
	return renderCtx.cameraProps.proj == MeshEx::CameraProjection_Perspective;
}
