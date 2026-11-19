#include "Common.hpp"
#include "MeshEx.hpp"

#include <ImageFlt.hpp>

// SHADERS:
// -----
#include "Shaders\D3D9\C\Static.h"
#include "Shaders\D3D9\C\Skinned.h"
#include "Shaders\D3D9\C\Texture.h"
#include "Shaders\D3D9\C\Colored.h"

#include "Shaders\D3D11\C\Static.h"
#include "Shaders\D3D11\C\Skinned.h"
#include "Shaders\D3D11\C\Texture.h"
#include "Shaders\D3D11\C\Colored.h"
// -----

namespace MeshEx
{
	struct FileHandle
	{
		CInputBufFile pFile;
		HFILE hFile;
	};

	// 16-byte alignment necessary since DX11 constant buffers are expected to be float4-aligned
	struct alignas(16) CBuffer_Vertex
	{
		Matrix wvp;
		Matrix wNormal;
	};

	struct alignas(16) CBuffer_Pixel
	{
		float32 ambient[4];
		float32 diffuse[4];
		float32 fogColor[4];
		float32 lightDir[3];
		float32 fogDist;
	};

	// Used as a state-block equivalent because D3D11 does not support that
	struct D3D11State
	{
		ID3D11Buffer* vCBuffer;
		ID3D11Buffer* pCBuffer;
		ID3D11InputLayout* iLayout;
		ID3D11RenderTargetView* rsv;
		ID3D11DepthStencilView* dsv;
		ID3D11DepthStencilState* dss;
		UINT stencilRef;
		ID3D11VertexShader* vShader;
		ID3D11ClassInstance* vClassInstance;
		UINT vNumClassInstances;
		ID3D11PixelShader* pShader;
		ID3D11ClassInstance* pClassInstance;
		UINT pNumClassInstances;
		ID3D11Buffer* vBuffer;
		UINT vStride;
		UINT vOffset;
		ID3D11Buffer* iBuffer;
		DXGI_FORMAT iFormat;
		UINT iOffset;
		ID3D11SamplerState* vSampler;
		ID3D11SamplerState* pSampler;
		ID3D11ShaderResourceView* vSrv;
		ID3D11ShaderResourceView* pSrv;
		ID3D11RasterizerState* rState;
		ID3D11BlendState* bState;
		FLOAT bFactor[4];
		UINT bSampleMask;

		D3D11_VIEWPORT vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		UINT numVps;

		D3D11_PRIMITIVE_TOPOLOGY topology;
	};
}

using namespace MeshEx;

/*

---------- UTILITY FUNCTIONS ----------

*/

void D3D11CaptureState(ID3D11DeviceContext* d3d11Ctx, D3D11State* outState)
{
	std::memset(outState, 0x0, sizeof(*outState));
	d3d11Ctx->VSGetConstantBuffers(0, 1, &outState->vCBuffer);
	d3d11Ctx->PSGetConstantBuffers(0, 1, &outState->pCBuffer);
	d3d11Ctx->IAGetInputLayout(&outState->iLayout);
	d3d11Ctx->OMGetRenderTargets(1, &outState->rsv, &outState->dsv);
	d3d11Ctx->OMGetDepthStencilState(&outState->dss, &outState->stencilRef);
	outState->vNumClassInstances = 1;
	d3d11Ctx->VSGetShader(&outState->vShader, &outState->vClassInstance, &outState->vNumClassInstances);
	outState->pNumClassInstances = 1;
	d3d11Ctx->PSGetShader(&outState->pShader, &outState->pClassInstance, &outState->pNumClassInstances);
	d3d11Ctx->IAGetVertexBuffers(0, 1, &outState->vBuffer, &outState->vStride, &outState->vOffset);
	d3d11Ctx->IAGetIndexBuffer(&outState->iBuffer, &outState->iFormat, &outState->iOffset);
	d3d11Ctx->VSGetSamplers(0, 1, &outState->vSampler);
	d3d11Ctx->PSGetSamplers(0, 1, &outState->pSampler);
	d3d11Ctx->VSGetShaderResources(0, 1, &outState->vSrv);
	d3d11Ctx->PSGetShaderResources(0, 1, &outState->pSrv);
	d3d11Ctx->RSGetState(&outState->rState);
	d3d11Ctx->OMGetBlendState(&outState->bState, outState->bFactor, &outState->bSampleMask);
	outState->numVps = ARRAYSIZE(outState->vps);
	d3d11Ctx->RSGetViewports(&outState->numVps, outState->vps);

	d3d11Ctx->IAGetPrimitiveTopology(&outState->topology);
}

