#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>
#include "DarkEdif.hpp"

#ifdef _WIN32
#include <CFile.hpp>
#include <Windows.h>
#include <d3d9.h>
#include <d3d11.h>
#endif

// DEFINES:
// -----
#define MEX_PI 3.14159265359f
#define MEX_GET_R(c) ((c) & 0xFF)
#define MEX_GET_G(c) (((c) & 0xFF00) >> 8)
#define MEX_GET_B(c) (((c) & 0xFF0000) >> 16)
#define MEX_GET_A(c) (((c) & 0xFF000000) >> 24)
#define MEX_MAX_BONES 128
// Max width/height of bone matrices texture
#define MEX_BONE_TEX_SIZE 64
#define MEX_MAX_BONE_INFLUENCE 4
#define MEX_MAX_NAME_CHARS 100
#define MEX_MAX_FILENAME_CHARS 512 // an arbitrary value, really
#define MEX_UNDEFINED_BONE -1.0f
// Max width/height of mesh surface
#define MEX_MAX_SURFACE_SIZE 4096
// Used to indicate an invalid 32-bit ID (index)
#define MEX_UNDEFINED uint32(-1)
// Used to indicate an invalid 16-bit ID (index)
#define MEX_UNDEFINED16 uint16(-1)
// Used to indicate an invalid 8-bit ID (index)
#define MEX_UNDEFINED8 uint8(-1)

#ifdef _WIN32
#define MEX_FILE_INVALID ((File)nullptr)
#endif
// -----

// TYPEDEFS:
// -----
// 8-bit, unsigned integer
typedef std::uint8_t uchar, byte, uint8;
// 8-bit, signed integer
typedef std::int8_t int8;
// 16-bit, signed integer
typedef std::int16_t int16;
// 16 bit, unsigned integer
typedef std::uint16_t ushort, word, uint16;
// 32-bit, signed integer
typedef std::int32_t int32;
// 32-bit, unsigned integer
typedef unsigned long ulong;
// 32-bit, unsigned integer
typedef std::uint32_t uint32, dword;
// 64-bit, unsigned integer
typedef std::uint64_t uint64;
// 64-bit, signed integer
typedef std::int64_t int64;
// Enough bits to hold a pointer value
typedef std::uintptr_t uintptr;
// 32-bit, floating-point number (at least on the platforms currently targeted)
typedef float float32;
// 64-bit, floating-point number
typedef double float64;
// Wide character
typedef wchar_t wchar;
// File
typedef void* File;
// -----

#ifdef _WIN32
#define SafeRelease(ptr) if (ptr) { ptr->Release(); ptr = nullptr; }
#endif

namespace MeshEx
{
	// Global context for the extension, namely it holds mesh data and uses it as a cache
	class GlobalContext;

	// File direction, for file I/O functions
	enum FileDirection : uint8
	{
		FileDirection_Begin,
		FileDirection_Current,
		FileDirection_End,
	};

	// Mesh filtering modes
	enum MeshFiltering : uint8
	{
		MeshFiltering_Point,
		MeshFiltering_Bilinear,
		MeshFiltering_Max
	};
	// Mesh UV addressing modes
	enum MeshAddressing : uint8
	{
		MeshAddressing_Wrap,
		MeshAddressing_Clamp,
		MeshAddressing_Max
	};
	// Mesh culling modes
	enum MeshCull : uint8
	{
		MeshCull_Back,
		MeshCull_Front,
		MeshCull_None,
		MeshCull_Max
	};
	// Frame interpolation modes
	enum FrameInterpolation : uint8
	{
		// Interpolate at all times, during each frame, and even during animation switch
		FrameInterpolation_Always,
		// Only interpolate during each frame, animation switching has no interpolation
		FrameInterpolation_Sometimes,
		// Never interpolate in any case
		FrameInterpolation_Never,
		// Used for bounds-checking
		FrameInterpolation_Max
	};

	enum CameraProjection : uint8
	{
		CameraProjection_Perspective,
		CameraProjection_Orthographic,
		CameraProjection_Max
	};

