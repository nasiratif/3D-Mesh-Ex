#pragma once
#include <cctype>
#include "DarkEdif.hpp"
#include "MeshEx.hpp"

class Extension final
{
public:
	// ======================================
	// Required variables + functions
	// Variables here must not be moved or swapped around or it can cause future issues
	// ======================================
	RunHeader* rhPtr;
	RunObjectMultiPlatPtr rdPtr;
#ifdef __ANDROID__
	global<jobject> javaExtPtr;
#elif defined(__APPLE__)
	void* const objCExtPtr;
#endif

	Edif::Runtime Runtime;

	static const int MinimumBuild = 296;
	static const int Version = 4;

	// Warning: OEFLAGS/OEPREFS cannot be freely modified when you have used them in MFAs.
	static constexpr OEFLAGS OEFLAGS = OEFLAGS::MANUAL_DISPLAY_EXT | OEFLAGS::VALUES | OEFLAGS::MOVEMENTS | OEFLAGS::SPRITES | OEFLAGS::RUN_BEFORE_FADE_IN;
	static constexpr OEPREFS OEPREFS = OEPREFS::SCROLLING_INDEPENDENT | OEPREFS::SLEEP | OEPREFS::KILL | OEPREFS::FINE_COLLISIONS | OEPREFS::INK_EFFECTS;
	// If OEFLAGS::WINDOW_PROC (otherwise you can delete)
	// static constexpr int WindowProcPriority = 100;
	// If OEFLAGS::TEXT (otherwise you can delete)
	// static constexpr TextCapacity TextCapacity = TextCapacity::None;

#ifdef _WIN32
	Extension(RunObject* const rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr);
#elif defined(__ANDROID__)
	Extension(const EDITDATA* const edPtr, const jobject javaExtPtr, const CreateObjectInfo* const cobPtr);
#else
	Extension(const EDITDATA* const edPtr, void* const objCExtPtr, const CreateObjectInfo* const cobPtr);
#endif
	~Extension();

	// ======================================
	// Extension data
	// ======================================
	MeshEx::RenderContext renderCtx;
#ifdef _WIN32
	cSurface* frameSurf;
#endif

	// To add items to the Fusion Debugger, just uncomment this line.
	DarkEdif::FusionDebugger FusionDebugger;
	// After enabling it, you run FusionDebugger.AddItemToDebugger() inside Extension's constructor
	// As an example:
	std::tstring exampleDebuggerTextItem;


	/*  Add any data you want to store in your extension to this class
		(eg. what you'd normally store in rdPtr in old SDKs).

		Unlike rdPtr, you can store real C++ objects with constructors
		and destructors, without having to call them manually or store
		a pointer.
	*/

	// int MyVariable;


	/*  Add your actions, conditions and expressions as real class member
		functions here. The arguments (and return type for expressions) must
		match EXACTLY what you defined in the JSON.

		Remember to link the actions, conditions and expressions to their
		numeric IDs in the class constructor (Extension.cpp)
	*/

	// Actions

	void SetObjectSize(int width, int height);
	void LoadMesh(const TCHAR* fileName);
	void ReplaceTexture(int textureID, const TCHAR* fileName);

	void SetYaw(float yaw);
	void SetPitch(float pitch);
	void SetRoll(float roll);

	void SetScaleX(float x);
	void SetScaleY(float y);
	void SetScaleZ(float z);

	void SetTexFilter(int filter);
	void SetUVAddress(int mode);

	void SetWireframe(int wireframe);
	void SetCullMode(int mode);

	void SetSubsetPosX(int subset, float x);
	void SetSubsetPosY(int subset, float y);
	void SetSubsetPosZ(int subset, float z);

	void SetSubsetYaw(int subset, float yaw);
	void SetSubsetPitch(int subset, float pitch);
	void SetSubsetRoll(int subset, float roll);

	void SetSubsetScaleX(int subset, float x);
	void SetSubsetScaleY(int subset, float y);
	void SetSubsetScaleZ(int subset, float z);


	void PlayAnim();
	void StopAnim();

	void SetLoop(int loop);
	void SetFrameInterp(int mode);

	void SetAnim(int anim);
	void SetAnimSpeed(float speed);
	void SetAnimFrame(float frame);


	void SetAmbientColor(unsigned int color);
	void SetDiffuseColor(unsigned int color);
	void SetFogColor(unsigned int color);

