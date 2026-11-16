#include "Common.hpp"
#include "MeshEx.hpp"

#include <ufbx.h>

namespace MeshEx
{
	Matrix RenderContext::identityMatrix;
}

using namespace MeshEx;

/*

---------- UTILITY FUNCTIONS ----------

*/

size_t FileReadUFBX(void* user, void* data, size_t size)
{
	return size <= UINT32_MAX ? FileRead((File)user, data, (uint32)size) : SIZE_MAX;
}

bool FileSeekUFBX(void* user, size_t size)
{
	if (size > UINT32_MAX)
		return false;

	FileSeek((File)user, (uint32)size, FileDirection_Current);
	return true;
}

uint64_t FileSizeUFBX(void* user) { return FileSize((File)user); }
void FileCloseUFBX(void* user) { FileClose((File)user); }

inline void CopyUFBXName(wchar* dest, const ufbx_string& src)
{
	memset(dest, L'\0', (MEX_MAX_NAME_CHARS + 1) * sizeof(*dest));
	for (uint32 i = 0; i < src.length; ++i)
	{
		if (i >= MEX_MAX_NAME_CHARS)
			break;
		dest[i] = (wchar)src.data[i];
	}
}

inline void CopyUFBXTransform(Transform& dest, const ufbx_transform& src)
{
	auto euler = ufbx_quat_to_euler(src.rotation, UFBX_ROTATION_ORDER_ZXY);
	dest.x = src.translation.x;
	dest.y = src.translation.z;
	dest.z = src.translation.y;
	dest.yaw = euler.z;
	dest.pitch = -euler.x;
	dest.roll = -euler.y;
	dest.scaleX = src.scale.x;
	dest.scaleY = src.scale.z;
	dest.scaleZ = src.scale.y;
}

inline void CopyUFBXTransform(TransformQuat& dest, const ufbx_transform& src)
{
	dest.x = src.translation.x;
	dest.y = src.translation.z;
	dest.z = src.translation.y;
	dest.quatW = src.rotation.w;
	dest.quatX = -src.rotation.x;
	dest.quatY = -src.rotation.z;
	dest.quatZ = -src.rotation.y;
	dest.scaleX = src.scale.x;
	dest.scaleY = src.scale.z;
	dest.scaleZ = src.scale.y;
}

// Handle FBX's weird 100x scaling that all FBXs I've tested do for some reason
// There probably exists a better way to handle this, but not that I'm aware
// Using a template so it works on both Transform & TransformQuat
template<typename T>
inline void ScaleUFBXTransform(T& dest, ufbx_real units)
{
	if (!units)
		return;

	dest.x *= units;
	dest.y *= units;
	dest.z *= units;

	dest.scaleX *= units;
	dest.scaleY *= units;
	dest.scaleZ *= units;
}

/*

---------- GLOBAL CONTEXT ----------

*/

GlobalContext::~GlobalContext()
{
	while (!meshes.empty())
		ReleaseMesh(meshes.size() - 1);

	ReleaseDriverResources();
}