	// Describes a 3D transform using Euler rotations
	struct Transform
	{
		bool operator==(const Transform& other)
		{
			return x == other.x && y == other.y && z == other.z &&
				scaleX == other.scaleX && y == other.scaleY && scaleZ == other.scaleZ &&
				yaw == other.yaw && pitch == other.pitch && roll == other.roll;
		}
		float32 x, y, z;
		float32 scaleX, scaleY, scaleZ;
		float32 yaw, pitch, roll;
	};
	// Same as Transform, but using quaternions rather than Euler
	// I personally prefer Euler as it's more straight forward but this is unfortunately necessary for skeletal animation :|
	struct TransformQuat
	{
		bool operator==(const TransformQuat& other)
		{
			return x == other.x && y == other.y && z == other.z &&
				scaleX == other.scaleX && y == other.scaleY && scaleZ == other.scaleZ &&
				quatW == other.quatW && quatX == other.quatX && quatY == other.quatY && quatZ == other.quatZ;
		}
		float32 x, y, z;
		float32 scaleX, scaleY, scaleZ;
		float32 quatW, quatX, quatY, quatZ;
	};

	struct MeshProps
	{
		Transform transform;
		MeshFiltering filtering = MeshFiltering_Bilinear;
		MeshAddressing addressMode = MeshAddressing_Wrap;
		bool wireframe = false;
		MeshCull cull = MeshCull_Back;
	};

	struct AnimationProps
	{
		bool loop = false;
		FrameInterpolation frameInterp = FrameInterpolation_Always;
		float64 animSpeed = 1.0;
		float64 animInterpSpeed = 0.2;
	};

	struct LightingProps
	{
		uint32 ambient = 0;
		uint32 diffuse = 0;
		uint32 fogColor = 0;
		float32 dirX = 0.0f, dirY = 0.0f, dirZ = 0.0f;
		float32 fogDist = 0.0f;
	};

	struct CameraProps
	{
		CameraProjection proj = CameraProjection_Perspective;
		float32 fov = 0.0f;
		float32 nearClip = 0.0f;
		float32 farClip = 0.0f;
		float32 x = 0.0f, y = 0.0f, z = 0.0f;
		float32 yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	};

	// Vertex data
	struct Vertex
	{
		float32 x, y, z;
		float32 normX, normY, normZ;
		float32 u, v;
		float32 boneIDs[MEX_MAX_BONE_INFLUENCE];
		float32 boneWeights[MEX_MAX_BONE_INFLUENCE];
	};

	// Row-major 4x4 matrix
	struct Matrix
	{
		float32 rows[4][4];
	};

	// Describes an individual bone in a mesh
	struct Bone
	{
		wchar name[MEX_MAX_NAME_CHARS + 1];
		int32 index;

		uint32 spaceTransformIndex;

		TransformQuat transform;
		TransformQuat interpTransform;
		TransformQuat prevInterpTransform;
		TransformQuat finalTransform;

		// An array of indices into other bones
		uint8 children[MEX_MAX_BONES];
		uint32 numChildren;
	};

	// This is used to seperate draw calls for faces with different materials
	struct Primitive
	{
		uint32 textureID = 0;
		std::vector<uint32> indices;

#ifdef _WIN32
		union
		{
			IDirect3DIndexBuffer9* d3d9IBuffer = nullptr;
			ID3D11Buffer* d3d11IBuffer;
		};
#endif
	};

	// Subset in a mesh file (each seperate object in an .obj/fbx)
	struct Subset
	{
		wchar name[MEX_MAX_NAME_CHARS + 1];
		int32 index;
		Transform transform;
		bool animated;

		std::vector<Vertex> vertices;
		std::vector<Primitive> primitives;
		Matrix meshToBone[MEX_MAX_BONES];

		// Index 0 = min, index 1 = max
		float32 minMaxX[2];
		float32 minMaxY[2];
		float32 minMaxZ[2];

		float32 aabbX[8];
		float32 aabbY[8];
		float32 aabbZ[8];

		uint32 parentBone;
		std::vector<uint32> children;

#ifdef _WIN32
		union
		{
			IDirect3DVertexBuffer9* d3d9VBuffer = nullptr;
			ID3D11Buffer* d3d11VBuffer;
		};
#endif
	};

	// Individual animation frame
	struct AnimFrame
	{
		// Transform data of each bone
		std::vector<TransformQuat> bones;
	};

	// Stores data regarding a single clip of animation
	struct AnimClip
	{
		wchar name[MEX_MAX_NAME_CHARS + 1];
		int32 index;
		std::vector<AnimFrame> frames;
	};

	// Loaded mesh file
	struct Mesh
	{
		wchar fileName[MEX_MAX_FILENAME_CHARS + 1];
		int32 index;