void D3D11ApplyState(ID3D11DeviceContext* d3d11Ctx, D3D11State* state)
{
	d3d11Ctx->VSSetConstantBuffers(0, 1, &state->vCBuffer);
	d3d11Ctx->PSSetConstantBuffers(0, 1, &state->pCBuffer);
	d3d11Ctx->IASetInputLayout(state->iLayout);
	d3d11Ctx->OMSetRenderTargets(1, &state->rsv, state->dsv);
	d3d11Ctx->OMSetDepthStencilState(state->dss, state->stencilRef);
	d3d11Ctx->VSSetShader(state->vShader, &state->vClassInstance, state->vNumClassInstances);
	d3d11Ctx->PSSetShader(state->pShader, &state->pClassInstance, state->pNumClassInstances);
	d3d11Ctx->IASetVertexBuffers(0, 1, &state->vBuffer, &state->vStride, &state->vOffset);
	d3d11Ctx->IASetIndexBuffer(state->iBuffer, state->iFormat, state->iOffset);
	d3d11Ctx->VSSetSamplers(0, 1, &state->vSampler);
	d3d11Ctx->PSSetSamplers(0, 1, &state->pSampler);
	d3d11Ctx->VSSetShaderResources(0, 1, &state->vSrv);
	d3d11Ctx->PSSetShaderResources(0, 1, &state->pSrv);
	d3d11Ctx->RSSetState(state->rState);
	d3d11Ctx->OMSetBlendState(state->bState, state->bFactor, state->bSampleMask);
	d3d11Ctx->RSSetViewports(state->numVps, state->vps);

	d3d11Ctx->IASetPrimitiveTopology(state->topology);

	SafeRelease(state->vCBuffer);
	SafeRelease(state->pCBuffer);
	SafeRelease(state->iLayout);
	SafeRelease(state->rsv);
	SafeRelease(state->dsv);
	SafeRelease(state->dss);
	SafeRelease(state->vShader);
	SafeRelease(state->vClassInstance);
	SafeRelease(state->pShader);
	SafeRelease(state->pClassInstance);
	SafeRelease(state->vBuffer);
	SafeRelease(state->iBuffer);
	SafeRelease(state->vSampler);
	SafeRelease(state->pSampler);
	SafeRelease(state->vSrv);
	SafeRelease(state->pSrv);
	SafeRelease(state->rState);
	SafeRelease(state->bState);
}

void RGBToFloat4(float32* dest, uint32 src)
{
	dest[0] = MEX_GET_R(src) / 255.0f;
	dest[1] = MEX_GET_G(src) / 255.0f;
	dest[2] = MEX_GET_B(src) / 255.0f;
	dest[3] = 1.0f;
}

void WINAPI OnLostDevice(cSurface* surf, LPARAM lParam)
{
	((RenderContext*)lParam)->OnDeviceLoss();
	surf->OnLostDevice();
}

/*

---------- MESH ----------

*/

bool Mesh::ResetDriverResources()
{
	switch (globalCtx->driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		if (!globalCtx->d3d9Device)
			return false;

		for (auto& subset : subsets)
		{
			SafeRelease(subset.d3d9VBuffer);
			auto dataLength = (uint32)subset.vertices.size() * sizeof(subset.vertices[0]);
			if (FAILED(globalCtx->d3d9Device->CreateVertexBuffer(
				dataLength,
				0,
				NULL,
				D3DPOOL_DEFAULT,
				&subset.d3d9VBuffer,
				NULL
			)))
				goto failed;

			void* pVertices;
			if (SUCCEEDED(subset.d3d9VBuffer->Lock(0, dataLength, (void**)&pVertices, D3DLOCK_DISCARD)))
			{
				std::memcpy(pVertices, subset.vertices.data(), dataLength);
				subset.d3d9VBuffer->Unlock();
			}
			else
			{
				goto failed;
			}

			for (auto& prim : subset.primitives)
			{
				SafeRelease(prim.d3d9IBuffer);
				dataLength = (uint32)prim.indices.size() * sizeof(prim.indices[0]);
				if (FAILED(globalCtx->d3d9Device->CreateIndexBuffer(
					dataLength,
					0,
					D3DFMT_INDEX32,
					D3DPOOL_DEFAULT,
					&prim.d3d9IBuffer,
					NULL
				)))
					goto failed;

				void* pIndices;
				if (SUCCEEDED(prim.d3d9IBuffer->Lock(0, dataLength, &pIndices, D3DLOCK_DISCARD)))
				{
					std::memcpy(pIndices, prim.indices.data(), dataLength);
					prim.d3d9IBuffer->Unlock();
				}
				else
				{
					goto failed;
				}
			}
		}
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		if (!globalCtx->d3d11Device)
			return false;

		D3D11_BUFFER_DESC bufferDesc = {};
		D3D11_SUBRESOURCE_DATA initialData = {};
		for (auto& subset : subsets)
		{
			bufferDesc.ByteWidth = (uint32)subset.vertices.size() * sizeof(subset.vertices[0]);
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			initialData.pSysMem = subset.vertices.data();

			SafeRelease(subset.d3d11VBuffer);
			if (FAILED(globalCtx->d3d11Device->CreateBuffer(&bufferDesc, &initialData, &subset.d3d11VBuffer)))
				goto failed;

			for (auto& prim : subset.primitives)
			{
				bufferDesc.ByteWidth = (uint32)prim.indices.size() * sizeof(prim.indices[0]);
				bufferDesc.Usage = D3D11_USAGE_DEFAULT;
				bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				initialData.pSysMem = prim.indices.data();

				SafeRelease(prim.d3d11IBuffer);
				if (FAILED(globalCtx->d3d11Device->CreateBuffer(&bufferDesc, &initialData, &prim.d3d11IBuffer)))
					goto failed;
			}
		}
		break;
	}
	}

	needReset = false;
	return true;

failed:
	ReleaseDriverResources();
	return false;
}

void Mesh::ReleaseDriverResources()
{
	switch (globalCtx->driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		for (auto& subset : subsets)
		{
			SafeRelease(subset.d3d9VBuffer);
			for (auto& prim : subset.primitives)
				SafeRelease(prim.d3d9IBuffer);
		}
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		for (auto& subset : subsets)
		{
			SafeRelease(subset.d3d11VBuffer);
			for (auto& prim : subset.primitives)
				SafeRelease(prim.d3d11IBuffer);
		}
		break;
	}
	}
	needReset = true;
}

/*

---------- GLOBAL CONTEXT ----------

*/