uint32 GlobalContext::LoadMesh(const wchar* fileName)
{
	for (auto& mesh : meshes)
		if (wcscmp(mesh.fileName, fileName) == 0)
			return (uint32)mesh.index; // already exists!

	auto ret = MEX_UNDEFINED;
	auto meshID = MEX_UNDEFINED;

	Mesh* mesh;
	// Would rather prefer to use Fusion's file opening functions, hence why I'm using a custom stream rather than ufbx_load_file
	ufbx_stream fileStream;
	ufbx_scene* scene = nullptr;
	ufbx_load_opts opts = {};

	auto file = MEX_FILE_INVALID;
	if (!FileOpen(fileName, file))
		goto finished;

	fileStream.user = file;
	fileStream.read_fn = FileReadUFBX;
	fileStream.skip_fn = FileSeekUFBX;
	fileStream.size_fn = FileSizeUFBX;
	fileStream.close_fn = FileCloseUFBX;

	// Adjust ufbx load options to respect the renderer's coordinate convention a little more:
	// -----
	opts.handedness_conversion_axis = UFBX_MIRROR_AXIS_Z;
	opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_axes.up = UFBX_COORDINATE_AXIS_NEGATIVE_Y;
	opts.target_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
	opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	// -----

	scene = ufbx_load_stream(&fileStream, &opts, nullptr);
	if (!scene)
		goto finished;

	meshID = (uint32)meshes.size();
	mesh = &meshes.emplace_back();
	mesh->index = (int32)meshID;
	mesh->globalCtx = this;
	wcsncpy(mesh->fileName, fileName, MEX_MAX_FILENAME_CHARS);
	mesh->fileName[MEX_MAX_FILENAME_CHARS] = '\0'; // wcsncpy does not null-terminate if the string is larger than the buffer :(

	// Parse bones:
	mesh->bones.reserve(scene->bones.count);
	for (const auto sceneBone : scene->bones)
	{
		if (mesh->bones.size() >= MEX_MAX_BONES)
			break;
		if (!sceneBone->instances.count)
			continue;

		auto boneNode = sceneBone->instances[0];
		auto& bone = mesh->bones.emplace_back();
		bone.index = (int32)mesh->bones.size() - 1;
		CopyUFBXName(bone.name, sceneBone->name);
		CopyUFBXTransform(bone.transform, boneNode->local_transform);
		bone.interpTransform = bone.prevInterpTransform = bone.finalTransform = bone.transform;

		auto parent = boneNode->parent;
		if (parent && parent->bone) // is this a child bone?
		{
			auto parentBoneIndex = parent->bone->typed_id;
			auto childBoneIndex = sceneBone->typed_id;
			auto& parentBone = mesh->bones[parentBoneIndex];
			if (parentBone.numChildren < MEX_MAX_BONES)
				parentBone.children[parentBone.numChildren++] = childBoneIndex;

			bone.spaceTransformIndex = parentBone.spaceTransformIndex;
		}
		else // it's a top-most bone
		{
			mesh->rootBones.emplace_back(sceneBone->typed_id);
			// Find the space transform for this bone
			// Needed mainly because of subsets with bone relations (is there a better way to do this?):
			if (boneNode->parent && boneNode->parent->attrib_type == UFBX_ELEMENT_EMPTY)
			{
				TransformQuat transform;
				CopyUFBXTransform(transform, boneNode->parent->local_transform);
				ScaleUFBXTransform(transform, scene->settings.unit_meters);

				uint32 spaceTransform = MEX_UNDEFINED;
				for (std::size_t i = 0; i < mesh->spaceTransforms.size(); ++i)
				{
					if (transform == mesh->spaceTransforms[i])
					{
						spaceTransform = (uint32)i;
						break;
					}
				}
				if (spaceTransform == MEX_UNDEFINED)
				{
					bone.spaceTransformIndex = (uint32)mesh->spaceTransforms.size();
					mesh->spaceTransforms.emplace_back(transform);
				}
				else
				{
					bone.spaceTransformIndex = spaceTransform;
				}
			}
			else
			{
				bone.spaceTransformIndex = MEX_UNDEFINED;
			}
		}
	}

	// Parse meshes:
	mesh->subsets.reserve(scene->meshes.count);
	for (const auto sceneMesh : scene->meshes)
	{
		if (!sceneMesh->instances.count || !sceneMesh->num_faces)
			continue;

		auto meshNode = sceneMesh->instances[0];

		auto& subset = mesh->subsets.emplace_back();
		subset.index = (int32)mesh->subsets.size() - 1;
		CopyUFBXName(subset.name, sceneMesh->name);
		CopyUFBXTransform(subset.transform, meshNode->local_transform);
		
		// Initialize subset min/max AABB:
		// -----
		subset.minMaxX[0] = FLT_MAX; subset.minMaxX[1] = FLT_MIN;
		subset.minMaxY[0] = FLT_MAX; subset.minMaxY[1] = FLT_MIN;
		subset.minMaxZ[0] = FLT_MAX; subset.minMaxZ[1] = FLT_MIN;
		// -----

		if (!meshNode->parent || meshNode->parent->attrib_type == UFBX_ELEMENT_UNKNOWN)
			ScaleUFBXTransform(subset.transform, scene->settings.unit_meters);

		subset.parentBone = meshNode->parent && meshNode->parent->bone ? meshNode->parent->bone->typed_id : UINT32_MAX;

		struct VertexBone
		{
			float32 boneIDs[MEX_MAX_BONE_INFLUENCE];
			float32 boneWeights[MEX_MAX_BONE_INFLUENCE];
		};
		auto vertBones = std::make_unique<VertexBone[]>(sceneMesh->num_vertices);

		// Extract vertices
		for (uint32 i = 0; i < sceneMesh->num_vertices; ++i)
		{
			auto& vertBone = vertBones[i];
			for (uint32 k = 0; k < MEX_MAX_BONE_INFLUENCE; ++k)
			{
				vertBone.boneIDs[k] = MEX_UNDEFINED_BONE;
				vertBone.boneWeights[k] = 0.0f;
			}
		}

		// Parse bone clusters (if there are any):
		subset.animated = sceneMesh->skin_deformers.count > 0;
		auto skin = subset.animated ? sceneMesh->skin_deformers[0] : nullptr;
		if (skin)
		{
			for (const auto cluster : skin->clusters)
			{
				auto boneID = cluster->bone_node->bone->typed_id;
				if (boneID < 0 || boneID >= mesh->bones.size())
					continue;

				// Construct bone space conversion matrix:
				// (note that the conventions of the source & destination matrices differ)
				// -----
				auto& mat = subset.meshToBone[boneID];
				mat.rows[0][0] = cluster->geometry_to_bone.cols[0].x;
				mat.rows[0][1] = cluster->geometry_to_bone.cols[2].x;
				mat.rows[0][2] = cluster->geometry_to_bone.cols[1].x;
				mat.rows[0][3] = cluster->geometry_to_bone.cols[3].x;

				mat.rows[1][0] = cluster->geometry_to_bone.cols[0].z;
				mat.rows[1][1] = cluster->geometry_to_bone.cols[2].z;
				mat.rows[1][2] = cluster->geometry_to_bone.cols[1].z;
				mat.rows[1][3] = cluster->geometry_to_bone.cols[3].z;

				mat.rows[2][0] = cluster->geometry_to_bone.cols[0].y;
				mat.rows[2][1] = cluster->geometry_to_bone.cols[2].y;
				mat.rows[2][2] = cluster->geometry_to_bone.cols[1].y;
				mat.rows[2][3] = cluster->geometry_to_bone.cols[3].y;

				mat.rows[3][0] = mat.rows[3][1] = mat.rows[3][2] = 0.0f;
				mat.rows[3][3] = 1.0f;
				// -----

				for (uint32 i = 0; i < cluster->num_weights; ++i)
				{
					const auto weight = cluster->weights[i];
					auto& vert = vertBones[cluster->vertices[i]];
					auto foundBone = false;
					for (uint32 k = 0; k < MEX_MAX_BONE_INFLUENCE; ++k)
					{
						if (vert.boneIDs[k] < 0.0f)
						{
							foundBone = true;
							vert.boneIDs[k] = (float)boneID;
							vert.boneWeights[k] = weight;
							break;
						}
					}
				}
			}
			// Normalize weights:
			// Only necessary for vertices that had more than MEX_MAX_BONE_INFLUENCE bones influencing it, or the weights just weren't already normalized
			for (uint32 i = 0; i < sceneMesh->num_vertices; ++i)
			{
				auto& vertBone = vertBones[i];

				float32 sum = 0.0f;
				for (uint32 i = 0; i < MEX_MAX_BONE_INFLUENCE; ++i)
					sum += vertBone.boneWeights[i];

				for (uint32 i = 0; i < MEX_MAX_BONE_INFLUENCE; ++i)
					vertBone.boneWeights[i] /= sum;
			}
		}

		auto maxTriIndices = sceneMesh->max_face_triangles * 3;
		auto faceIndices = std::make_unique<uint32[]>(maxTriIndices);

		auto currentTexID = sceneMesh->face_material.count && sceneMesh->materials.count ? sceneMesh->materials[sceneMesh->face_material[0]]->typed_id : 0;
		auto currentPrimitive = &subset.primitives.emplace_back();
		currentPrimitive->textureID = currentTexID;
		// Extract face data:
		for (uint32 i = 0; i < sceneMesh->faces.count; ++i)
		{
			const auto face = sceneMesh->faces[i];
			if (sceneMesh->face_material.count && sceneMesh->materials.count)
			{
				auto texID = sceneMesh->materials[sceneMesh->face_material[i]]->typed_id;
				if (texID != currentTexID)
				{
					currentPrimitive = &subset.primitives.emplace_back();
					currentPrimitive->textureID = currentTexID = texID;
				}
			}

			subset.vertices.resize(subset.vertices.size() + face.num_indices);
			auto numVerts = ufbx_triangulate_face(faceIndices.get(), maxTriIndices, sceneMesh, face) * 3;
			for (uint32 i = 0; i < numVerts; ++i)
			{
				auto faceIndex = faceIndices[i];
				auto vertBoneIndex = sceneMesh->vertex_indices[faceIndex];
				currentPrimitive->indices.emplace_back(faceIndex);

				auto& vert = subset.vertices[faceIndex];
				if (sceneMesh->vertex_position.exists)
				{
					vert.x = sceneMesh->vertex_position[faceIndex].x;
					vert.y = sceneMesh->vertex_position[faceIndex].z;
					vert.z = sceneMesh->vertex_position[faceIndex].y;
				}
				else
				{
					vert.x = vert.y = vert.z = 0.0f;
				}

				if (sceneMesh->vertex_normal.exists)
				{
					vert.normX = sceneMesh->vertex_normal[faceIndex].x;
					vert.normY = sceneMesh->vertex_normal[faceIndex].z;
					vert.normZ = sceneMesh->vertex_normal[faceIndex].y;
				}
				else
				{
					vert.normX = vert.normY = vert.normZ = 0.0f;
				}

				if (sceneMesh->vertex_uv.exists)
				{
					vert.u = sceneMesh->vertex_uv[faceIndex].x;
					vert.v = sceneMesh->vertex_uv[faceIndex].y;
				}
				else
				{
					vert.u = vert.v = 0.0f;
				}

				for (uint32 k = 0; k < MEX_MAX_BONE_INFLUENCE; ++k)
				{
					vert.boneIDs[k] = vertBones[vertBoneIndex].boneIDs[k];
					vert.boneWeights[k] = vertBones[vertBoneIndex].boneWeights[k];
				}

				// Construct min/max bounding box:
				// -----
				if (vert.x < subset.minMaxX[0])
					subset.minMaxX[0] = vert.x;
				if (vert.y < subset.minMaxY[0])
					subset.minMaxY[0] = vert.y;
				if (vert.z < subset.minMaxZ[0])
					subset.minMaxZ[0] = vert.z;

				if (vert.x > subset.minMaxX[1])
					subset.minMaxX[1] = vert.x;
				if (vert.y > subset.minMaxY[1])
					subset.minMaxY[1] = vert.y;
				if (vert.z > subset.minMaxZ[1])
					subset.minMaxZ[1] = vert.z;
				// -----
			}
		}

		// Construct AABB bounding box from the previously-created min/max values:
		for (uint32 i = 0; i < 8; ++i)
		{
			auto x = i % 2;
			auto y = (i / 2) % 2;
			auto z = (i / 4) % 2;
			subset.aabbX[i] = subset.minMaxX[x];
			subset.aabbY[i] = subset.minMaxY[y];
			subset.aabbZ[i] = subset.minMaxZ[z];
		}

		auto parent = meshNode->parent;
		if (parent && parent->mesh)
		{
			auto& parentMesh = mesh->subsets[parent->mesh->typed_id];
			parentMesh.children.emplace_back(sceneMesh->typed_id);
		}
		else
		{
			mesh->rootSubsets.emplace_back(sceneMesh->typed_id);
		}
	}

	mesh->animFPS = scene->settings.frames_per_second;
	// Parse animations:
	for (const auto animStack : scene->anim_stacks)
	{
		const auto stackAnim = animStack->anim;
		auto& anim = mesh->anims.emplace_back();
		anim.index = (int32)mesh->anims.size() - 1;
		CopyUFBXName(anim.name, animStack->name);
		
		auto numFrames = (uint32)((stackAnim->time_end - stackAnim->time_begin) * scene->settings.frames_per_second);
		anim.frames.resize(numFrames);

		auto time = 0.0;
		auto timeStep = 1.0 / mesh->animFPS;
		// Create transform data for each individual frame:
		for (auto& frame : anim.frames)
		{
			frame.bones.resize(mesh->bones.size());
			for (uint32 i = 0; i < frame.bones.size(); ++i)
			{
				auto& bone = frame.bones[i];
				const auto sceneBone = scene->bones[i];
				if (!sceneBone->instances.count)
					continue;

				const auto boneNode = sceneBone->instances[0];
				CopyUFBXTransform(bone, ufbx_evaluate_transform(stackAnim, boneNode, time));
			}
			time += timeStep;
		}
	}

	// Initialize mesh resources:
	if (!mesh->ResetDriverResources())
		goto finished;

	ret = meshID;
finished:
	if (scene)
		ufbx_free_scene(scene);
	if (ret == MEX_UNDEFINED && meshID != MEX_UNDEFINED)
		ReleaseMesh(meshID);

	return ret;
}