	void SetLightDirX(float x);
	void SetLightDirY(float y);
	void SetLightDirZ(float z);

	void SetFogDistance(float dist);


	void SetProjection(int projection);

	void SetFOV(float fov);
	void SetNearClip(float nearClip);
	void SetFarClip(float farClip);

	void SetCameraPosX(float x);
	void SetCameraPosY(float y);
	void SetCameraPosZ(float z);

	void SetCameraYaw(float yaw);
	void SetCameraPitch(float pitch);
	void SetCameraRoll(float roll);


	void SetAnimInterpSpeed(float speed);

	void SetSubsetAABBRadius(int subset, float radius);

	// Conditions

	bool ImmediateCondition();

	bool IsMeshLoaded();

	bool UsingPointFiltering();
	bool UsingBilinearFiltering();

	bool UsingWrapUV();
	bool UsingClampUV();

	bool IsWireframe();

	bool UsingBackFace();
	bool UsingFrontFace();
	bool UsingNoneFace();

	bool IsAnimLooping();
	bool UsingAlwaysInterp();
	bool UsingSometimesInterp();
	bool UsingNeverInterp();

	bool UsingPerspective();

	// Expressions

	int GetWidth();
	int GetHeight();

	float GetYaw();
	float GetPitch();
	float GetRoll();

	float GetScaleX();
	float GetScaleY();
	float GetScaleZ();

	float GetSubsetPosX(int subset);
	float GetSubsetPosY(int subset);
	float GetSubsetPosZ(int subset);

	float GetSubsetYaw(int subset);
	float GetSubsetPitch(int subset);
	float GetSubsetRoll(int subset);

	float GetSubsetScaleX(int subset);
	float GetSubsetScaleY(int subset);
	float GetSubsetScaleZ(int subset);

	int GetNumSubsets();
	int GetSubsetIndex(const wchar* name);
	const TCHAR* GetSubsetName(int subset);


	int GetCurrentAnimIndex();
	float GetAnimSpeed();
	float GetCurrentAnimFrame();

	int GetNumAnims();
	int GetAnimIndex(const TCHAR* name);
	const TCHAR* GetAnimName(int anim);
	float GetAnimFPS();

	int GetNumBones();
	int GetBoneIndex(const TCHAR* name);
	const TCHAR* GetBoneName(int bone);
	int GetProjectedBoneX(int bone, int subset);
	int GetProjectedBoneY(int bone, int subset);


	int GetAmbientColor();
	int GetDiffuseColor();
	int GetFogColor();

	float GetLightDirX();
	float GetLightDirY();
	float GetLightDirZ();

	float GetFogDistance();


	float GetFOV();
	float GetNearClip();
	float GetFarClip();

	float GetCameraPosX();
	float GetCameraPosY();
	float GetCameraPosZ();

	float GetCameraYaw();
	float GetCameraPitch();
	float GetCameraRoll();


	float GetAnimInterpSpeed();

	float GetSubsetAABBRadius(int subset);

	const TCHAR* GetMeshFileName();

	// Runs every tick of Fusion's runtime, can be toggled off and back on
	REFLAG Handle();

#if TEXT_OEFLAG_EXTENSION
	// Extension text struct. Required for text exts.
	DarkEdif::FontInfoMultiPlat font;
	void OnFontChanged(bool colorEdit, DarkEdif::Rect* rc);
#endif
#if DARKEDIF_DISPLAY_TYPE == DARKEDIF_DISPLAY_SIMPLE
	// Extension display surface ptr. Required for simple display exts.
	DarkEdif::Surface * surf = nullptr;
#elif DARKEDIF_DISPLAY_TYPE == DARKEDIF_DISPLAY_MANUAL
	void Display();
	void GetZoneInfos();
	DarkEdif::Surface * GetDisplaySurface();
	DarkEdif::CollisionMask * GetCollisionMask(std::uint32_t flags);
#endif

	// These are called if there's no function linked to an ID
	void UnlinkedAction(int ID);
	long UnlinkedCondition(int ID);
	long UnlinkedExpression(int ID);

#if PAUSABLE_EXTENSION
	// Called when Fusion runtime is pausing - not just the F3 pause dialog
	void FusionRuntimePaused();
	// Called when Fusion runtime is resuming after a pause
	void FusionRuntimeContinued();
#endif // PAUSABLE_EXTENSION
};
