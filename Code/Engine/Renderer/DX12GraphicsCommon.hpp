#pragma once
#include "Engine/Renderer/RendererCommon.hpp"
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

/****************************************************************************************
Texture1D<T>
Texture2D<T>
Texture3D<T>
TextureCube<T>
RW...
*/
//struct TexturexxxInit 
//{
//	D3D12_RESOURCE_DIMENSION m_dimensions = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 1d 2d 3d
//	unsigned int m_width = 0;
//	unsigned int m_height = 0;
//	unsigned short m_depthOrArraySize = 1;
//	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN; // Buffer is unknown, Texture is DXGI_FORMAT_R8G8B8A8_UNORM
//	unsigned short m_mipLevels = 1; // 1: no mipmap 0: auto maximized mip
//	// Sample Desc: no msaa Count is 1 Quality is 0
//	// Layout: texture D3D12_TEXTURE_LAYOUT_UNKNOWN, buffer D3D12_TEXTURE_LAYOUT_ROW_MAJOR
//	D3D12_RESOURCE_FLAGS m_flags = D3D12_RESOURCE_FLAG_NONE;
//	// SRV Only:		D3D12_RESOURCE_FLAG_NONE
//	// Render Target:	D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
//	// Depth Buffer:	D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
//	// Allow UAV:		| D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
//	// Deny SRV:		D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
//	
//	D3D12_RESOURCE_STATES m_initialState = D3D12_RESOURCE_STATES(-1);
//	// Texture Sample only:	D3D12_RESOURCE_STATE_COPY_DEST -> Copy/Upload -> D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
//	// Render Target:		D3D12_RESOURCE_STATE_RENDER_TARGET
//	// Depth Buffer:		D3D12_RESOURCE_STATE_DEPTH_WRITE
//	// UAV, Read Write:		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
//
//};
// Should Texture has some limits. bool, later when do sth GUARANTEE it is rt,ds,uav

uint32_t DXGI_FORMAT_GetStride(DXGI_FORMAT format);

D3D12_RESOURCE_DIMENSION ToDX12Dimension(DXResourceDimension dimension);
//D3D12_RESOURCE_FLAGS	ToDX12Flags(DXResourceFlags flags);