void GlobalContext::ReleaseMesh(uint32 meshID)
{
	auto& mesh = GetMesh(meshID);

	mesh.ReleaseDriverResources();
	if (!mesh.subsets.empty())
		mesh.subsets.clear();

	if (!mesh.rootSubsets.empty())
		mesh.rootSubsets.clear();

	if (!mesh.bones.empty())
		mesh.bones.clear();

	if (!mesh.rootBones.empty())
		mesh.rootBones.clear();

	if (!mesh.anims.empty())
		mesh.anims.clear();

	mesh.animFPS = 0.0;
	meshes.erase(meshes.begin() + meshID);
}


uint32 GlobalContext::AddTexture(const Texture& texture)
{
	if (textures.size() > UINT32_MAX)
		return MEX_UNDEFINED;

	auto id = (uint32)textures.size();
	textures.emplace_back(texture);
	return id;
}


uint32 GlobalContext::GetNumMeshes()
{
	return (uint32)meshes.size();
}

Mesh& GlobalContext::GetMesh(uint32 index)
{
	return meshes[index];
}


uint32 GlobalContext::GetNumTextures()
{
	return (uint32)textures.size();
}

Texture& GlobalContext::GetTexture(uint32 index)
{
	return textures[index];
}


bool GlobalContext::TextureExists(const Texture& texture, uint32& outID)
{
	outID = MEX_UNDEFINED;
	for (std::size_t i = 0; i < textures.size(); ++i)
	{
		const auto& other = textures[i];
		if (texture.imageID == MEX_UNDEFINED16 && other.imageID == MEX_UNDEFINED16 ?
			wcscmp(texture.fileName, other.fileName) == 0 :
			texture.imageID == other.imageID)
		{
			outID = (uint32)i;
			return true;
		}
	}
	return false;
}


bool GlobalContext::NeedReset()
{
	return needReset;
}

/*

---------- RENDER CONTEXT ----------

*/

RenderContext::RenderContext()
{
	MatrixIdentity(identityMatrix);
	memset(boneTransforms, 0x0, sizeof(boneTransforms));
	memset(boneTransformsNormal, 0x0, sizeof(boneTransformsNormal));
	memset(currentSubsetDeforms, 0x0, sizeof(currentSubsetDeforms));
}

RenderContext::~RenderContext()
{
	if (!inited)
		return;

	ReleaseDriverResources();
	inited = false;
}


void RenderContext::ReadProperties(const EDITDATA* edPtr)
{
	// These properties are being read by index rather than by name for performance reasons:

	meshProps.transform.x = meshProps.transform.y = meshProps.transform.z = 0.0f;
	meshProps.transform.yaw = edPtr->Props.GetPropertyNum(4);
	meshProps.transform.pitch = edPtr->Props.GetPropertyNum(5);
	meshProps.transform.roll = edPtr->Props.GetPropertyNum(6);

	meshProps.transform.scaleX = edPtr->Props.GetPropertyNum(8);
	meshProps.transform.scaleY = edPtr->Props.GetPropertyNum(9);
	meshProps.transform.scaleZ = edPtr->Props.GetPropertyNum(10);

	meshProps.filtering = edPtr->Props.GetPropertyStr(12) == _T("Point") ? MeshFiltering_Point : MeshFiltering_Bilinear;
	meshProps.addressMode = edPtr->Props.GetPropertyStr(13) == _T("Wrap") ? MeshAddressing_Wrap : MeshAddressing_Clamp;
	meshProps.wireframe = edPtr->Props.IsPropChecked(15);

	auto cullMode = edPtr->Props.GetPropertyStr(16);
	if (cullMode == _T("Back-face"))
		meshProps.cull = MeshCull_Back;
	else if (cullMode == _T("Front-face"))
		meshProps.cull = MeshCull_Front;
	else
		meshProps.cull = MeshCull_None;

	animProps.loop = edPtr->Props.IsPropChecked(19);

	auto frameInterp = edPtr->Props.GetPropertyStr(20);
	if (frameInterp == _T("Always"))
		animProps.frameInterp = FrameInterpolation_Always;
	else if (frameInterp == _T("Sometimes"))
		animProps.frameInterp = FrameInterpolation_Sometimes;
	else
		animProps.frameInterp = FrameInterpolation_Never;

	animProps.animSpeed = edPtr->Props.GetPropertyNum(21);
	animProps.animInterpSpeed = std::max<float64>(edPtr->Props.GetPropertyNum(22), 0.0);

	lightingProps.ambient = (uint32)edPtr->Props.GetPropertyNum(26);
	lightingProps.diffuse = (uint32)edPtr->Props.GetPropertyNum(27);
	lightingProps.fogColor = (uint32)edPtr->Props.GetPropertyNum(28);
	lightingProps.dirX = edPtr->Props.GetPropertyNum(30);
	lightingProps.dirY = edPtr->Props.GetPropertyNum(31);
	lightingProps.dirZ = edPtr->Props.GetPropertyNum(32);
	lightingProps.fogDist = edPtr->Props.GetPropertyNum(34);

	cameraProps.proj = edPtr->Props.GetPropertyStr(38) == _T("Perspective") ? CameraProjection_Perspective : CameraProjection_Orthographic;
	cameraProps.fov = edPtr->Props.GetPropertyNum(40);
	cameraProps.nearClip = edPtr->Props.GetPropertyNum(41);
	cameraProps.farClip = edPtr->Props.GetPropertyNum(42);
	cameraProps.x = edPtr->Props.GetPropertyNum(44);
	cameraProps.y = edPtr->Props.GetPropertyNum(45);
	cameraProps.z = edPtr->Props.GetPropertyNum(46);
	cameraProps.yaw = edPtr->Props.GetPropertyNum(48);
	cameraProps.pitch = edPtr->Props.GetPropertyNum(49);
	cameraProps.roll = edPtr->Props.GetPropertyNum(50);
}


