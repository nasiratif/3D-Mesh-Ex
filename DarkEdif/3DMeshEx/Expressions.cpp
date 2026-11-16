#include "Common.hpp"

int Extension::GetWidth()
{
	return renderCtx.GetSurfaceWidth();
}

int Extension::GetHeight()
{
	return renderCtx.GetSurfaceHeight();
}


float Extension::GetYaw()
{
	return renderCtx.meshProps.transform.yaw;
}

float Extension::GetPitch()
{
	return renderCtx.meshProps.transform.pitch;
}

float Extension::GetRoll()
{
	return renderCtx.meshProps.transform.roll;
}


float Extension::GetScaleX()
{
	return renderCtx.meshProps.transform.scaleX;
}

float Extension::GetScaleY()
{
	return renderCtx.meshProps.transform.scaleY;
}

float Extension::GetScaleZ()
{
	return renderCtx.meshProps.transform.scaleZ;
}


float Extension::GetSubsetPosX(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->x;
}

float Extension::GetSubsetPosY(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->y;
}

float Extension::GetSubsetPosZ(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->z;
}


float Extension::GetSubsetYaw(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->yaw;
}

float Extension::GetSubsetPitch(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->pitch;
}

float Extension::GetSubsetRoll(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->roll;
}


float Extension::GetSubsetScaleX(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->scaleX;
}

float Extension::GetSubsetScaleY(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->scaleY;
}

float Extension::GetSubsetScaleZ(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return renderCtx.GetLocalSubsetTransform(pSubset)->scaleZ;
}


int Extension::GetNumSubsets()
{
	return renderCtx.GetNumSubsets();
}

int Extension::GetSubsetIndex(const wchar* name)
{
	auto pSubset = renderCtx.GetSubset(name);
	if (!pSubset)
		return -1;

	return pSubset->index;
}

const TCHAR* Extension::GetSubsetName(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return _T("");

	return Runtime.CopyString(pSubset->name);
}


int Extension::GetCurrentAnimIndex()
{
	return renderCtx.GetCurrentAnim();
}

float Extension::GetAnimSpeed()
{
	return (float)renderCtx.animProps.animSpeed;
}

float Extension::GetCurrentAnimFrame()
{
	return (float)renderCtx.GetCurrentAnimFrame();
}


int Extension::GetNumAnims()
{
	return renderCtx.GetNumAnims();
}

int Extension::GetAnimIndex(const TCHAR* name)
{
	auto pAnim = renderCtx.GetAnim(name);
	if (!pAnim)
		return -1;

	return pAnim->index;
}

const TCHAR* Extension::GetAnimName(int anim)
{
	auto pAnim = renderCtx.GetAnim(anim);
	if (!pAnim)
		return _T("");

	return Runtime.CopyString(pAnim->name);
}

float Extension::GetAnimFPS()
{
	return (float)renderCtx.GetAnimFPS();
}


int Extension::GetNumBones()
{
	return renderCtx.GetNumBones();
}

int Extension::GetBoneIndex(const TCHAR* name)
{
	auto pBone = renderCtx.GetBone(name);
	if (!pBone)
		return -1;

	return pBone->index;
}

const TCHAR* Extension::GetBoneName(int bone)
{
	auto pBone = renderCtx.GetBone(bone);
	if (!pBone)
		return _T("");

	return Runtime.CopyString(pBone->name);
}

int Extension::GetProjectedBoneX(int bone, int subset)
{
	int32 x, y;
	renderCtx.GetProjectedBonePos(bone, subset, x, y);
	return x + rdPtr->get_rHo()->get_X();
}

int Extension::GetProjectedBoneY(int bone, int subset)
{
	int32 x, y;
	renderCtx.GetProjectedBonePos(bone, subset, x, y);
	return y + rdPtr->get_rHo()->get_Y();
}


int Extension::GetAmbientColor()
{
	return (int)renderCtx.lightingProps.ambient;
}

int Extension::GetDiffuseColor()
{
	return (int)renderCtx.lightingProps.diffuse;
}

int Extension::GetFogColor()
{
	return (int)renderCtx.lightingProps.fogColor;
}


float Extension::GetLightDirX()
{
	return renderCtx.lightingProps.dirX;
}

float Extension::GetLightDirY()
{
	return renderCtx.lightingProps.dirY;
}

float Extension::GetLightDirZ()
{
	return renderCtx.lightingProps.dirZ;
}


float Extension::GetFogDistance()
{
	return renderCtx.lightingProps.fogDist;
}


float Extension::GetFOV()
{
	return renderCtx.cameraProps.fov;
}

float Extension::GetNearClip()
{
	return renderCtx.cameraProps.nearClip;
}

float Extension::GetFarClip()
{
	return renderCtx.cameraProps.farClip;
}


float Extension::GetCameraPosX()
{
	return renderCtx.cameraProps.x;
}

float Extension::GetCameraPosY()
{
	return renderCtx.cameraProps.y;
}

float Extension::GetCameraPosZ()
{
	return renderCtx.cameraProps.z;
}


float Extension::GetCameraYaw()
{
	return renderCtx.cameraProps.yaw;
}

float Extension::GetCameraPitch()
{
	return renderCtx.cameraProps.pitch;
}

float Extension::GetCameraRoll()
{
	return renderCtx.cameraProps.roll;
}


float Extension::GetAnimInterpSpeed()
{
	return (float)renderCtx.animProps.animInterpSpeed;
}


float Extension::GetSubsetAABBRadius(int subset)
{
	auto pSubset = renderCtx.GetSubset(subset);
	if (!pSubset)
		return 0.0f;

	return *renderCtx.GetLocalSubsetAABB(pSubset);
}


const TCHAR* Extension::GetMeshFileName()
{
	auto pMesh = renderCtx.GetMesh();
	return pMesh ? Runtime.CopyString(pMesh->fileName) : _T("");
}