		std::vector<Subset> subsets;
		// Indices into child-less subsets
		std::vector<uint32> rootSubsets;

		std::vector<Bone> bones;
		// Indices into child-less bones
		std::vector<uint32> rootBones;

		// Space transforms, taken from armature nodes (ufbx empties)
		std::vector<TransformQuat> spaceTransforms;

		std::vector<AnimClip> anims;

		float64 animFPS = 0.0f;
		// Does the mesh's GPU resources need to be recreated? (think D3D9 device loss)
		bool needReset = false;

		GlobalContext* globalCtx = nullptr;

		bool ResetDriverResources();
		void ReleaseDriverResources();
	};

	// Loaded texture
	struct Texture
	{
		// Filename of this texture, only valid if imageID is not MEX_UNDEFINED16
		wchar fileName[MEX_MAX_FILENAME_CHARS + 1];
		// Fusion image bank ID
		uint16 imageID;
#ifdef WIN32
		// HWA surface of this texture
		std::shared_ptr<cSurface> surf;
#endif
	};

	// Runtime global context holding shared data across all instances of the extension
	class GlobalContext
	{
	public:
		~GlobalContext();

		// Initialize global context
		// This is where global graphics API data is stored
		bool InitDriver();
		// Initializes graphics API state
		bool ResetDriver();
		// Frees graphics API state
		void ReleaseDriverResources();

		// Load a mesh from a filename, and returns an ID (the index) to it
		// If this exact filename has already been loaded, the existing index is returned
		uint32 LoadMesh(const wchar* fileName);
		// Frees a loaded mesh
		void ReleaseMesh(uint32 meshID);

		// Creates a Fusion surface for an image bank texture
		bool CreateTextureSurfaceFromImageBank(void* fusionApp, Texture& texture);
		// Creates a Fusion surface for a file texture
		bool CreateTextureSurfaceFromFile(RunHeader* rhPtr, Texture& texture);
		// Add a texture to this context
		uint32 AddTexture(const Texture& texture);

		// Returns the number of meshes currently loaded in this context
		uint32 GetNumMeshes();
		// Returns a temporary reference to a mesh
		Mesh& GetMesh(uint32 index);

		// Returns the number of textures currently loaded in this context
		uint32 GetNumTextures();
		// Returns a temporary reference to a texture
		Texture& GetTexture(uint32 index);

		bool TextureExists(const Texture& texture, uint32& outID);

		// Does the graphics driver need to be reset? (think D3D9 device loss)
		bool NeedReset();
	private:
		bool inited = false;
		bool needReset = false;
		std::vector<Mesh> meshes;
		std::vector<Texture> textures;
	public:
#ifdef _WIN32
		SurfaceDriver driverType = SurfaceDriver::DIB;
		// Should always be set to Fusion's device
		union
		{
			IDirect3DDevice9* d3d9Device = nullptr;
			ID3D11Device* d3d11Device;
		};
		IDirect3DStateBlock9* d3d9SB = nullptr;
		ID3D11DeviceContext* d3d11Ctx = nullptr;
		ID3D11Buffer* d3d11VCBuffer = nullptr;
		ID3D11Buffer* d3d11PCBuffer = nullptr;
		union
		{
			IDirect3DVertexDeclaration9* d3d9VDec = nullptr;
			ID3D11InputLayout* d3d11ILayout;
		};
		ID3D11DepthStencilState* d3d11DSS = nullptr;
		// Stores bone matrices packed into a texture
		// This is more efficient than naively storing an array of float4x4s in the shader
		union
		{
			IDirect3DTexture9* d3d9DeformTex = nullptr;
			ID3D11Texture2D* d3d11DeformTex;
		};
		ID3D11ShaderResourceView* d3d11DeformSRV = nullptr;
		union
		{
			IDirect3DVertexShader9* d3d9StaticShader = nullptr;
			ID3D11VertexShader* d3d11StaticShader;
		};
		union
		{
			IDirect3DVertexShader9* d3d9SkinningShader = nullptr;
			ID3D11VertexShader* d3d11SkinningShader;
		};
		union
		{
			IDirect3DPixelShader9* d3d9TexShader = nullptr;
			ID3D11PixelShader* d3d11TexShader;
		};
		union
		{
			IDirect3DPixelShader9* d3d9ColorShader = nullptr;
			ID3D11PixelShader* d3d11ColorShader;
		};
		ID3D11SamplerState* d3d11PointClampSampler = nullptr;
		ID3D11BlendState* d3d11BState = nullptr;
#endif
	};