void RenderContext::CreateSurface(int32 renderWidth, int32 renderHeight)
{
	width = std::clamp(renderWidth, 1, MEX_MAX_SURFACE_SIZE);
	height = std::clamp(renderHeight, 1, MEX_MAX_SURFACE_SIZE);

	cMask.reset();
	updateCMask = true;

	PlatformCreateSurface();
}


bool RenderContext::SetMesh(uint32 meshID)
{
	if (meshID == MEX_UNDEFINED)
		return false;

	if (this->meshID != meshID)
	{
		auto& mesh = globalCtx->GetMesh(meshID);

		meshLoaded = true;
		animPlaying = true;
		currentAnim = 0;
		currentFrame = 0.0;
		currentAnimInterp = 1.0;

		if (!subsetTransforms.empty())
			subsetTransforms.clear();
		for (auto& subset : mesh.subsets)
			subsetTransforms.emplace_back(subset.transform);

		if (!subsetAABBs.empty())
			subsetAABBs.clear();
		subsetAABBs.resize(mesh.subsets.size(), 1.0f);

		this->meshID = meshID;
	}

	return true;
}

void RenderContext::UpdateMesh(float64 baseFPS, float64 mvtTimerCoef)
{
	if (meshID == MEX_UNDEFINED)
		return;

	auto& mesh = globalCtx->GetMesh(meshID);

	if (currentAnim >= 0 && currentAnim < (int32)mesh.anims.size())
	{
		const auto& anim = mesh.anims[currentAnim];
		if (!anim.frames.empty())
		{
			auto startIndex = std::clamp<uint32>((uint32)currentFrame, 0, (uint32)anim.frames.size() - 1);
			auto endIndex = std::clamp<uint32>(animProps.animSpeed >= 0.0 ? (uint32)currentFrame + 1 : (uint32)currentFrame - 1, 0, (uint32)anim.frames.size() - 1);

			auto t = (float)(currentFrame - (int64)currentFrame);

			const auto& startAnimFrame = anim.frames[startIndex];
			const auto& endAnimFrame = anim.frames[endIndex];
			// Calculate current bone transforms, based on interpolation setting:
			switch (animProps.frameInterp)
			{
			case FrameInterpolation_Always:
			case FrameInterpolation_Sometimes:
			{
				auto interpT = (float)currentAnimInterp;
				for (uint32 i = 0; i < mesh.bones.size(); ++i)
				{
					auto& bone = mesh.bones[i];
					const auto& startTransform = startAnimFrame.bones[i];
					const auto& endTransform = endAnimFrame.bones[i];
					bone.interpTransform.x = Lerp(startTransform.x, endTransform.x, t);
					bone.interpTransform.y = Lerp(startTransform.y, endTransform.y, t);
					bone.interpTransform.z = Lerp(startTransform.z, endTransform.z, t);
					bone.interpTransform.quatW = Slerp(startTransform.quatW, endTransform.quatW, t);
					bone.interpTransform.quatX = Slerp(startTransform.quatX, endTransform.quatX, t);
					bone.interpTransform.quatY = Slerp(startTransform.quatY, endTransform.quatY, t);
					bone.interpTransform.quatZ = Slerp(startTransform.quatZ, endTransform.quatZ, t);
					bone.interpTransform.scaleX = Lerp(startTransform.scaleX, endTransform.scaleX, t);
					bone.interpTransform.scaleY = Lerp(startTransform.scaleY, endTransform.scaleY, t);
					bone.interpTransform.scaleZ = Lerp(startTransform.scaleZ, endTransform.scaleZ, t);

					if (animProps.frameInterp == FrameInterpolation_Always)
					{
						bone.finalTransform.x = Lerp(bone.prevInterpTransform.x, bone.interpTransform.x, interpT);
						bone.finalTransform.y = Lerp(bone.prevInterpTransform.y, bone.interpTransform.y, interpT);
						bone.finalTransform.z = Lerp(bone.prevInterpTransform.z, bone.interpTransform.z, interpT);
						bone.finalTransform.quatW = Slerp(bone.prevInterpTransform.quatW, bone.interpTransform.quatW, interpT);
						bone.finalTransform.quatX = Slerp(bone.prevInterpTransform.quatX, bone.interpTransform.quatX, interpT);
						bone.finalTransform.quatY = Slerp(bone.prevInterpTransform.quatY, bone.interpTransform.quatY, interpT);
						bone.finalTransform.quatZ = Slerp(bone.prevInterpTransform.quatZ, bone.interpTransform.quatZ, interpT);
						bone.finalTransform.scaleX = Lerp(bone.prevInterpTransform.scaleX, bone.interpTransform.scaleX, interpT);
						bone.finalTransform.scaleY = Lerp(bone.prevInterpTransform.scaleY, bone.interpTransform.scaleY, interpT);
						bone.finalTransform.scaleZ = Lerp(bone.prevInterpTransform.scaleZ, bone.interpTransform.scaleZ, interpT);
					}
					else
					{
						bone.finalTransform = bone.interpTransform;
						bone.prevInterpTransform = bone.finalTransform;
					}
				}
				break;
			}
			case FrameInterpolation_Never:
			default:
			{
				for (uint32 i = 0; i < (uint32)mesh.bones.size(); ++i)
				{
					auto& bone = mesh.bones[i];
					bone.finalTransform = bone.prevInterpTransform = startAnimFrame.bones[i];
				}
				break;
			}
			}

			if (animPlaying)
			{
				auto step = (1.0 / baseFPS) * mesh.animFPS * animProps.animSpeed * mvtTimerCoef;
				currentFrame += step;
				currentAnimInterp += abs(step * animProps.animInterpSpeed);
				if (currentAnimInterp > 1.0)
					currentAnimInterp = 1.0;

				if (currentFrame >= anim.frames.size())
				{
					if (animProps.loop)
					{
						for (auto& bone : mesh.bones)
							bone.prevInterpTransform = bone.finalTransform;

						currentFrame = currentAnimInterp = 0.0;
					}
					else
					{
						currentFrame = (float64)anim.frames.size();
					}
				}
				else if (currentFrame < 0.0)
				{
					if (animProps.loop)
					{
						for (auto& bone : mesh.bones)
							bone.prevInterpTransform = bone.finalTransform;

						currentFrame = (float64)anim.frames.size() - 1.0;
						currentAnimInterp = 0.0;
					}
					else
					{
						currentFrame = 0.0;
					}
					currentFrame = animProps.loop ? (float64)anim.frames.size() - 1.0 : 0.0;
				}
			}
		}
		else
		{
			for (auto& bone : mesh.bones)
				bone.interpTransform = bone.prevInterpTransform = bone.finalTransform = bone.transform;
		}
	}

	// Update matrices:
	// -----
	WorldViewProj(wvp, wNormal);
	for (auto rootBoneID : mesh.rootBones)
		UpdateBoneTransforms(mesh, rootBoneID, identityMatrix, identityMatrix);
	// -----
}