bool GlobalContext::InitDriver()
{
	if (!inited)
	{
		auto surf = WinGetSurface((int)Edif::SDK->mV->mvIdEditWin);
		if (!surf)
			return false;

		auto surfDriver = (SurfaceDriver)surf->GetDriver();
		if (surfDriver != SurfaceDriver::Direct3D9 && surfDriver != SurfaceDriver::Direct3D11)
			return false;

		FusionD3DSurfDriverInfo di;
		di.surfInfoSize = sizeof(di);
		if (surf->GetDriverInfo(&di))
		{
			switch (surfDriver)
			{
			case SurfaceDriver::Direct3D9:
			{
				d3d9Device = di.D3DDevice.D3D9Device;
				if (!d3d9Device)
					return false;

				break;
			}
			case SurfaceDriver::Direct3D11:
			{
				d3d11Device = di.D3DDevice.D3D11Device;
				d3d11Ctx = di.D3DContextOrTech.D3D11DevCtx;
				if (!d3d11Device || !d3d11Ctx)
					return false;

				break;
			}
			}
		}
		else
		{
			return false;
		}

		driverType = surfDriver;
		ResetDriver();
		inited = true;
	}
	return true;
}

bool GlobalContext::ResetDriver()
{
	switch (driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		if (!d3d9Device)
			return false;

		SafeRelease(d3d9SB);
		if (FAILED(d3d9Device->CreateStateBlock(D3DSBT_ALL, &d3d9SB)))
			return false;

		static const D3DVERTEXELEMENT9 vElements[] =
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
			{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
			{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			{ 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
			{ 0, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
			D3DDECL_END()
		};

		SafeRelease(d3d9VDec);
		if (FAILED(d3d9Device->CreateVertexDeclaration(vElements, &d3d9VDec)))
			return false;

		// Vertex shaders:
		// -----
		SafeRelease(d3d9StaticShader);
		if (FAILED(d3d9Device->CreateVertexShader((DWORD*)staticVSBytes9, &d3d9StaticShader)))
			return false;

		SafeRelease(d3d9SkinningShader);
		if (FAILED(d3d9Device->CreateVertexShader((DWORD*)skinnedVSBytes9, &d3d9SkinningShader)))
			return false;
		// -----

		// Pixel shaders:
		// -----
		SafeRelease(d3d9TexShader);
		if (FAILED(d3d9Device->CreatePixelShader((DWORD*)texturePSBytes9, &d3d9TexShader)))
			return false;

		SafeRelease(d3d9ColorShader);
		if (FAILED(d3d9Device->CreatePixelShader((DWORD*)coloredPSBytes9, &d3d9ColorShader)))
			return false;
		// -----

		// Bone matrices texture:
		SafeRelease(d3d9DeformTex);
		if (FAILED(d3d9Device->CreateTexture(
			MEX_BONE_TEX_SIZE,
			MEX_BONE_TEX_SIZE,
			1,
			D3DUSAGE_DYNAMIC,
			D3DFMT_A32B32G32R32F,
			D3DPOOL_DEFAULT,
			&d3d9DeformTex,
			NULL
		)))
			return false;

		D3DLOCKED_RECT locked;
		if (SUCCEEDED(d3d9DeformTex->LockRect(0, &locked, NULL, D3DLOCK_DISCARD)))
		{
			std::memset(locked.pBits, 0x0, locked.Pitch * MEX_BONE_TEX_SIZE);
			d3d9DeformTex->UnlockRect(0);
		}

		needReset = false;
		return true;
	}
	case SurfaceDriver::Direct3D11:
	{
		if (!d3d11Device)
			return false;

		static const D3D11_INPUT_ELEMENT_DESC iDesc[] =
		{
			{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normX), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, u), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, boneIDs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, boneWeights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		if (FAILED(d3d11Device->CreateInputLayout(iDesc, ARRAYSIZE(iDesc), staticVSBytes11, sizeof(staticVSBytes11), &d3d11ILayout)))
			return false;

		// Vertex shaders:
		// -----
		SafeRelease(d3d11StaticShader);
		if (FAILED(d3d11Device->CreateVertexShader(staticVSBytes11, sizeof(staticVSBytes11), NULL, &d3d11StaticShader)))
			return false;

		SafeRelease(d3d11SkinningShader);
		if (FAILED(d3d11Device->CreateVertexShader(skinnedVSBytes11, sizeof(skinnedVSBytes11), NULL, &d3d11SkinningShader)))
			return false;
		// -----

		// Pixel shaders:
		// -----
		SafeRelease(d3d11ColorShader);
		if (FAILED(d3d11Device->CreatePixelShader(coloredPSBytes11, sizeof(coloredPSBytes11), NULL, &d3d11ColorShader)))
			return false;

		SafeRelease(d3d11TexShader);
		if (FAILED(d3d11Device->CreatePixelShader(texturePSBytes11, sizeof(texturePSBytes11), NULL, &d3d11TexShader)))
			return false;
		// -----

		// Bone matrices texture:
		// -----
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = MEX_BONE_TEX_SIZE;
		texDesc.Height = MEX_BONE_TEX_SIZE;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texDesc.Usage = D3D11_USAGE_DYNAMIC;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		SafeRelease(d3d11DeformTex);
		if (FAILED(d3d11Device->CreateTexture2D(&texDesc, NULL, &d3d11DeformTex)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		SafeRelease(d3d11DeformSRV);
		if (FAILED(d3d11Device->CreateShaderResourceView(d3d11DeformTex, &srvDesc, &d3d11DeformSRV)))
			return false;

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(d3d11Ctx->Map(d3d11DeformTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			std::memset(mapped.pData, 0x0, mapped.RowPitch * texDesc.Height);
			d3d11Ctx->Unmap(d3d11DeformTex, 0);
		}
		// -----

		// Constant buffer:
		// -----
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = 256; // random arbitrary size
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		SafeRelease(d3d11VCBuffer);
		if (FAILED(d3d11Device->CreateBuffer(&bufferDesc, NULL, &d3d11VCBuffer)))
			return false;

		SafeRelease(d3d11PCBuffer);
		if (FAILED(d3d11Device->CreateBuffer(&bufferDesc, NULL, &d3d11PCBuffer)))
			return false;

		// -----

		// Point/clamp sampler state
		// -----
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		SafeRelease(d3d11PointClampSampler);
		if (FAILED(d3d11Device->CreateSamplerState(&samplerDesc, &d3d11PointClampSampler)))
			return false;
		// -----

		// Blend state:
		// -----
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		SafeRelease(d3d11BState);
		if (FAILED(d3d11Device->CreateBlendState(&blendDesc, &d3d11BState)))
			return false;
		// -----

		// Depth-stencil state
		// -----
		D3D11_DEPTH_STENCIL_DESC depthDesc = {};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;
		SafeRelease(d3d11DSS);
		if (FAILED(d3d11Device->CreateDepthStencilState(&depthDesc, &d3d11DSS)))
			return false;
		// -----

		needReset = false;
		return true;
	}
	default:
	{
		return false;
	}
	}
}

void GlobalContext::ReleaseDriverResources()
{
	switch (driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		SafeRelease(d3d9SB);
		SafeRelease(d3d9DeformTex);
		SafeRelease(d3d9VDec);
		SafeRelease(d3d9StaticShader);
		SafeRelease(d3d9SkinningShader);
		SafeRelease(d3d9TexShader);
		SafeRelease(d3d9ColorShader);
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		SafeRelease(d3d11VCBuffer);
		SafeRelease(d3d11PCBuffer);
		SafeRelease(d3d11DSS);
		SafeRelease(d3d11DeformTex);
		SafeRelease(d3d11DeformSRV);
		SafeRelease(d3d11ILayout);
		SafeRelease(d3d11StaticShader);
		SafeRelease(d3d11SkinningShader);
		SafeRelease(d3d11TexShader);
		SafeRelease(d3d11ColorShader);
		SafeRelease(d3d11PointClampSampler);
		SafeRelease(d3d11BState);
		break;
	}
	}
	needReset = true;
}


bool GlobalContext::CreateTextureSurfaceFromImageBank(void* fusionApp, Texture& texture)
{
	cSurface *proto, tempSurf;
	if (!GetSurfacePrototype(&proto, 32, (int)SurfaceType::HWA_ManagedTexture, (int)driverType) ||
		!LockImageSurface(fusionApp, texture.imageID, tempSurf, (int)LockImageSurfaceMode::ReadBlitOnly))
		return false;

	// Create an HWA surface and blit the DIB surface to it:
	// LockImageSurfaceMode has HWACompatible.. but it seemed iffy when I tried it
	texture.surf.reset(NewSurface(), DeleteSurface);
	texture.surf->Create(tempSurf.GetWidth(), tempSurf.GetHeight(), proto);
	tempSurf.Blit(*texture.surf);
	UnlockImageSurface(tempSurf);
	return true;
}

bool GlobalContext::CreateTextureSurfaceFromFile(RunHeader* rhPtr, Texture& texture)
{
	cSurface *proto, tempSurf;
	DWORD filterID;
	if (!GetSurfacePrototype(&proto, 32, (int)SurfaceType::HWA_ManagedTexture, (int)driverType) ||
		!ImportImage(rhPtr->rh4.rh4Mv->mvImgFilterMgr, (const UShortWCHAR*)texture.fileName, &tempSurf, &filterID, 0))
		return false;

	texture.surf.reset(NewSurface(), DeleteSurface);
	texture.surf->Create(tempSurf.GetWidth(), tempSurf.GetHeight(), proto);
	tempSurf.Blit(*texture.surf);
	return true;
}

/*

---------- RENDER CONTEXT ----------

*/

bool RenderContext::InitDriver(int32 renderWidth, int32 renderHeight)
{
	if (!inited)
	{
		if (!globalCtx->InitDriver())
			return false;

		CreateSurface(renderWidth, renderHeight);
		inited = true;
	}
	return true;
}


void RenderContext::DrawToSurface()
{
	if (meshID == MEX_UNDEFINED)
		return;
	auto& mesh = globalCtx->GetMesh(meshID);

	if (globalCtx->NeedReset())
	{
		if (!globalCtx->ResetDriver())
			return;
	}
	if (mesh.needReset)
	{
		if (!mesh.ResetDriverResources())
			return;

		CreateSurface(width, height);
	}

	switch (globalCtx->driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		if (!globalCtx->d3d9Device)
			return;

		if (SUCCEEDED(globalCtx->d3d9SB->Capture()))
		{
			if (surf.BeginRendering(TRUE, 0))
			{
				globalCtx->d3d9Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
				globalCtx->d3d9Device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
				globalCtx->d3d9Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
				globalCtx->d3d9Device->SetRenderState(D3DRS_CLIPPING, TRUE);

				globalCtx->d3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				globalCtx->d3d9Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				globalCtx->d3d9Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				globalCtx->d3d9Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

				auto filtering = meshProps.filtering == MeshFiltering_Point ? D3DTEXF_POINT : D3DTEXF_LINEAR;
				auto addressMode = meshProps.addressMode == MeshAddressing_Wrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
				auto wireframe = meshProps.wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID;
				globalCtx->d3d9Device->SetSamplerState(0, D3DSAMP_MINFILTER, filtering);
				globalCtx->d3d9Device->SetSamplerState(0, D3DSAMP_MAGFILTER, filtering);
				globalCtx->d3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSU, addressMode);
				globalCtx->d3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSV, addressMode);
				globalCtx->d3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSW, addressMode);
				globalCtx->d3d9Device->SetRenderState(D3DRS_FILLMODE, wireframe);
				D3DCULL cull;
				switch (meshProps.cull)
				{
				case MeshCull_Back:
				{
					cull = D3DCULL_CCW;
					break;
				}
				case MeshCull_Front:
				{
					cull = D3DCULL_CW;
					break;
				}
				case MeshCull_None:
				default:
				{
					cull = D3DCULL_NONE;
					break;
				}
				}
				globalCtx->d3d9Device->SetRenderState(D3DRS_CULLMODE, cull);

				IDirect3DSurface9* prevDepthBuffer;
				globalCtx->d3d9Device->GetDepthStencilSurface(&prevDepthBuffer);
				globalCtx->d3d9Device->SetDepthStencilSurface(d3d9DepthBuffer);
				globalCtx->d3d9Device->Clear(1, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, NULL);

				D3DVIEWPORT9 vp;
				vp.X = vp.Y = 0;
				vp.Width = (DWORD)width;
				vp.Height = (DWORD)height;
				vp.MinZ = 0.0f;
				vp.MaxZ = 1.0f;
				globalCtx->d3d9Device->SetViewport(&vp);

				globalCtx->d3d9Device->SetVertexDeclaration(globalCtx->d3d9VDec);
				float32 ambient[4];
				float32 diffuse[4];
				float32 fogColor[4];
				RGBToFloat4(ambient, lightingProps.ambient);
				RGBToFloat4(diffuse, lightingProps.diffuse);
				RGBToFloat4(fogColor, lightingProps.fogColor);
				float32 lightDir[4] = { lightingProps.dirX, lightingProps.dirY, lightingProps.dirZ, 1.0f };
				globalCtx->d3d9Device->SetPixelShaderConstantF(0, ambient, 1);
				globalCtx->d3d9Device->SetPixelShaderConstantF(1, diffuse, 1);
				globalCtx->d3d9Device->SetPixelShaderConstantF(2, fogColor, 1);
				globalCtx->d3d9Device->SetPixelShaderConstantF(3, lightDir, 1);
				globalCtx->d3d9Device->SetPixelShaderConstantF(4, &lightingProps.fogDist, 1);

				for (auto rootSubsetID : mesh.rootSubsets)
					DrawSubset(mesh, rootSubsetID, identityMatrix, identityMatrix);

				// Restore some state not handled by the state block:
				// -----
				globalCtx->d3d9Device->SetDepthStencilSurface(prevDepthBuffer);
				if (prevDepthBuffer)
					prevDepthBuffer->Release();
				// -----
				surf.EndRendering();

				updateCMask = true;
			}
			globalCtx->d3d9SB->Apply();
		}
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		if (!globalCtx->d3d11Device)
			return;

		D3D11State ds;
		if (surf.BeginRendering(TRUE, 0))
		{
			D3D11CaptureState(globalCtx->d3d11Ctx, &ds);
			if (!d3d11Sampler || prevFiltering != meshProps.filtering || prevAddressMode != meshProps.addressMode)
			{
				D3D11_SAMPLER_DESC samplerDesc = {};
				samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = meshProps.addressMode == MeshAddressing_Wrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
				samplerDesc.Filter = meshProps.filtering == MeshFiltering_Point ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
				SafeRelease(d3d11Sampler);
				if (FAILED(globalCtx->d3d11Device->CreateSamplerState(&samplerDesc, &d3d11Sampler)))
					return;

				prevFiltering = meshProps.filtering;
				prevAddressMode = meshProps.addressMode;
			}
			if (!d3d11RState || prevWireframe != meshProps.wireframe || prevCull != meshProps.cull)
			{
				D3D11_RASTERIZER_DESC rasterizerDesc = {};
				rasterizerDesc.FillMode = meshProps.wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
				switch (meshProps.cull)
				{
				case MeshCull_Back:
				{
					rasterizerDesc.CullMode = D3D11_CULL_BACK;
					break;
				}
				case MeshCull_Front:
				{
					rasterizerDesc.CullMode = D3D11_CULL_FRONT;
					break;
				}
				case MeshCull_None:
				default:
				{
					rasterizerDesc.CullMode = D3D11_CULL_NONE;
					break;
				}
				}
				rasterizerDesc.FrontCounterClockwise = FALSE;
				rasterizerDesc.DepthClipEnable = TRUE;
				SafeRelease(d3d11RState);
				if (FAILED(globalCtx->d3d11Device->CreateRasterizerState(&rasterizerDesc, &d3d11RState)))
					return;

				prevWireframe = meshProps.wireframe;
				prevCull = meshProps.cull;
			}

			globalCtx->d3d11Ctx->RSSetState(d3d11RState);
			FLOAT blendFactors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			globalCtx->d3d11Ctx->OMSetBlendState(globalCtx->d3d11BState, blendFactors, 0xFFFFFFFF);
			globalCtx->d3d11Ctx->PSSetSamplers(0, 1, &d3d11Sampler);

			globalCtx->d3d11Ctx->OMSetRenderTargets(1, &ds.rsv, d3d11DepthBuffer);
			globalCtx->d3d11Ctx->OMSetDepthStencilState(globalCtx->d3d11DSS, 0);
			globalCtx->d3d11Ctx->ClearDepthStencilView(d3d11DepthBuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			D3D11_VIEWPORT vp;
			vp.TopLeftX = vp.TopLeftY = 0.0f;
			vp.Width = (float)width;
			vp.Height = (float)height;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			globalCtx->d3d11Ctx->RSSetViewports(1, &vp);

			globalCtx->d3d11Ctx->IASetInputLayout(globalCtx->d3d11ILayout);
			globalCtx->d3d11Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			CBuffer_Pixel cbPixel;
			RGBToFloat4(cbPixel.ambient, lightingProps.ambient);
			RGBToFloat4(cbPixel.diffuse, lightingProps.diffuse);
			RGBToFloat4(cbPixel.fogColor, lightingProps.fogColor);
			cbPixel.lightDir[0] = lightingProps.dirX;
			cbPixel.lightDir[1] = lightingProps.dirY;
			cbPixel.lightDir[2] = lightingProps.dirZ;
			cbPixel.fogDist = lightingProps.fogDist;

			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(globalCtx->d3d11Ctx->Map(globalCtx->d3d11PCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				std::memcpy(mapped.pData, &cbPixel, sizeof(cbPixel));
				globalCtx->d3d11Ctx->Unmap(globalCtx->d3d11PCBuffer, 0);
			}
			globalCtx->d3d11Ctx->PSSetConstantBuffers(0, 1, &globalCtx->d3d11PCBuffer);

			for (auto rootSubsetID : mesh.rootSubsets)
				DrawSubset(mesh, rootSubsetID, identityMatrix, identityMatrix);

			D3D11ApplyState(globalCtx->d3d11Ctx, &ds);
			surf.EndRendering();

			updateCMask = true;
		}
		break;
	}
	}
}


cSurface* RenderContext::GetSurface()
{
	return &surf;
}

GlobalContext* RenderContext::GetGlobalContext()
{
	return globalCtx;
}


void RenderContext::OnDeviceLoss()
{
	for (uint32 i = 0; i < globalCtx->GetNumMeshes(); ++i)
		globalCtx->GetMesh(i).ReleaseDriverResources();

	ReleaseDriverResources();
	globalCtx->ReleaseDriverResources();
}


void RenderContext::PlatformDrawSubset(Mesh& mesh, const Subset& subset, const Matrix& mat, const Matrix& matNormal)
{
	switch (globalCtx->driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		if (!globalCtx->d3d9DeformTex)
			return;

		Matrix d3dMat;
		PlatformConvertMatrix(mat, d3dMat);
		globalCtx->d3d9Device->SetVertexShaderConstantF(0, (float32*)&d3dMat, 4);

		PlatformConvertMatrix(matNormal, d3dMat);
		globalCtx->d3d9Device->SetVertexShaderConstantF(4, (float32*)&d3dMat, 4);

		float32 screenRes[4] = { (float)width, (float)height, 0.0f, 1.0f };
		globalCtx->d3d9Device->SetVertexShaderConstantF(8, screenRes, 1);

		if (subset.animated)
		{
			globalCtx->d3d9Device->SetVertexShader(globalCtx->d3d9SkinningShader);
			for (auto rootBoneID : mesh.rootBones)
				UpdateSubsetDeforms(mesh, subset, rootBoneID);

			D3DLOCKED_RECT locked;
			if (SUCCEEDED(globalCtx->d3d9DeformTex->LockRect(0, &locked, NULL, NULL)))
			{
				WriteBoneDeformTexture(mesh, (float32*)locked.pBits, (uint32)locked.Pitch);
				globalCtx->d3d9DeformTex->UnlockRect(0);
			}

			globalCtx->d3d9Device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			globalCtx->d3d9Device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			globalCtx->d3d9Device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
			globalCtx->d3d9Device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			globalCtx->d3d9Device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			globalCtx->d3d9Device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
			globalCtx->d3d9Device->SetTexture(D3DVERTEXTEXTURESAMPLER0, globalCtx->d3d9DeformTex);
		}
		else
		{
			globalCtx->d3d9Device->SetVertexShader(globalCtx->d3d9StaticShader);
		}
		globalCtx->d3d9Device->SetStreamSource(0, subset.d3d9VBuffer, 0, sizeof(Vertex));
		for (const auto& prim : subset.primitives)
		{
			globalCtx->d3d9Device->SetIndices(prim.d3d9IBuffer);
			if (prim.textureID < textureIDs.size())
			{
				FusionD3DSurfDriverInfo di = {};
				di.surfInfoSize = sizeof(di);
				globalCtx->GetTexture(textureIDs[prim.textureID]).surf->GetDriverInfo(&di);

				if (di.D3DTexture.D3D9Texture)
				{
					globalCtx->d3d9Device->SetTexture(0, di.D3DTexture.D3D9Texture);
					globalCtx->d3d9Device->SetPixelShader(globalCtx->d3d9TexShader);
				}
				else
				{
					globalCtx->d3d9Device->SetPixelShader(globalCtx->d3d9ColorShader);
				}
			}
			else
			{
				globalCtx->d3d9Device->SetPixelShader(globalCtx->d3d9ColorShader);
			}
			globalCtx->d3d9Device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, subset.vertices.size(), 0, prim.indices.size() / 3);
		}
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		CBuffer_Vertex cbVertex;
		PlatformConvertMatrix(mat, cbVertex.wvp);
		PlatformConvertMatrix(matNormal, cbVertex.wNormal);

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(globalCtx->d3d11Ctx->Map(globalCtx->d3d11VCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			std::memcpy(mapped.pData, &cbVertex, sizeof(cbVertex));
			globalCtx->d3d11Ctx->Unmap(globalCtx->d3d11VCBuffer, 0);
		}
		globalCtx->d3d11Ctx->VSSetConstantBuffers(0, 1, &globalCtx->d3d11VCBuffer);

		if (subset.animated && globalCtx->d3d11DeformSRV)
		{
			globalCtx->d3d11Ctx->VSSetShader(globalCtx->d3d11SkinningShader, NULL, NULL);
			for (auto rootBoneID : mesh.rootBones)
				UpdateSubsetDeforms(mesh, subset, rootBoneID);

			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(globalCtx->d3d11Ctx->Map(globalCtx->d3d11DeformTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				WriteBoneDeformTexture(mesh, (float32*)mapped.pData, (uint32)mapped.RowPitch);
				globalCtx->d3d11Ctx->Unmap(globalCtx->d3d11DeformTex, 0);
			}

			globalCtx->d3d11Ctx->VSSetSamplers(0, 1, &globalCtx->d3d11PointClampSampler);
			globalCtx->d3d11Ctx->VSSetShaderResources(0, 1, &globalCtx->d3d11DeformSRV);
		}
		else
		{
			globalCtx->d3d11Ctx->VSSetShader(globalCtx->d3d11StaticShader, NULL, NULL);
		}
		UINT stride = sizeof(Vertex), offset = 0;
		globalCtx->d3d11Ctx->IASetVertexBuffers(0, 1, &subset.d3d11VBuffer, &stride, &offset);
		for (const auto& prim : subset.primitives)
		{
			globalCtx->d3d11Ctx->IASetIndexBuffer(prim.d3d11IBuffer, DXGI_FORMAT_R32_UINT, offset);
			if (prim.textureID < textureIDs.size())
			{
				FusionD3DSurfDriverInfo di = {};
				di.surfInfoSize = sizeof(di);
				globalCtx->GetTexture(textureIDs[prim.textureID]).surf->GetDriverInfo(&di);

				auto srv = di.D3DTexture.D3D11TextureHolder ? di.D3DTexture.D3D11TextureHolder->D3D11ShaderResourceView : nullptr;
				if (srv)
				{
					globalCtx->d3d11Ctx->PSSetShaderResources(0, 1, &srv);
					globalCtx->d3d11Ctx->PSSetShader(globalCtx->d3d11TexShader, NULL, NULL);
				}
				else
				{
					globalCtx->d3d11Ctx->PSSetShader(globalCtx->d3d11ColorShader, NULL, NULL);
				}
			}
			else
			{
				globalCtx->d3d11Ctx->PSSetShader(globalCtx->d3d11ColorShader, NULL, NULL);
			}
			globalCtx->d3d11Ctx->DrawIndexed((UINT)prim.indices.size(), 0, 0);
		}
		break;
	}
	}
}

void RenderContext::PlatformCreateSurface()
{
	cSurface* proto;
	if (GetSurfacePrototype(&proto, 32, (int)SurfaceType::HWA_RenderTargetTexture, (int)globalCtx->driverType))
	{
		surf.Create(width, height, proto);
		surf.AddLostDeviceCallBack(OnLostDevice, (LPARAM)this);

		switch (globalCtx->driverType)
		{
		case SurfaceDriver::Direct3D9:
		{
			if (!globalCtx->d3d9Device)
				return;

			SafeRelease(d3d9DepthBuffer);
			globalCtx->d3d9Device->CreateDepthStencilSurface(
				width, height,
				D3DFMT_D24X8,
				D3DMULTISAMPLE_NONE,
				0,
				FALSE,
				&d3d9DepthBuffer,
				NULL
			);

			SafeRelease(d3d9CMask);
			break;
		}
		case SurfaceDriver::Direct3D11:
		{
			if (!globalCtx->d3d11Device)
				return;

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.ArraySize = 1;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			SafeRelease(d3d11DepthBufferTex);
			if (SUCCEEDED(globalCtx->d3d11Device->CreateTexture2D(&texDesc, NULL, &d3d11DepthBufferTex)))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = texDesc.Format;
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;

				SafeRelease(d3d11DepthBuffer);
				globalCtx->d3d11Device->CreateDepthStencilView(d3d11DepthBufferTex, &dsvDesc, &d3d11DepthBuffer);
			}

			SafeRelease(d3d11CMask);
			break;
		}
		}
	}
	else
		DarkEdif::MsgBox::Error(_T("Error"), _T("Failed to get surf prototype"));
}

void RenderContext::ReleaseDriverResources()
{
	switch (globalCtx->driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		SafeRelease(d3d9DepthBuffer);
		SafeRelease(d3d9CMask);
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		SafeRelease(d3d11DepthBuffer);
		SafeRelease(d3d11DepthBufferTex);
		SafeRelease(d3d11CMask);
		SafeRelease(d3d11Sampler);
		SafeRelease(d3d11RState);
		break;
	}
	}
}

