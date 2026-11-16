#include "Common.hpp"
#include <iomanip>
using namespace std::chrono_literals;

#if EditorBuild
// Upgrades EDITDATA; called only if there is data between eHeader and Props
bool EDITDATA::MigrateMiddle(mv* mV, const EDITDATA* const oldEdPtr, EDITDATA* const newEdPtr)
{
	// In this function, data between eHeader and font/Props must be copied from oldEdPtr to newEdPtr.
	// Note that if you've modified EDITDATA layout between versions,
	// then note that EDITDATA is not a valid pointer for oldEdPtr.
	// You will have to create an old EDITDATA struct and cast oldEdPtr to it.
	const int oldExtVer = (oldEdPtr->eHeader.extVersion & ~DarkEdif::Properties::AllVersionFlags);

	// oldEdPtr is not necessarily a valid EDITDATA pointer, as EDITDATA is your CURRENT version EDITDATA,
	// not your MFA saved old version.
	// If you added variables between oldExtVer and Extension::Version,
	// you must init them.
	// If you rearranged them between vars, you must find the old offset and copy them.
	// If you didn't move or rearrange anything, a straight memcpy() is normally fine:
	// just don't overwrite newEdPtr->eHeader or newEdPtr->Props.

#if DARKEDIF_DISPLAY_TYPE > DARKEDIF_DISPLAY_ANIMATIONS
	// Check what offset the objSize is in in your old data, it does not auto-copy it.
	newEdPtr->objSize = oldEdPtr->objSize;
#endif
	return true;
}
#endif // EditorBuild

///
/// EXTENSION CONSTRUCTOR/DESTRUCTOR
///

#ifdef _WIN32
Extension::Extension(RunObject* const _rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr) :
	rdPtr(_rdPtr), rhPtr(_rdPtr->get_rHo()->get_AdRunHeader()), Runtime(this), FusionDebugger(this)
#elif defined(__ANDROID__)
Extension::Extension(const EDITDATA* const edPtr, const jobject javaExtPtr, const CreateObjectInfo* const cobPtr) :
	javaExtPtr(javaExtPtr, "Extension::javaExtPtr from Extension ctor"),
	Runtime(this, this->javaExtPtr), FusionDebugger(this)
#else
Extension::Extension(const EDITDATA* const edPtr, void* const objCExtPtr, const CreateObjectInfo* const cobPtr) :
	objCExtPtr(objCExtPtr), Runtime(this, objCExtPtr), FusionDebugger(this)