void RenderContext::LoadTextures(void* fusionApp, const EDITDATA* edPtr)
{
	auto numImages = edPtr->Props.GetPropertyNumImages(2);
	for (uint16 i = 0; i < numImages; ++i)
	{
		Texture texture;
		texture.imageID = edPtr->Props.GetPropertyImageID(2, i);

		uint32 id;
		if (!globalCtx->TextureExists(texture, id) && globalCtx->CreateTextureSurfaceFromImageBank(fusionApp, texture))
			id = globalCtx->AddTexture(texture);

		if (id != MEX_UNDEFINED)
			textureIDs.emplace_back(id);
	}
}

void RenderContext::ReplaceTexture(RunHeader* rhPtr, int32 textureID, const wchar* fileName)
{
	if (textureID < 0 || (std::size_t)textureID >= textureIDs.size())
		return;

	Texture texture;
	wcsncpy(texture.fileName, fileName, MEX_MAX_FILENAME_CHARS);
	texture.fileName[MEX_MAX_FILENAME_CHARS] = L'\0';
	texture.imageID = MEX_UNDEFINED16;

	uint32 id;
	if (!globalCtx->TextureExists(texture, id) && globalCtx->CreateTextureSurfaceFromFile(rhPtr, texture))
		id = globalCtx->AddTexture(texture);

	if (id != MEX_UNDEFINED)
		textureIDs[textureID] = id;
}


void RenderContext::PlayAnim()
{
	animPlaying = true;
}

void RenderContext::StopAnim()
{
	animPlaying = false;
}


void RenderContext::SetCurrentAnim(int32 anim)
{
	if (meshID == MEX_UNDEFINED)
		return;
	auto& mesh = globalCtx->GetMesh(meshID);

	if (anim != currentAnim && anim >= 0 && anim < (int32)mesh.anims.size())
	{
		for (auto& bone : mesh.bones)
			bone.prevInterpTransform = bone.finalTransform;

		currentAnim = anim;
		currentFrame = currentAnimInterp = 0.0;
	}
}

void RenderContext::SetCurrentAnimFrame(float64 frame)
{
	if (meshID == MEX_UNDEFINED)
		return;
	auto& mesh = globalCtx->GetMesh(meshID);

	if (mesh.anims.empty())
		return;

	currentFrame = std::clamp<float64>(frame, 0.0, mesh.anims.size());
}


void RenderContext::SetGlobalContext(GlobalContext* globalCtx)
{
	this->globalCtx = globalCtx;
}


Mesh* RenderContext::GetMesh()
{
	return meshID != MEX_UNDEFINED ? &globalCtx->GetMesh(meshID) : nullptr;
}


int32 RenderContext::GetNumSubsets()
{
	if (meshID == MEX_UNDEFINED)
		return 0;
	auto& mesh = globalCtx->GetMesh(meshID);

	return (int32)mesh.subsets.size();
}

Subset* RenderContext::GetSubset(int32 index)
{
	if (meshID == MEX_UNDEFINED)
		return nullptr;
	auto& mesh = globalCtx->GetMesh(meshID);

	if (index < 0 || index >= (int32)mesh.subsets.size())
		return nullptr;

	return &mesh.subsets[index];
}

Subset* RenderContext::GetSubset(const wchar* name)
{
	if (meshID == MEX_UNDEFINED)
		return nullptr;
	auto& mesh = globalCtx->GetMesh(meshID);

	for (auto& subset : mesh.subsets)
		if (wcscmp(subset.name, name) == 0)
			return &subset;

	return nullptr;
}

Transform* RenderContext::GetLocalSubsetTransform(const Subset* subset)
{
	return &subsetTransforms[subset->index];
}

float32* RenderContext::GetLocalSubsetAABB(const Subset* subset)
{
	return &subsetAABBs[subset->index];
}


int32 RenderContext::GetNumAnims()
{
	if (meshID == MEX_UNDEFINED)
		return 0;
	auto& mesh = globalCtx->GetMesh(meshID);

	return (int32)mesh.anims.size();
}

int32 RenderContext::GetCurrentAnim()
{
	return currentAnim;
}

float64 RenderContext::GetCurrentAnimFrame()
{
	return currentFrame;
}

float64 RenderContext::GetAnimFPS()
{
	if (meshID == MEX_UNDEFINED)
		return 0.0;
	auto& mesh = globalCtx->GetMesh(meshID);

	return mesh.animFPS;
}

AnimClip* RenderContext::GetAnim(int32 index)
{
	if (meshID == MEX_UNDEFINED)
		return nullptr;
	auto& mesh = globalCtx->GetMesh(meshID);

	if (index < 0 || index >= (int32)mesh.anims.size())
		return nullptr;

	return &mesh.anims[index];
}

AnimClip* RenderContext::GetAnim(const wchar* name)
{
	if (meshID == MEX_UNDEFINED)
		return nullptr;
	auto& mesh = globalCtx->GetMesh(meshID);

	for (auto& anim : mesh.anims)
		if (wcscmp(anim.name, name) == 0)
			return &anim;

	return nullptr;
}


int32 RenderContext::GetNumBones()
{
	if (meshID == MEX_UNDEFINED)
		return 0;
	auto& mesh = globalCtx->GetMesh(meshID);

	return (int32)mesh.bones.size();
}

Bone* RenderContext::GetBone(int32 index)
{
	if (meshID == MEX_UNDEFINED)
		return nullptr;
	auto& mesh = globalCtx->GetMesh(meshID);

	if (index < 0 || index >= (int32)mesh.bones.size())
		return nullptr;

	return &mesh.bones[index];
}

Bone* RenderContext::GetBone(const wchar* name)
{
	if (meshID == MEX_UNDEFINED)
		return nullptr;
	auto& mesh = globalCtx->GetMesh(meshID);

	for (auto& bone : mesh.bones)
		if (wcscmp(bone.name, name) == 0)
			return &bone;

	return nullptr;
}


void RenderContext::GetProjectedBonePos(int32 boneID, int32 subsetID, int32& outX, int32& outY)
{
	if (meshID == MEX_UNDEFINED)
	{
		outX = outY = 0;
		return;
	}
	auto& mesh = globalCtx->GetMesh(meshID);

	if (boneID < 0 || boneID >= (int32)mesh.bones.size())
	{
		outX = outY = 0;
		return;
	}

	Matrix mat;
	if (subsetID >= 0 && subsetID < (int32)subsetTransforms.size())
	{
		Matrix subsetMat;
		MatrixTransform(subsetTransforms[subsetID], subsetMat);

		MatrixMultiply(subsetMat, boneTransforms[boneID], mat);
		MatrixMultiply(wvp, mat, mat);
	}
	else
	{
		MatrixMultiply(wvp, boneTransforms[boneID], mat);
	}

	Vertex src;
	src.x = src.y = src.z = 0.0f;

	float32 destX, destY, destZ, destW;
	MatrixMultiply(mat, src, destX, destY, destZ, destW);

	if (destW > 0.0f)
	{
		destX /= destW;
		destY /= destW;
		outX = ScreenSpaceX(destX);
		outY = ScreenSpaceY(destY);
	}
	else
	{
		outX = 0;
		outY = height;
	}
}


DarkEdif::CollisionMask* RenderContext::GetCollisionMask()
{
	if (updateCMask)
	{
		if (!cMask.get())
			cMask = DarkEdif::CollisionMask::Create(width, height);

		if (!renderOffscreen)
		{
			DrawToSurface();
			renderOffscreen = true;
		}

		UpdateCollisionMask();
		updateCMask = false;
	}
	return cMask.get();
}


