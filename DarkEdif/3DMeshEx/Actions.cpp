#include "Common.hpp"

void Extension::SetObjectSize(int width, int height)
{
	if (width == renderCtx.GetSurfaceWidth() && height == renderCtx.GetSurfaceHeight())
		return;

	renderCtx.CreateSurface(width, height);
	rdPtr->get_rHo()->SetImgWidth(renderCtx.GetSurfaceWidth());
	rdPtr->get_rHo()->SetImgHeight(renderCtx.GetSurfaceHeight());
}


void Extension::LoadMesh(const TCHAR* fileName)
{
	renderCtx.SetMesh(renderCtx.GetGlobalContext()->LoadMesh(fileName));
}

void Extension::ReplaceTexture(int textureID, const TCHAR* fileName)
{
	renderCtx.ReplaceTexture(rdPtr->get_rHo()->get_AdRunHeader(), textureID, fileName);
}

void Extension::SetYaw(float yaw)
{
	renderCtx.meshProps.transform.yaw = yaw;
}

void Extension::SetPitch(float pitch)
{
	renderCtx.meshProps.transform.pitch = pitch;
}

void Extension::SetRoll(float roll)
{
	renderCtx.meshProps.transform.roll = roll;
}


void Extension::SetScaleX(float x)
{
	renderCtx.meshProps.transform.scaleX = x;
}

void Extension::SetScaleY(float y)
{
	renderCtx.meshProps.transform.scaleY = y;
}

void Extension::SetScaleZ(float z)
{
	renderCtx.meshProps.transform.scaleZ = z;
}


void Extension::SetTexFilter(int filter)
{
	if (filter < 0 || filter >= MeshEx::MeshFiltering_Max)
		return;

	renderCtx.meshProps.filtering = (MeshEx::MeshFiltering)filter;
}

void Extension::SetUVAddress(int mode)
{
	if (mode < 0 || mode >= MeshEx::MeshAddressing_Max)
		return;

	renderCtx.meshProps.addressMode = (MeshEx::MeshAddressing)mode;
}


void Extension::SetWireframe(int wireframe)
{
	renderCtx.meshProps.wireframe = wireframe != 0;
}

void Extension::SetCullMode(int mode)
{
	if (mode < 0 || mode >= MeshEx::MeshCull_Max)
		return;

	renderCtx.meshProps.cull = (MeshEx::MeshCull)mode;
}


void Extension::SetSubsetPosX(int subset, float x)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->x = x;
}

void Extension::SetSubsetPosY(int subset, float y)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->y = y;
}

void Extension::SetSubsetPosZ(int subset, float z)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->z = z;
}


void Extension::SetSubsetYaw(int subset, float yaw)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->yaw = yaw;
}

void Extension::SetSubsetPitch(int subset, float pitch)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->pitch = pitch;
}

void Extension::SetSubsetRoll(int subset, float roll)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->roll = roll;
}


void Extension::SetSubsetScaleX(int subset, float x)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->scaleX = x;
}

void Extension::SetSubsetScaleY(int subset, float y)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->scaleY = y;
}

void Extension::SetSubsetScaleZ(int subset, float z)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	renderCtx.GetLocalSubsetTransform(pSubset)->scaleZ = z;
}



void Extension::PlayAnim()
{
	renderCtx.PlayAnim();
}

void Extension::StopAnim()
{
	renderCtx.StopAnim();
}


void Extension::SetLoop(int loop)
{
	renderCtx.animProps.loop = loop != 0;
}

void Extension::SetFrameInterp(int mode)
{
	if (mode < 0 || mode >= MeshEx::FrameInterpolation_Max)
		return;

	renderCtx.animProps.frameInterp = (MeshEx::FrameInterpolation)mode;
}


void Extension::SetAnim(int anim)
{
	renderCtx.SetCurrentAnim(anim);
}

void Extension::SetAnimSpeed(float speed)
{
	renderCtx.animProps.animSpeed = speed;
}

void Extension::SetAnimFrame(float frame)
{
	renderCtx.SetCurrentAnimFrame(frame);
}


void Extension::SetAmbientColor(unsigned int color)
{
	renderCtx.lightingProps.ambient = color;
}

void Extension::SetDiffuseColor(unsigned int color)
{
	renderCtx.lightingProps.diffuse = color;
}

void Extension::SetFogColor(unsigned int color)
{
	renderCtx.lightingProps.fogColor = color;
}


void Extension::SetLightDirX(float x)
{
	renderCtx.lightingProps.dirX = x;
}

void Extension::SetLightDirY(float y)
{
	renderCtx.lightingProps.dirY = y;
}

void Extension::SetLightDirZ(float z)
{
	renderCtx.lightingProps.dirZ = z;
}


void Extension::SetFogDistance(float dist)
{
	renderCtx.lightingProps.fogDist = dist;
}



void Extension::SetProjection(int projection)
{
	if (projection < 0 || projection >= MeshEx::CameraProjection_Max)
		return;

	renderCtx.cameraProps.proj = (MeshEx::CameraProjection)projection;
}


void Extension::SetFOV(float fov)
{
	renderCtx.cameraProps.fov = fov;
}

void Extension::SetNearClip(float nearClip)
{
	renderCtx.cameraProps.nearClip = nearClip;
}

void Extension::SetFarClip(float farClip)
{
	renderCtx.cameraProps.farClip = farClip;
}



void Extension::SetCameraPosX(float x)
{
	renderCtx.cameraProps.x = x;
}

void Extension::SetCameraPosY(float y)
{
	renderCtx.cameraProps.y = y;
}

void Extension::SetCameraPosZ(float z)
{
	renderCtx.cameraProps.z = z;
}


void Extension::SetCameraYaw(float yaw)
{
	renderCtx.cameraProps.yaw = yaw;
}

void Extension::SetCameraPitch(float pitch)
{
	renderCtx.cameraProps.pitch = pitch;
}

void Extension::SetCameraRoll(float roll)
{
	renderCtx.cameraProps.roll = roll;
}


void Extension::SetAnimInterpSpeed(float speed)
{
	renderCtx.animProps.animInterpSpeed = std::max<float64>(speed, 0.0);
}


void Extension::SetSubsetAABBRadius(int subset, float radius)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return;

	*renderCtx.GetLocalSubsetAABB(pSubset) = radius;
}