void RenderContext::UpdateCollisionMask()
{
	switch (globalCtx->driverType)
	{
	case SurfaceDriver::Direct3D9:
	{
		FusionD3DSurfDriverInfo di;
		di.surfInfoSize = sizeof(di);
		surf.GetDriverInfo(&di);

		auto tex = di.D3DTexture.D3D9Texture;
		if (!tex)
			return;

		if (!d3d9CMask && FAILED(globalCtx->d3d9Device->CreateOffscreenPlainSurface(
			width, height,
			D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM,
			&d3d9CMask,
			NULL
		)))
			return;

		IDirect3DSurface9* texSurf;
		if (SUCCEEDED(tex->GetSurfaceLevel(0, &texSurf)))
		{
			if (SUCCEEDED(globalCtx->d3d9Device->GetRenderTargetData(texSurf, d3d9CMask)))
			{
				D3DLOCKED_RECT locked;
				if (SUCCEEDED(d3d9CMask->LockRect(&locked, NULL, D3DLOCK_READONLY)))
				{
					WriteMaskData(cMask->get_data(), (uint32*)locked.pBits, cMask->get_widthBytes(), (uint32)locked.Pitch);
					d3d9CMask->UnlockRect();
				}
			}
			texSurf->Release();
		}
		break;
	}
	case SurfaceDriver::Direct3D11:
	{
		FusionD3DSurfDriverInfo di;
		di.surfInfoSize = sizeof(di);
		surf.GetDriverInfo(&di);

		auto srv = di.D3DTexture.D3D11TextureHolder ? di.D3DTexture.D3D11TextureHolder->D3D11ShaderResourceView : nullptr;
		if (!srv)
			return;

		ID3D11Texture2D* tex;
		srv->GetResource((ID3D11Resource**)&tex);
		if (!tex)
			return;

		if (!d3d11CMask)
		{
			D3D11_TEXTURE2D_DESC srcTexDesc;
			tex->GetDesc(&srcTexDesc);

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = srcTexDesc.Width;
			texDesc.Height = srcTexDesc.Height;
			texDesc.MipLevels = 1;
			texDesc.Format = srcTexDesc.Format;
			texDesc.Usage = D3D11_USAGE_STAGING;
			texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			texDesc.ArraySize = 1;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			if (FAILED(globalCtx->d3d11Device->CreateTexture2D(&texDesc, NULL, &d3d11CMask)))
			{
				tex->Release();
				return;
			}
		}

		globalCtx->d3d11Ctx->CopyResource(d3d11CMask, tex);

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(globalCtx->d3d11Ctx->Map(d3d11CMask, 0, D3D11_MAP_READ, 0, &mapped)))
		{
			WriteMaskData(cMask->get_data(), (uint32*)mapped.pData, cMask->get_widthBytes(), mapped.RowPitch);
			globalCtx->d3d11Ctx->Unmap(d3d11CMask, 0);
		}
		tex->Release();
		break;
	}
	}
}