int32 RenderContext::GetSurfaceWidth()
{
	return width;
}

int32 RenderContext::GetSurfaceHeight()
{
	return height;
}


bool RenderContext::IsInited()
{
	return inited;
}

bool RenderContext::IsMeshLoaded()
{
	return meshLoaded;
}

bool RenderContext::IsRenderingOffscreen()
{
	return renderOffscreen;
}


bool RenderContext::IsAnimPlaying()
{
	return animPlaying;
}


void RenderContext::WorldViewProj(Matrix& out, Matrix& outNormal)
{
	Matrix world;
	MatrixTransform(meshProps.transform, world, &outNormal);

	Matrix view;
	MatrixTranslation(-cameraProps.x, -cameraProps.y, -cameraProps.z, view);

	Matrix yawMat;
	MatrixYaw(cameraProps.yaw, yawMat);

	Matrix pitchMat;
	MatrixPitch(cameraProps.pitch, pitchMat);

	Matrix rollMat;
	MatrixRoll(cameraProps.roll, rollMat);

	Matrix angleMat;
	MatrixMultiply(pitchMat, yawMat, angleMat);
	MatrixMultiply(rollMat, angleMat, angleMat);
	MatrixMultiply(angleMat, view, view);

	Matrix proj;
	if (cameraProps.proj == CameraProjection_Perspective)
		MatrixPerspective(cameraProps.fov, (float)width / (float)height, cameraProps.nearClip, cameraProps.farClip, proj);
	else
		MatrixOrthographic(cameraProps.fov, (float)width / (float)height, cameraProps.nearClip, cameraProps.farClip, proj);

	MatrixMultiply(view, world, out);
	MatrixMultiply(proj, out, out);
}

void RenderContext::UpdateBoneTransforms(Mesh& mesh, uint32 boneID, Matrix parentTransform, Matrix parentNormal)
{
	const auto& bone = mesh.bones[boneID];

	Matrix localTransform, localTransformNormal;
	MatrixTransform(bone.finalTransform, localTransform, &localTransformNormal);

	auto& destTransform = boneTransforms[boneID];
	auto& destTransformNormal = boneTransformsNormal[boneID];
	MatrixMultiply(parentTransform, localTransform, destTransform);
	MatrixMultiply(parentNormal, localTransformNormal, destTransformNormal);

	for (uint32 i = 0; i < bone.numChildren; ++i)
		UpdateBoneTransforms(mesh, bone.children[i], destTransform, destTransformNormal);
}

void RenderContext::UpdateSubsetDeforms(Mesh& mesh, const Subset& subset, uint32 boneID)
{
	const auto& bone = mesh.bones[boneID];
	MatrixMultiply(boneTransforms[boneID], subset.meshToBone[boneID], currentSubsetDeforms[boneID]);

	for (uint32 i = 0; i < bone.numChildren; ++i)
		UpdateSubsetDeforms(mesh, subset, bone.children[i]);
}

void RenderContext::DrawSubset(Mesh& mesh, uint32 subsetID, Matrix parentTransform, Matrix parentNormal)
{
	const auto& subset = mesh.subsets[subsetID];

	Matrix mat, matNormal;
	MatrixTransform(subsetTransforms[subsetID], mat, &matNormal);
	// If the subset has bone relations, apply the bone transforms onto it:
	if (subset.parentBone < mesh.bones.size())
	{
		MatrixMultiply(boneTransforms[subset.parentBone], mat, mat);
		MatrixMultiply(boneTransformsNormal[subset.parentBone], matNormal, matNormal);

		// From what I understand, since the subset transform is being applied BEFORE the bone transform, the global space transform that's done by FBX meshes needs to be reapplied:
		// -----
		const auto& bone = mesh.bones[subset.parentBone];
		if (bone.spaceTransformIndex != MEX_UNDEFINED)
		{
			Matrix spaceTransform;
			MatrixTransform(mesh.spaceTransforms[bone.spaceTransformIndex], spaceTransform);
			MatrixMultiply(spaceTransform, mat, mat);
		}
		// -----
	}
	MatrixMultiply(parentTransform, mat, mat);
	MatrixMultiply(parentNormal, matNormal, matNormal);

	Matrix finalMat, finalMatNormal;
	MatrixMultiply(wvp, mat, finalMat);
	MatrixMultiply(wNormal, matNormal, finalMatNormal);

	if (SubsetInsideFrustum(mesh, subset, finalMat))
		PlatformDrawSubset(mesh, subset, finalMat, finalMatNormal);

	for (auto childSubset : subset.children)
		DrawSubset(mesh, childSubset, mat, matNormal);
}

bool RenderContext::SubsetInsideFrustum(Mesh& mesh, const Subset& subset, const Matrix& finalMat)
{
	struct Point
	{
		float32 x;
		float32 y;
		float32 z;
		float32 w;
	} aabbPoints[8];
	const auto aabbRadius = subsetAABBs[subset.index];

	uint32 numOutOfBounds;
	// Create AABB points:
	for (uint32 i = 0; i < 8; ++i)
	{
		aabbPoints[i].x = subset.aabbX[i] * aabbRadius;
		aabbPoints[i].y = subset.aabbY[i] * aabbRadius;
		aabbPoints[i].z = subset.aabbZ[i] * aabbRadius;
		aabbPoints[i].w = 1.0f;
		MatrixMultiply(finalMat, aabbPoints[i].x, aabbPoints[i].y, aabbPoints[i].z, aabbPoints[i].w, aabbPoints[i].x, aabbPoints[i].y, aabbPoints[i].z, aabbPoints[i].w);
	}

	numOutOfBounds = 0;
	// Obstructed by near plane?
	for (uint32 i = 0; i < 8; ++i)
	{
		if (aabbPoints[i].z + aabbPoints[i].w < 0.0f)
			numOutOfBounds++;
	}
	if (numOutOfBounds >= 8)
		return false;

	numOutOfBounds = 0;
	// Obstructed by far plane?
	for (uint32 i = 0; i < 8; ++i)
	{
		if (-aabbPoints[i].z + aabbPoints[i].w < 0.0f)
			numOutOfBounds++;
	}
	if (numOutOfBounds >= 8)
		return false;

	// Clamp W to near/far boundaries:
	for (uint32 i = 0; i < 8; ++i)
	{
		auto nearClip = aabbPoints[i].z + aabbPoints[i].w;
		if (nearClip < 0.0f)
			aabbPoints[i].w -= nearClip;

		auto farClip = -aabbPoints[i].z + aabbPoints[i].w;
		if (farClip < 0)
			aabbPoints[i].w += farClip;
	}

	numOutOfBounds = 0;
	// Outside the left side?
	for (uint32 i = 0; i < 8; ++i)
	{
		auto x = aabbPoints[i].x / aabbPoints[i].w;
		if (x < -1.0f)
			numOutOfBounds++;
	}
	if (numOutOfBounds >= 8)
		return false;

	numOutOfBounds = 0;
	// Outside the right side?
	for (uint32 i = 0; i < 8; ++i)
	{
		auto x = aabbPoints[i].x / aabbPoints[i].w;
		if (x >= 1.0f)
			numOutOfBounds++;
	}
	if (numOutOfBounds >= 8)
		return false;

	numOutOfBounds = 0;
	// Outside the top side?
	for (uint32 i = 0; i < 8; ++i)
	{
		auto y = aabbPoints[i].y / aabbPoints[i].w;
		if (y < -1.0f)
			numOutOfBounds++;
	}
	if (numOutOfBounds >= 8)
		return false;

	numOutOfBounds = 0;
	// Outside the bottom side?
	for (uint32 i = 0; i < 8; ++i)
	{
		auto y = aabbPoints[i].y / aabbPoints[i].w;
		if (y >= 1.0f)
			numOutOfBounds++;
	}
	if (numOutOfBounds >= 8)
		return false;

	return true;
}

