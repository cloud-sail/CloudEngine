#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "ThirdParty/directx/d3dx12.h"

//-----------------------------------------------------------------------------------------------
// Important: 
// only include this file in .cpp file
// only used in engine code
//-----------------------------------------------------------------------------------------------

#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

#if defined(OPAQUE)
#undef OPAQUE
#endif

extern const CD3DX12_STATIC_SAMPLER_DESC g_staticSamplers[];
extern const size_t g_numStaticSamplers;


namespace DX12Graphics
{
	constexpr unsigned int MAX_TEXTURE_NUM = 1024;


	//-----------------------------------------------------------------------------------------------
	void InitializeCommonState();
	void DestroyCommonState();


	extern DXGI_FORMAT BackBufferFormat;
	extern DXGI_FORMAT DepthStencilFormat;

	// Common Sampler, Use static sampler now

	// Common Texture, Opaque white normal map...

	// Common Rootsig...

	// Common PSO

	extern D3D12_RASTERIZER_DESC RasterizerSolidCullNone;
	extern D3D12_RASTERIZER_DESC RasterizerSolidCullBack;
	extern D3D12_RASTERIZER_DESC RasterizerWireframeCullNone;
	extern D3D12_RASTERIZER_DESC RasterizerWireframeCullBack;
	
	extern D3D12_BLEND_DESC BlendOpaque;
	extern D3D12_BLEND_DESC BlendAlpha;	
	extern D3D12_BLEND_DESC BlendAdditive;
	
	extern D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
	extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyAlways;
	extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyLessEqual;
	extern D3D12_DEPTH_STENCIL_DESC DepthStateReadWriteLessEqual;
}