	// Render context for each individual mesh instance
	class RenderContext
	{
	public:
		RenderContext();
		~RenderContext();
		// Initializes this render instance. Make sure global data is set first!
		bool InitDriver(int32 renderWidth, int32 renderHeight);

		// Reads all properties *except* the mesh filename
		void ReadProperties(const EDITDATA* edPtr);

		void CreateSurface(int32 renderWidth, int32 renderHeight);
		void DrawToSurface();

		bool SetMesh(uint32 meshID);
		void UpdateMesh(float64 baseFPS, float64 mvtTimerCoef);
		void LoadTextures(void* fusionApp, const EDITDATA* edPtr);
		void ReplaceTexture(RunHeader* rhPtr, int32 textureID, const wchar* fileName);

		void PlayAnim();
		void StopAnim();

		void SetCurrentAnim(int32 anim);
		void SetCurrentAnimFrame(float64 frame);

		void SetGlobalContext(GlobalContext* globalCtx);

		// Returns a temporary pointer to the currently loaded mesh, or null if no mesh is loaded
		Mesh* GetMesh();

		int32 GetNumSubsets();
		// Returns a temporary pointer to a subset by index
		Subset* GetSubset(int32 index);
		// Returns a temporary pointer to a subset by name
		Subset* GetSubset(const wchar* name);
		// Returns a temporary pointer to a subset's transform
		Transform* GetLocalSubsetTransform(const Subset* subset);
		// Returns a temporary pointer to a subset's frustum-culling radius
		float32* GetLocalSubsetAABB(const Subset* subset);

		int32 GetNumAnims();
		int32 GetCurrentAnim();
		float64 GetCurrentAnimFrame();
		float64 GetAnimFPS();
		// Returns a temporary pointer to an animation by index
		AnimClip* GetAnim(int32 index);
		// Returns a temporary pointer to an animation by name
		AnimClip* GetAnim(const wchar* name);

		int32 GetNumBones();
		Bone* GetBone(int32 index);
		Bone* GetBone(const wchar* name);
		void GetProjectedBonePos(int32 boneID, int32 subsetID, int32& outX, int32& outY);

		DarkEdif::CollisionMask* GetCollisionMask();
		int32 GetSurfaceWidth();
		int32 GetSurfaceHeight();

#ifdef _WIN32
		cSurface* GetSurface();
#endif
		GlobalContext* GetGlobalContext();

		bool IsInited();
		bool IsMeshLoaded();
		// Rendering off-screen is needed if the collision mask is needed. The mesh could be off-screen if collision checks are done so we need to account for that
		bool IsRenderingOffscreen();

		bool IsAnimPlaying();

		void OnDeviceLoss();

		MeshProps meshProps;
		AnimationProps animProps;
		LightingProps lightingProps;
		CameraProps cameraProps;
	private:
		static Matrix identityMatrix;

		void WorldViewProj(Matrix& out, Matrix& outNormal);
		void UpdateBoneTransforms(Mesh& mesh, uint32 boneID, Matrix parentTransform, Matrix parentNormal);
		void UpdateSubsetDeforms(Mesh& mesh, const Subset& subset, uint32 boneID);
		void DrawSubset(Mesh& mesh, uint32 subsetID, Matrix parentTransform, Matrix parentNormal);
		void PlatformDrawSubset(Mesh& mesh, const Subset& subset, const Matrix& mat, const Matrix& matNormal);
		// Rudimentary frustum-culling check
		bool SubsetInsideFrustum(Mesh& mesh, const Subset& subset, const Matrix& finalMat);
		void PlatformCreateSurface();
		void ReleaseDriverResources();
		// Writes matrix data packed into a texture, the way this works is that each pixel is a single row in a matrix
		void WriteBoneDeformTexture(Mesh& mesh, float32* dest, uint32 pitch);
		void UpdateCollisionMask();
		// Writes a mask buffer suitable for sMask
		// While we could just use CreateMask to generate one for us, that isn't implemented for HWA surfaces, even though I don't think there is any reason for it not to be, therefore necessitating a custom implementation of it
		// srcData is assumed to be RGBA32 data
		void WriteMaskData(uint16* destMask, const uint32* srcData, uint32 destPitchBytes, uint32 srcPitchBytes);
		// Convert an X coordinate ranging from -1.0:1.0 to an integer from 0:width
		inline int32 ScreenSpaceX(float32 x);
		// Convert a Y coordinate ranging from -1.0:1.0 to an integer from 0:height
		inline int32 ScreenSpaceY(float32 y);