void RenderContext::WriteBoneDeformTexture(Mesh& mesh, float32* dest, uint32 pitch)
{
	auto transforms = currentSubsetDeforms;
	for (int32 i = 0; i < (int32)mesh.bones.size(); ++i)
	{
		const auto& boneTransform = currentSubsetDeforms[i];
		const auto& boneTransformNormal = boneTransformsNormal[i];
		auto matrixPixelX = (i * 4) % MEX_BONE_TEX_SIZE;
		auto matrixPixelY = (i * 4) / MEX_BONE_TEX_SIZE;
		auto destMat = (float32*)((byte*)(dest + (matrixPixelX * 4)) + matrixPixelY * pitch);
		auto destMatNormal = ((byte*)destMat) + (MEX_BONE_TEX_SIZE / 2) * pitch; // the normals will just reside halfway below
		auto srcMat = boneTransform.rows;
		auto srcMatNormal = boneTransformNormal.rows;
		memcpy(destMat, srcMat, sizeof(boneTransform.rows));
		memcpy(destMatNormal, srcMatNormal, sizeof(boneTransformNormal.rows));
	}
}

void RenderContext::WriteMaskData(uint16* destMask, const uint32* srcData, uint32 destPitchBytes, uint32 srcPitchBytes)
{
	// Fusion creates an sMask by filling out it's header then the pixel data is
	// stored at the end of it. Each pixel is packed into 1 bit; an 8x1 mask will stil be
	// 1 byte as each bit corresponds to a single pixel. A bit is set if the pixel is opaque,
	// otherwise it's cleared if transparent.

	auto src = (byte*)srcData;
	auto dest = (byte*)destMask;
	for (int32 r = 0; r < height; ++r)
	{
		auto destBits = (uint16*)dest;
		auto bitIndex = (uint16)0;
		auto srcPixels = (uint32*)src;

		*destBits = 0;
		for (int32 p = 0; p < width; ++p)
		{
			auto maskValue = (uint16)(MEX_GET_A(*(srcPixels++)) > 0);
			if (bitIndex >= CHAR_BIT * sizeof(uint16))
			{
				*(++destBits) = 0;
				bitIndex = 0;
			}
			*destBits = *destBits | (maskValue << bitIndex++);
		}
		src += srcPitchBytes;
		dest += destPitchBytes;
	}
}

int32 RenderContext::ScreenSpaceX(float32 x)
{
	auto half = (float32)width / 2.0f;
	return (int32)(half + half * x);
}

int32 RenderContext::ScreenSpaceY(float32 y)
{
	auto half = (float32)height / 2.0f;
	return (int32)(half + half * y);
}

/*

---------- MATRICES ----------

*/