/*

---------- MISCELLANEOUS FUNCTIONS ----------

*/

void MeshEx::PlatformConvertMatrix(Matrix matrix, Matrix& out)
{
	// Direct3D matrices are stored as column-major, but my matrices are row-major:
	out.rows[0][0] = matrix.rows[0][0];
	out.rows[1][0] = matrix.rows[0][1];
	out.rows[2][0] = matrix.rows[0][2];
	out.rows[3][0] = matrix.rows[0][3];

	out.rows[0][1] = matrix.rows[1][0];
	out.rows[1][1] = matrix.rows[1][1];
	out.rows[2][1] = matrix.rows[1][2];
	out.rows[3][1] = matrix.rows[1][3];

	out.rows[0][2] = matrix.rows[2][0];
	out.rows[1][2] = matrix.rows[2][1];
	out.rows[2][2] = matrix.rows[2][2];
	out.rows[3][2] = matrix.rows[2][3];

	out.rows[0][3] = matrix.rows[3][0];
	out.rows[1][3] = matrix.rows[3][1];
	out.rows[2][3] = matrix.rows[3][2];
	out.rows[3][3] = matrix.rows[3][3];
}

/*

---------- FILE API ----------

*/

bool MeshEx::FileOpen(const wchar* fileName, File& outFile)
{
	auto ret = true;
	auto handle = new FileHandle;
	outFile = handle;

	DWORD dwSize, dwOffset;
#if EditorBuild
	if (DarkEdif::RunMode != DarkEdif::MFXRunMode::Editor) // OpenHFile is a runtime-only function
	{
		handle->hFile = (HFILE)Edif::SDK->mV->mvOpenHFileW(fileName, &dwSize, 0);
	}
	else
	{
		handle->hFile = File_Open((const UShortWCHAR*)fileName, OF_READ);
		dwSize = handle->hFile != HFILE_ERROR ? (DWORD)File_GetLength_Ex(handle->hFile) : 0;
	}
#else
	handle->hFile = (HFILE)Edif::SDK->mV->mvOpenHFileW(fileName, &dwSize, 0);
#endif
	
	if (handle->hFile == HFILE_ERROR)
	{
		ret = false;
		goto finished;
	}

	dwOffset = File_GetPosition(handle->hFile);
	if (handle->pFile.Create(handle->hFile, dwOffset, dwSize) != 0)
	{
		ret = false;
		goto finished;
	}

finished:
	if (!ret)
	{
		FileClose(outFile);
		outFile = MEX_FILE_INVALID;
	}

	return ret;
}