#endif
{
	/*
		Link all your action/condition/expression functions to their IDs to match the
		IDs in the JSON here
	*/

	LinkAction(0, SetObjectSize);
	LinkAction(1, LoadMesh);
	LinkAction(2, ReplaceTexture);

	LinkAction(3, SetYaw);
	LinkAction(4, SetPitch);
	LinkAction(5, SetRoll);

	LinkAction(6, SetScaleX);
	LinkAction(7, SetScaleY);
	LinkAction(8, SetScaleZ);

	LinkAction(9, SetTexFilter);
	LinkAction(10, SetUVAddress);

	LinkAction(11, SetWireframe);
	LinkAction(12, SetCullMode);

	LinkAction(13, SetSubsetPosX);
	LinkAction(14, SetSubsetPosY);
	LinkAction(15, SetSubsetPosZ);

	LinkAction(16, SetSubsetYaw);
	LinkAction(17, SetSubsetPitch);
	LinkAction(18, SetSubsetRoll);

	LinkAction(19, SetSubsetScaleX);
	LinkAction(20, SetSubsetScaleY);
	LinkAction(21, SetSubsetScaleZ);


	LinkAction(22, PlayAnim);
	LinkAction(23, StopAnim);

	LinkAction(24, SetLoop);
	LinkAction(25, SetFrameInterp);

	LinkAction(26, SetAnim);
	LinkAction(27, SetAnimSpeed);
	LinkAction(28, SetAnimFrame);


	LinkAction(29, SetAmbientColor);
	LinkAction(30, SetDiffuseColor);
	LinkAction(31, SetFogColor);

	LinkAction(32, SetLightDirX);
	LinkAction(33, SetLightDirY);
	LinkAction(34, SetLightDirZ);

	LinkAction(35, SetFogDistance);

	LinkAction(36, SetProjection);
	LinkAction(37, SetFOV);
	LinkAction(38, SetNearClip);
	LinkAction(39, SetFarClip);

	LinkAction(40, SetCameraPosX);
	LinkAction(41, SetCameraPosY);
	LinkAction(42, SetCameraPosZ);

	LinkAction(43, SetCameraYaw);
	LinkAction(44, SetCameraPitch);
	LinkAction(45, SetCameraRoll);


	LinkAction(46, SetAnimInterpSpeed);

	LinkAction(47, SetSubsetAABBRadius);


	LinkCondition(0, ImmediateCondition);
	LinkCondition(1, ImmediateCondition);

	LinkCondition(2, ImmediateCondition);
	LinkCondition(3, ImmediateCondition);


	LinkCondition(4, IsMeshLoaded);

	LinkCondition(5, UsingPointFiltering);
	LinkCondition(6, UsingBilinearFiltering);

	LinkCondition(7, UsingWrapUV);
	LinkCondition(8, UsingClampUV);

	LinkCondition(9, IsWireframe);

	LinkCondition(10, UsingBackFace);
	LinkCondition(11, UsingFrontFace);
	LinkCondition(12, UsingNoneFace);

	LinkCondition(13, IsAnimLooping);
	LinkCondition(14, UsingAlwaysInterp);
	LinkCondition(15, UsingSometimesInterp);
	LinkCondition(16, UsingNeverInterp);

	LinkCondition(17, UsingPerspective);


	LinkExpression(0, GetWidth);
	LinkExpression(1, GetHeight);

	LinkExpression(2, GetYaw);
	LinkExpression(3, GetPitch);
	LinkExpression(4, GetRoll);

	LinkExpression(5, GetScaleX);
	LinkExpression(6, GetScaleY);
	LinkExpression(7, GetScaleZ);

	LinkExpression(8, GetSubsetPosX);
	LinkExpression(9, GetSubsetPosY);
	LinkExpression(10, GetSubsetPosZ);

	LinkExpression(11, GetSubsetYaw);
	LinkExpression(12, GetSubsetPitch);
	LinkExpression(13, GetSubsetRoll);

	LinkExpression(14, GetSubsetScaleX);
	LinkExpression(15, GetSubsetScaleY);
	LinkExpression(16, GetSubsetScaleZ);

	LinkExpression(17, GetNumSubsets);
	LinkExpression(18, GetSubsetIndex);
	LinkExpression(19, GetSubsetName);


	LinkExpression(20, GetCurrentAnimIndex);
	LinkExpression(21, GetAnimSpeed);
	LinkExpression(22, GetCurrentAnimFrame);

	LinkExpression(23, GetNumAnims);
	LinkExpression(24, GetAnimIndex);
	LinkExpression(25, GetAnimName);
	LinkExpression(26, GetAnimFPS);

	LinkExpression(27, GetNumBones);
	LinkExpression(28, GetBoneIndex);
	LinkExpression(29, GetBoneName);
	LinkExpression(30, GetProjectedBoneX);
	LinkExpression(31, GetProjectedBoneY);


	LinkExpression(32, GetAmbientColor);
	LinkExpression(33, GetDiffuseColor);
	LinkExpression(34, GetFogColor);

	LinkExpression(35, GetLightDirX);
	LinkExpression(36, GetLightDirY);
	LinkExpression(37, GetLightDirZ);

	LinkExpression(38, GetFogDistance);


	LinkExpression(39, GetFOV);
	LinkExpression(40, GetNearClip);
	LinkExpression(41, GetFarClip);

	LinkExpression(42, GetCameraPosX);
	LinkExpression(43, GetCameraPosY);
	LinkExpression(44, GetCameraPosZ);

	LinkExpression(45, GetCameraYaw);
	LinkExpression(46, GetCameraPitch);
	LinkExpression(47, GetCameraRoll);


	LinkExpression(48, GetAnimInterpSpeed);

	LinkExpression(49, GetSubsetAABBRadius);

	LinkExpression(50, GetMeshFileName);

	/*
		This is where you'd do anything you'd do in CreateRunObject in the original SDK

		It's the only place you'll get access to edPtr at runtime, so you should transfer
		anything from edPtr to the extension class here.

	*/

	void* pApp;
	MeshEx::GlobalContext* globalCtx;
#ifdef _WIN32
	frameSurf = WinGetSurface((int)Edif::SDK->mV->mvIdEditWin);
	pApp = rhPtr->rhIdAppli;
	globalCtx = (MeshEx::GlobalContext*)rhPtr->rh4.rh4Mv->mvGetExtUserData(rhPtr->rhApp, hInstLib);
#else
	// TODO: whenever I get the chance to port this ext to other platforms, add platform-specific code to accomdate this
	pApp = nullptr; 
#endif
	renderCtx.SetGlobalContext(globalCtx);
	renderCtx.InitDriver(edPtr->objSize.width, edPtr->objSize.height);
	renderCtx.ReadProperties(edPtr);
	renderCtx.SetMesh(renderCtx.GetGlobalContext()->LoadMesh(edPtr->Props.GetPropertyStr(1).c_str()));
	renderCtx.LoadTextures(pApp, edPtr);

	rdPtr->get_rHo()->SetX(cobPtr->get_X());
	rdPtr->get_rHo()->SetY(cobPtr->get_Y());
	rdPtr->get_rHo()->SetImgWidth(renderCtx.GetSurfaceWidth());
	rdPtr->get_rHo()->SetImgHeight(renderCtx.GetSurfaceHeight());

	// Don't use "this" inside these lambda functions, always ext.
	// There can be nothing in the [] section of the lambda.
	// If you're not sure about lambdas, you can remove this debugger stuff without any side effects;
	// it's just an example of how to use the debugger. You can view it in Fusion itself to see.
	FusionDebugger.AddItemToDebugger(
		// Prefix before debugger value, and initial text; if we pass null for initial text, it uses reader func
		_T("Current mesh: "sv), NULL,
		// reader function for your debug item
		[](Extension* const ext, std::tstring& writeTo) {
			auto pMesh = ext->renderCtx.GetMesh();
			writeTo = pMesh ? pMesh->fileName : _T("(no mesh loaded)");
		},
		// writer function (can be null if you don't want user to be able to edit it in debugger)
		NULL,
		500, NULL
	);

	// Read object DarkEdif properties; you can pass property name, or property index
	// This will work on all platforms the same way.
	// See edPtr->Props functions.
	//bool checkboxWithinFolder = edPtr->Props.IsPropChecked("Checkbox within folder"sv);
	//std::tstring editable6Text = edPtr->Props.GetPropertyStr("Editable 6"sv);

	// These lines do nothing, but prevent the compiler warning the variables are unused

#if TEXT_OEFLAG_EXTENSION
	// Copy from edittime data into runtime data
	font.CopyFromEditFont(this, edPtr->font);

	// Set Runtime.fontChangedFunc to trigger a function if the runtime
	// changes your ext's font via the built-in font actions
	Runtime.fontChangedFunc = &Extension::OnFontChanged;
#endif

#if DARKEDIF_DISPLAY_TYPE == DARKEDIF_DISPLAY_SIMPLE
	Runtime.SetSurfaceWithSize(edPtr->objSize.width, edPtr->objSize.height);
	// surf is already constructed
	// To start, let's make the image a solid blue fill
	surf->FillImageWith(DarkEdif::SurfaceFill::Solid(DarkEdif::ColorRGB(0, 0, 127)));
#endif
}