void MeshEx::MatrixIdentity(Matrix& out)
{
	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = out.rows[1][1] = out.rows[2][2] = out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixTranslation(float32 x, float32 y, float32 z, Matrix& out)
{
	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = 1.0f;
	out.rows[0][3] = x;

	out.rows[1][1] = 1.0f;
	out.rows[1][3] = y;

	out.rows[2][2] = 1.0f;
	out.rows[2][3] = z;

	out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixYaw(float32 angle, Matrix& out)
{
	auto sine = Sin(angle);
	auto cosine = Cos(angle);

	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = cosine;
	out.rows[0][2] = -sine;

	out.rows[1][1] = 1.0f;

	out.rows[2][0] = sine;
	out.rows[2][2] = cosine;

	out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixPitch(float32 angle, Matrix& out)
{
	auto sine = Sin(angle);
	auto cosine = Cos(angle);

	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = 1.0f;

	out.rows[1][1] = cosine;
	out.rows[1][2] = -sine;

	out.rows[2][1] = sine;
	out.rows[2][2] = cosine;

	out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixRoll(float32 angle, Matrix& out)
{
	auto sine = Sin(angle);
	auto cosine = Cos(angle);

	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = cosine;
	out.rows[0][1] = -sine;

	out.rows[1][0] = sine;
	out.rows[1][1] = cosine;

	out.rows[2][2] = 1.0f;

	out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixQuaternion(float32 w, float32 x, float32 y, float32 z, Matrix& out)
{
	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = (w * w) + (x * x) - (y * y) - (z * z);
	out.rows[0][1] = (2.0f * x * y) - (2.0f * w * z);
	out.rows[0][2] = (2.0f * x * z) + (2.0f * w * y);

	out.rows[1][0] = (2.0f * x * y) + (2.0f * w * z);
	out.rows[1][1] = (w * w) - (x * x) + (y * y) - (z * z);
	out.rows[1][2] = (2.0f * y * z) - (2.0f * w * x);

	out.rows[2][0] = (2.0f * x * z) - (2.0f * w * y);
	out.rows[2][1] = (2.0f * y * z) + (2.0f * w * x);
	out.rows[2][2] = (w * w) - (x * x) - (y * y) + (z * z);

	out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixScaling(float32 x, float32 y, float32 z, Matrix& out)
{
	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = x;
	out.rows[1][1] = y;
	out.rows[2][2] = z;
	out.rows[3][3] = 1.0f;
}

void MeshEx::MatrixTransform(const Transform& transform, Matrix& out, Matrix* outNormal)
{
	MatrixScaling(transform.scaleX, transform.scaleY, transform.scaleZ, out);
	if (fmod(transform.yaw, 360.0f) || fmod(transform.pitch, 360.0f) || fmod(transform.roll, 360.0f))
	{
		Matrix yawMat;
		MatrixYaw(transform.yaw, yawMat);

		Matrix pitchMat;
		MatrixPitch(transform.pitch, pitchMat);

		Matrix rollMat;
		MatrixRoll(transform.roll, rollMat);

		Matrix rotationMat;
		MatrixMultiply(pitchMat, yawMat, rotationMat);
		MatrixMultiply(rollMat, rotationMat, rotationMat);
		MatrixMultiply(rotationMat, out, out);
		if (outNormal)
			*outNormal = rotationMat;
	}
	else if (outNormal)
	{
		MatrixIdentity(*outNormal);
	}

	Matrix translation;
	MatrixTranslation(transform.x, transform.y, transform.z, translation);
	MatrixMultiply(translation, out, out);
}

void MeshEx::MatrixTransform(const TransformQuat& transform, Matrix& out, Matrix* outNormal)
{
	MatrixScaling(transform.scaleX, transform.scaleY, transform.scaleZ, out);

	Matrix rotation;
	MatrixQuaternion(transform.quatW, transform.quatX, transform.quatY, transform.quatZ, rotation);
	MatrixMultiply(rotation, out, out);
	if (outNormal)
		*outNormal = rotation;

	Matrix translation;
	MatrixTranslation(transform.x, transform.y, transform.z, translation);
	MatrixMultiply(translation, out, out);
}


void MeshEx::MatrixPerspective(float32 fov, float32 aspectRatio, float32 nearClip, float32 farClip, Matrix& out)
{
	auto focalLength = 1.0f / Tan(fov / 2.0f);

	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = focalLength / aspectRatio;
	out.rows[1][1] = focalLength;

	out.rows[2][2] = (nearClip + farClip) / (farClip - nearClip);
	out.rows[2][3] = (2.0f * nearClip * farClip) / (nearClip - farClip);
	out.rows[3][2] = 1.0f;
}

void MeshEx::MatrixOrthographic(float32 fov, float32 aspectRatio, float32 nearClip, float32 farClip, Matrix& out)
{
	auto focalLength = 1.0f / Tan(fov / 2.0f);

	memset(&out, 0x0, sizeof(out));
	out.rows[0][0] = focalLength / aspectRatio;
	out.rows[1][1] = focalLength;

	out.rows[2][2] = 1.0f / (farClip - nearClip);
	out.rows[2][3] = -nearClip / (farClip - nearClip);
	out.rows[3][3] = 1.0f;
}


void MeshEx::MatrixMultiply(Matrix first, Matrix second, Matrix& out)
{
	// X:
	// -----
	out.rows[0][0] =
		first.rows[0][0] * second.rows[0][0] +
		first.rows[0][1] * second.rows[1][0] +
		first.rows[0][2] * second.rows[2][0] +
		first.rows[0][3] * second.rows[3][0];

	out.rows[0][1] =
		first.rows[0][0] * second.rows[0][1] +
		first.rows[0][1] * second.rows[1][1] +
		first.rows[0][2] * second.rows[2][1] +
		first.rows[0][3] * second.rows[3][1];

	out.rows[0][2] =
		first.rows[0][0] * second.rows[0][2] +
		first.rows[0][1] * second.rows[1][2] +
		first.rows[0][2] * second.rows[2][2] +
		first.rows[0][3] * second.rows[3][2];

	out.rows[0][3] =
		first.rows[0][0] * second.rows[0][3] +
		first.rows[0][1] * second.rows[1][3] +
		first.rows[0][2] * second.rows[2][3] +
		first.rows[0][3] * second.rows[3][3];
	// -----

	// Y:
	// -----
	out.rows[1][0] =
		first.rows[1][0] * second.rows[0][0] +
		first.rows[1][1] * second.rows[1][0] +
		first.rows[1][2] * second.rows[2][0] +
		first.rows[1][3] * second.rows[3][0];

	out.rows[1][1] =
		first.rows[1][0] * second.rows[0][1] +
		first.rows[1][1] * second.rows[1][1] +
		first.rows[1][2] * second.rows[2][1] +
		first.rows[1][3] * second.rows[3][1];

	out.rows[1][2] =
		first.rows[1][0] * second.rows[0][2] +
		first.rows[1][1] * second.rows[1][2] +
		first.rows[1][2] * second.rows[2][2] +
		first.rows[1][3] * second.rows[3][2];

	out.rows[1][3] =
		first.rows[1][0] * second.rows[0][3] +
		first.rows[1][1] * second.rows[1][3] +
		first.rows[1][2] * second.rows[2][3] +
		first.rows[1][3] * second.rows[3][3];
	// -----

	// Z:
	// -----
	out.rows[2][0] =
		first.rows[2][0] * second.rows[0][0] +
		first.rows[2][1] * second.rows[1][0] +
		first.rows[2][2] * second.rows[2][0] +
		first.rows[2][3] * second.rows[3][0];

	out.rows[2][1] =
		first.rows[2][0] * second.rows[0][1] +
		first.rows[2][1] * second.rows[1][1] +
		first.rows[2][2] * second.rows[2][1] +
		first.rows[2][3] * second.rows[3][1];

	out.rows[2][2] =
		first.rows[2][0] * second.rows[0][2] +
		first.rows[2][1] * second.rows[1][2] +
		first.rows[2][2] * second.rows[2][2] +
		first.rows[2][3] * second.rows[3][2];

	out.rows[2][3] =
		first.rows[2][0] * second.rows[0][3] +
		first.rows[2][1] * second.rows[1][3] +
		first.rows[2][2] * second.rows[2][3] +
		first.rows[2][3] * second.rows[3][3];
	// -----

	// W:
	// -----
	out.rows[3][0] =
		first.rows[3][0] * second.rows[0][0] +
		first.rows[3][1] * second.rows[1][0] +
		first.rows[3][2] * second.rows[2][0] +
		first.rows[3][3] * second.rows[3][0];

	out.rows[3][1] =
		first.rows[3][0] * second.rows[0][1] +
		first.rows[3][1] * second.rows[1][1] +
		first.rows[3][2] * second.rows[2][1] +
		first.rows[3][3] * second.rows[3][1];

	out.rows[3][2] =
		first.rows[3][0] * second.rows[0][2] +
		first.rows[3][1] * second.rows[1][2] +
		first.rows[3][2] * second.rows[2][2] +
		first.rows[3][3] * second.rows[3][2];

	out.rows[3][3] =
		first.rows[3][0] * second.rows[0][3] +
		first.rows[3][1] * second.rows[1][3] +
		first.rows[3][2] * second.rows[2][3] +
		first.rows[3][3] * second.rows[3][3];
	// -----
}

void MeshEx::MatrixMultiply(Matrix first, Vertex second, float32& outX, float32& outY, float32& outZ, float32& outW)
{
	outX =
		(first.rows[0][0] * second.x) +
		(first.rows[0][1] * second.y) +
		(first.rows[0][2] * second.z) +
		first.rows[0][3];

	outY =
		(first.rows[1][0] * second.x) +
		(first.rows[1][1] * second.y) +
		(first.rows[1][2] * second.z) +
		first.rows[1][3];

	outZ =
		(first.rows[2][0] * second.x) +
		(first.rows[2][1] * second.y) +
		(first.rows[2][2] * second.z) +
		first.rows[2][3];

	outW =
		(first.rows[3][0] * second.x) +
		(first.rows[3][1] * second.y) +
		(first.rows[3][2] * second.z) +
		first.rows[3][3];
}

void MeshEx::MatrixMultiply(Matrix first, float32 secondX, float32 secondY, float32 secondZ, float32 secondW, float32& outX, float32& outY, float32& outZ, float32& outW)
{
	outX =
		(first.rows[0][0] * secondX) +
		(first.rows[0][1] * secondY) +
		(first.rows[0][2] * secondZ) +
		(first.rows[0][3] * secondW);

	outY =
		(first.rows[1][0] * secondX) +
		(first.rows[1][1] * secondY) +
		(first.rows[1][2] * secondZ) +
		(first.rows[1][3] * secondW);

	outZ =
		(first.rows[2][0] * secondX) +
		(first.rows[2][1] * secondY) +
		(first.rows[2][2] * secondZ) +
		(first.rows[2][3] * secondW);

	outW =
		(first.rows[3][0] * secondX) +
		(first.rows[3][1] * secondY) +
		(first.rows[3][2] * secondZ) +
		(first.rows[3][3] * secondW);
}

/*

---------- MATH ----------

*/

float32 MeshEx::Sin(float32 degrees)
{
	auto radians = degrees * MEX_PI / 180.0f;
	return sin(radians);
}

float32 MeshEx::Cos(float32 degrees)
{
	auto radians = degrees * MEX_PI / 180.0f;
	return cos(radians);
}

float32 MeshEx::Tan(float32 degrees)
{
	auto radians = degrees * MEX_PI / 180.0f;
	return tan(radians);
}


float32 MeshEx::Lerp(float32 a, float32 b, float32 t)
{
	return a + t * (b - a);
}

float32 MeshEx::LerpAngle(float32 a, float32 b, float32 t)
{
	auto diff = fmod(b - a + 180.0f, 360.0f) - 180.0f;
	return a + diff * t;
}

float32 MeshEx::Slerp(float32 a, float32 b, float32 t)
{
	return (1.0f - t) * a + (t * b);
}