		bool inited = false;
		bool meshLoaded = false;
		bool renderOffscreen = false;

		// Currently loaded mesh ID
		uint32 meshID = MEX_UNDEFINED;
		std::vector<uint32> textureIDs;

		std::vector<Transform> subsetTransforms;
		std::vector<float32> subsetAABBs;

		bool animPlaying = false;
		int32 currentAnim = 0;
		float64 currentFrame = 0.0f;
		// Used on "Always" frame interpolation
		float64 currentAnimInterp = 0.0;

		// World-view-projection matrices
		Matrix wvp, wNormal;
		// Bone transformation matrices
		Matrix boneTransforms[MEX_MAX_BONES];
		Matrix boneTransformsNormal[MEX_MAX_BONES];
		// Final bone skinning matrices
		Matrix currentSubsetDeforms[MEX_MAX_BONES];

		int32 width = 0;
		int32 height = 0;

		GlobalContext* globalCtx = nullptr;
#ifdef _WIN32
		cSurface surf;
		union
		{
			IDirect3DSurface9* d3d9DepthBuffer = nullptr;
			ID3D11DepthStencilView* d3d11DepthBuffer;
		};
		ID3D11Texture2D* d3d11DepthBufferTex = nullptr;
		union
		{
			IDirect3DSurface9* d3d9CMask = nullptr;
			ID3D11Texture2D* d3d11CMask;
		};
		ID3D11SamplerState* d3d11Sampler = nullptr;
		ID3D11RasterizerState* d3d11RState = nullptr;

		MeshFiltering prevFiltering = MeshFiltering_Bilinear;
		MeshAddressing prevAddressMode = MeshAddressing_Wrap;

		bool prevWireframe = false;
		MeshCull prevCull = MeshCull_Back;
#endif
		std::unique_ptr<DarkEdif::CollisionMask> cMask;
		// Used to determine whether the collision mask should be updated
		// This is set every time the mesh is rendered
		bool updateCMask = true;
	};

	void PlatformConvertMatrix(Matrix matrix, Matrix& out);

	// MATRICES:
	// -----
	void MatrixIdentity(Matrix& out);
	void MatrixTranslation(float32 x, float32 y, float32 z, Matrix& out);
	void MatrixYaw(float32 angle, Matrix& out);
	void MatrixPitch(float32 angle, Matrix& out);
	void MatrixRoll(float32 angle, Matrix& out);
	void MatrixQuaternion(float32 w, float32 x, float32 y, float32 z, Matrix& out);
	void MatrixScaling(float32 x, float32 y, float32 z, Matrix& out);
	void MatrixTransform(const Transform& transform, Matrix& out, Matrix* outNormal = nullptr);
	void MatrixTransform(const TransformQuat& transform, Matrix& out, Matrix* outNormal = nullptr);

	void MatrixPerspective(float32 fov, float32 aspectRatio, float32 nearClip, float32 farClip, Matrix& out);
	void MatrixOrthographic(float32 fov, float32 aspectRatio, float32 nearClip, float32 farClip, Matrix& out);

	void MatrixMultiply(Matrix first, Matrix second, Matrix& out);
	void MatrixMultiply(Matrix first, Vertex second, float32& outX, float32& outY, float32& outZ, float32& outW);
	void MatrixMultiply(Matrix first, float32 secondX, float32 secondY, float32 secondZ, float32 secondW, float32& outX, float32& outY, float32& outZ, float32& outW);
	// -----

	// MATH:
	// -----
	inline float32 Sin(float32 degrees);
	inline float32 Cos(float32 degrees);
	inline float32 Tan(float32 degrees);

	inline float32 Lerp(float32 a, float32 b, float32 t);
	inline float32 LerpAngle(float32 a, float32 b, float32 t);
	inline float32 Slerp(float32 a, float32 b, float32 t);
	// -----

	// FILE API:
	// -----
	bool FileOpen(const wchar* fileName, File& outFile);
	void FileClose(File file);

	uint32 FileRead(File file, void* buffer, uint32 bufferSize);
	uint32 FileSeek(File file, uint32 offset, FileDirection direction);
	uint64 FileSize(File file);
	// -----
}