Extension::~Extension()
{

}


// Runs every tick of Fusion's runtime, can be toggled off and back on
REFLAG Extension::Handle()
{
	/*	If your extension won't draw to the window, but it still needs
		to do something every Fusion loop, use:
			return REFLAG::NONE;

		If you don't need to do something in Handle anymore, use:
			return REFLAG::ONE_SHOT;
		...you can later re-enable Handle with Runtime.Rehandle(),
		but don't use one-shot with a displaying ext.

		If you're drawing with simple display, use:
			return surf->GetAndResetAltered() ? REFLAG::DISPLAY : REFLAG::NONE;

		If you're drawing manually, use rdPtr->get_roc()->get_changed(),
		and potentially your surfaces GetAndResetAltered(), to decide
		what REFLAG to return.
		DarkEdif expects you to do manual drawing in Extension::Display()
		and other funcs.
	*/

#if DARKEDIF_DISPLAY_TYPE >= DARKEDIF_DISPLAY_SIMPLE
	// Example variables for showing how display can be modified.
	// These values should be in your Extension, not static; this is just a brief demo,
	// to show how to easily time your ext's display changes.
	// Display does not have to be done in Handle, but you do need to return
	// REFLAG::DISPLAY here and/or do rdPtr->get_roc()->set_changed(true) when display is changed.
	/*static std::chrono::steady_clock clock;
	static decltype(clock)::time_point nextTick;
	static const std::uint32_t colors[] = {
		DarkEdif::ColorRGB(128, 0, 0), DarkEdif::ColorRGB(168, 157, 50),
		DarkEdif::ColorRGB(50, 168, 64), DarkEdif::ColorRGB(50, 54, 168)
	};
	static std::size_t colorIndex = 0;
	if (nextTick < clock.now())
	{
		nextTick = clock.now() + 200ms;
		if (++colorIndex >= std::size(colors))
			colorIndex = 0;
		surf.FillImageWith(DarkEdif::SurfaceFill::Solid(colors[colorIndex]));
	}
	return surf.GetAndResetAltered() ? REFLAG::DISPLAY : REFLAG::NONE;*/
#endif

	if (renderCtx.InitDriver(renderCtx.GetSurfaceWidth(), renderCtx.GetSurfaceHeight()))
	{
		Runtime.GenerateEvent(0); // pre-update event
		float64 baseFPS, delta;
#ifdef _WIN32
		baseFPS = (float64)rhPtr->rhFrame->MvtTimerBase;
		delta = rdPtr->get_rHo()->get_AdRunHeader()->rh4.rh4MvtTimerCoef;
#endif
		renderCtx.UpdateMesh(baseFPS, delta);
		Runtime.GenerateEvent(1); // post-update event

		if (renderCtx.IsRenderingOffscreen())
		{
			Runtime.GenerateEvent(2); // pre-draw event
			renderCtx.DrawToSurface();
			Runtime.GenerateEvent(3); // post-draw event
		}
		return REFLAG::DISPLAY;
	}
	else
	{
		return REFLAG::NONE;
	}
}