void MeshEx::FileClose(File file)
{
	auto handle = (FileHandle*)file;
	if (handle != MEX_FILE_INVALID && handle->hFile != HFILE_ERROR)
	{
#if EditorBuild
		if (DarkEdif::RunMode != DarkEdif::MFXRunMode::Editor)
			Edif::SDK->mV->mvCloseHFile((HANDLE)handle->hFile);
		else
			File_Close(handle->hFile);
#else
		Edif::SDK->mV->mvCloseHFile((HANDLE)handle->hFile);
#endif

		delete handle;
	}
}


uint32 MeshEx::FileRead(File file, void* buffer, uint32 bufferSize)
{
	auto handle = (FileHandle*)file;
	if (!handle)
		return 0;

	DWORD numRead = 0;
	handle->pFile.Read((LPBYTE)buffer, (DWORD)bufferSize, &numRead);
	return (uint32)numRead;
}

uint32 MeshEx::FileSeek(File file, uint32 offset, FileDirection direction)
{
	auto handle = (FileHandle*)file;
	if (!handle)
		return 0;

	int method;
	switch (direction)
	{
	case FileDirection_Begin:
		method = SEEK_SET;
		break;
	case FileDirection_Current:
		method = SEEK_CUR;
		break;
	case FileDirection_End:
		method = SEEK_END;
		break;
	default:
		return 0;
	}
	return (uint32)handle->pFile.Seek((long)offset, method);
}

uint64 MeshEx::FileSize(File file)
{
	auto handle = (FileHandle*)file;
	if (!handle)
		return 0;

	return (uint64)handle->pFile.GetLengthEx();
}