#if PAUSABLE_EXTENSION
// Called when Fusion runtime is pausing - not just the F3 pause dialog
void Extension::FusionRuntimePaused()
{

}

// Called when Fusion runtime is resuming after a pause
void Extension::FusionRuntimeContinued()
{

}
#endif // PAUSABLE_EXTENSION

#if TEXT_OEFLAG_EXTENSION
/* Triggered when the runtime changes the font name or size via built-in actions
 * @param colorEdit If true, font color was edited. If false, the font typeface was.
 * @param rect		If not null, contains a hint as to what size the object should be.
 *					This is based on differences between original font size and current one.
 *					You can ignore this hint, but it indicates you may want to call
 *					rdPtr->get_roc()->SetSize(). */
void Extension::OnFontChanged(bool colorEdit, DarkEdif::Rect * rect)
{
	// Prevent warnings about unused variables
	(void)colorEdit;
	(void)rect;

	// We could call Runtime.Redraw() to redraw ext immediately,
	// but we'll be patient and wait for next event tick
	rdPtr->get_roc()->set_changed(true);
	Runtime.Rehandle();
}

#endif

#if DARKEDIF_DISPLAY_TYPE == DARKEDIF_DISPLAY_MANUAL

void Extension::Display()
{
	// You should manually draw the surface(s) here.
	// Surface::BlitToFrameWithExtEffects will let you draw with the ext's sprites/X/Y accounted for,
	// but note you should also manage the collision mask in GetCollisionMask().
	// If you have a single surface, and don't want simple drawing, use the GetDisplaySurface().
	//
	//surf->BlitToFrameWithExtEffects(this, Point { x offset, y offset });

	if (renderCtx.IsInited())
	{
		if (!renderCtx.IsRenderingOffscreen())
		{
			Runtime.GenerateEvent(2); // pre-draw event
			renderCtx.DrawToSurface();
			Runtime.GenerateEvent(3); // post-draw event
		}

		auto hoPtr = rdPtr->get_rHo();
		auto rhPtr = hoPtr->get_AdRunHeader();
		auto rs = rdPtr->get_ros();
		auto x = hoPtr->get_X() - rhPtr->get_WindowX();
		auto y = hoPtr->get_Y() - rhPtr->get_WindowY();
#ifdef _WIN32
		auto bMode = (uint32)rs->get_Effect() & (uint32)BlitOperation::Transparent ? BMODE_TRANSP : BMODE_OPAQUE;
		auto bOp = (BlitOp)(rs->get_Effect() & BlitOperation::EffectMask);
		auto effectParam = rdPtr->get_ros()->get_EffectParam();
		renderCtx.GetSurface()->Blit(*frameSurf, x, y, bMode, bOp, effectParam);
#endif
	}
}


void Extension::GetZoneInfos()
{
	// LOGI(_T("##### Extension::GetZoneInfos() called.\n"));
	// You can safely do nothing here. Some extensions do:
	// rdPtr->get_rHo()->set_ImgWidth()/set_ImgHeight()
	// but it is done automatically in CF2.5, so shouldn't be necessary.
}

DarkEdif::Surface * Extension::GetDisplaySurface()
{
	// Old style MMF2 fade-in/fade-out transistions requires this function,
	// Auto-rendering of Sprite effects (alpha coeff etc) will be applied by returning a surface here.
	// If you have multiple surfaces, you should can return null here and draw them yourself.
	// If you return null here, Display() will be called instead.
	return nullptr;
}
DarkEdif::CollisionMask * Extension::GetCollisionMask(std::uint32_t flags)
{
	// If your ext supports fine collision, you should return the collision mask corresponding to
	// which pixels have collision and which don't. DarkEdif::Surface can generate collision masks,
	// but it can be computationally expensive.
	// If your ext uses only box collision, DE will detect that and never call this function.
	(void)flags;

	if ((uint32)rdPtr->get_ros()->get_Effect() & (uint32)BlitOperation::Transparent)
		return renderCtx.GetCollisionMask();

	return nullptr;
}

#endif // DARKEDIF_DISPLAY_MANUAL

// These are called if there's no function linked to an ID

void Extension::UnlinkedAction(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedAction() called"), _T("Running a fallback for action ID %d. Make sure you ran LinkAction()."), ID);
}

long Extension::UnlinkedCondition(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedCondition() called"), _T("Running a fallback for condition ID %d. Make sure you ran LinkCondition()."), ID);
	return 0;
}

long Extension::UnlinkedExpression(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedExpression() called"), _T("Running a fallback for expression ID %d. Make sure you ran LinkExpression()."), ID);
	// Unlinked A/C/E is fatal error, but try not to return null string and definitely crash it
	if ((std::size_t)ID < Edif::SDK->ExpressionInfos.size() && Edif::SDK->ExpressionInfos[ID]->Flags.ef == ExpReturnType::String)
		return (long)Runtime.CopyString(_T(""));
	return 0;
}
