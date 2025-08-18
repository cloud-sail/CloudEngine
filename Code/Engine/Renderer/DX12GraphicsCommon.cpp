#include "Engine/Renderer/DX12GraphicsCommon.hpp"
#include "ThirdParty/directx/d3dx12.h"

//-----------------------------------------------------------------------------------------------
// Sampler Desc
//-----------------------------------------------------------------------------------------------
// Shader/Common/StaticSampler.hlsli
const CD3DX12_STATIC_SAMPLER_DESC g_staticSamplers[] =
{
	// s0: pointWrap
	CD3DX12_STATIC_SAMPLER_DESC(
		0,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP),	// addressW

	// s1: pointClamp
	CD3DX12_STATIC_SAMPLER_DESC(
		1,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP),	// addressW

	// s2: linearWrap
	CD3DX12_STATIC_SAMPLER_DESC(
		2,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP),	// addressW

	// s3: linearClamp
	CD3DX12_STATIC_SAMPLER_DESC(
		3,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP),	// addressW

	// s4: anisotropicWrap
	CD3DX12_STATIC_SAMPLER_DESC(
		4,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,								// mipLODBias
		8),									// maxAnisotropy

	// s5: anisotropicClamp
	CD3DX12_STATIC_SAMPLER_DESC(
		5,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8),
};

const size_t g_numStaticSamplers = sizeof(g_staticSamplers) / sizeof(CD3DX12_STATIC_SAMPLER_DESC);

uint32_t DXGI_FORMAT_GetStride(DXGI_FORMAT format)
{
	switch (format)
	{
	// 8-bit formats
	case DXGI_FORMAT_R8_UNORM:            return 1;
	case DXGI_FORMAT_R8_SNORM:            return 1;
	case DXGI_FORMAT_R8_UINT:             return 1;
	case DXGI_FORMAT_R8_SINT:             return 1;
	case DXGI_FORMAT_A8_UNORM:            return 1;

	// 16-bit formats
	case DXGI_FORMAT_R16_FLOAT:           return 2;
	case DXGI_FORMAT_R16_UNORM:           return 2;
	case DXGI_FORMAT_R16_SNORM:           return 2;
	case DXGI_FORMAT_R16_UINT:            return 2;
	case DXGI_FORMAT_R16_SINT:            return 2;
	case DXGI_FORMAT_R8G8_UNORM:          return 2;
	case DXGI_FORMAT_R8G8_SNORM:          return 2;
	case DXGI_FORMAT_R8G8_UINT:           return 2;
	case DXGI_FORMAT_R8G8_SINT:           return 2;

	// 24-bit formats
	case DXGI_FORMAT_R8G8B8A8_UNORM:      return 4;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return 4;
	case DXGI_FORMAT_R8G8B8A8_SNORM:      return 4;
	case DXGI_FORMAT_R8G8B8A8_UINT:       return 4;
	case DXGI_FORMAT_R8G8B8A8_SINT:       return 4;
	case DXGI_FORMAT_B8G8R8A8_UNORM:      return 4;
	case DXGI_FORMAT_B8G8R8X8_UNORM:      return 4;

	// 16-bit/channel (32-bit, 48-bit, 64-bit)
	case DXGI_FORMAT_R16G16_FLOAT:        return 4;
	case DXGI_FORMAT_R16G16_UNORM:        return 4;
	case DXGI_FORMAT_R16G16_SNORM:        return 4;
	case DXGI_FORMAT_R16G16_UINT:         return 4;
	case DXGI_FORMAT_R16G16_SINT:         return 4;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:  return 8;
	case DXGI_FORMAT_R16G16B16A16_UNORM:  return 8;
	case DXGI_FORMAT_R16G16B16A16_SNORM:  return 8;
	case DXGI_FORMAT_R16G16B16A16_UINT:   return 8;
	case DXGI_FORMAT_R16G16B16A16_SINT:   return 8;

	// 32-bit/channel
	case DXGI_FORMAT_R32_FLOAT:           return 4;
	case DXGI_FORMAT_R32_UINT:            return 4;
	case DXGI_FORMAT_R32_SINT:            return 4;
	case DXGI_FORMAT_R32G32_FLOAT:        return 8;
	case DXGI_FORMAT_R32G32_UINT:         return 8;
	case DXGI_FORMAT_R32G32_SINT:         return 8;
	case DXGI_FORMAT_R32G32B32_FLOAT:     return 12;
	case DXGI_FORMAT_R32G32B32_UINT:      return 12;
	case DXGI_FORMAT_R32G32B32_SINT:      return 12;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:  return 16;
	case DXGI_FORMAT_R32G32B32A32_UINT:   return 16;
	case DXGI_FORMAT_R32G32B32A32_SINT:   return 16;

	// Packed formats
	case DXGI_FORMAT_R10G10B10A2_UNORM:   return 4;
	case DXGI_FORMAT_R10G10B10A2_UINT:    return 4;
	case DXGI_FORMAT_R11G11B10_FLOAT:     return 4;

	// Depth-stencil formats
	case DXGI_FORMAT_D16_UNORM:           return 2;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:   return 4;
	case DXGI_FORMAT_D32_FLOAT:           return 4;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:return 8;
	default: return 0;
	}
}

D3D12_RESOURCE_DIMENSION ToDX12Dimension(DXResourceDimension dimension)
{
	switch (dimension)
	{
	case DXResourceDimension::BUFFER:		return D3D12_RESOURCE_DIMENSION_BUFFER;
	case DXResourceDimension::TEXTURE1D:	return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case DXResourceDimension::TEXTURE2D:	return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case DXResourceDimension::TEXTURE3D:	return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	default:								return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}
}

//D3D12_RESOURCE_FLAGS ToDX12Flags(DXResourceFlags flags)
//{
//	D3D12_RESOURCE_FLAGS dxFlags = D3D12_RESOURCE_FLAG_NONE;
//	if (flags & DXRF_RENDER_TARGET)
//		dxFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
//	if (flags & DXRF_DEPTH_STENCIL)
//		dxFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//	if (flags & DXRF_UNORDERED_ACCESS)
//		dxFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
//	if (flags & DXRF_DENY_SHADER_RESOURCE)
//		dxFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
//	return dxFlags;
//}

namespace DX12Graphics
{
	//-----------------------------------------------------------------------------------------------

	//-----------------------------------------------------------------------------------------------
	DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	D3D12_RASTERIZER_DESC RasterizerSolidCullNone;
	D3D12_RASTERIZER_DESC RasterizerSolidCullBack;
	D3D12_RASTERIZER_DESC RasterizerWireframeCullNone;
	D3D12_RASTERIZER_DESC RasterizerWireframeCullBack;

	D3D12_BLEND_DESC BlendOpaque;
	D3D12_BLEND_DESC BlendAlpha;
	D3D12_BLEND_DESC BlendAdditive;

	D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
	D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyAlways;
	D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyLessEqual;
	D3D12_DEPTH_STENCIL_DESC DepthStateReadWriteLessEqual;
}

void DX12Graphics::InitializeCommonState()
{
	//-----------------------------------------------------------------------------------------------
	D3D12_RASTERIZER_DESC defaultRasterizerDesc = {};

	defaultRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	defaultRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	defaultRasterizerDesc.FrontCounterClockwise = TRUE;
	defaultRasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	defaultRasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	defaultRasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	defaultRasterizerDesc.DepthClipEnable = TRUE;
	defaultRasterizerDesc.MultisampleEnable = FALSE;
	defaultRasterizerDesc.AntialiasedLineEnable = FALSE;
	defaultRasterizerDesc.ForcedSampleCount = 0;
	defaultRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	RasterizerSolidCullNone = defaultRasterizerDesc;
	RasterizerSolidCullNone.CullMode = D3D12_CULL_MODE_NONE;

	RasterizerSolidCullBack = defaultRasterizerDesc;

	RasterizerWireframeCullNone = defaultRasterizerDesc;
	RasterizerWireframeCullNone.FillMode = D3D12_FILL_MODE_WIREFRAME;
	RasterizerWireframeCullNone.CullMode = D3D12_CULL_MODE_NONE;

	RasterizerWireframeCullBack = defaultRasterizerDesc;
	RasterizerWireframeCullBack.FillMode = D3D12_FILL_MODE_WIREFRAME;

	//-----------------------------------------------------------------------------------------------
	D3D12_BLEND_DESC defaultBlendDesc = {};
	defaultBlendDesc.AlphaToCoverageEnable = FALSE;
	defaultBlendDesc.IndependentBlendEnable = FALSE;
	defaultBlendDesc.RenderTarget[0].BlendEnable = FALSE;
	defaultBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	defaultBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	defaultBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	defaultBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	defaultBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	defaultBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	defaultBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	defaultBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	defaultBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	BlendOpaque = defaultBlendDesc;
	BlendOpaque.RenderTarget[0].BlendEnable = TRUE;


	BlendAlpha = defaultBlendDesc;
	BlendAlpha.RenderTarget[0].BlendEnable = TRUE;
	BlendAlpha.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	BlendAlpha.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	BlendAlpha.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	BlendAlpha.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	BlendAlpha.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	BlendAlpha.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	BlendAdditive = defaultBlendDesc;
	BlendAdditive.RenderTarget[0].BlendEnable = TRUE;
	BlendAdditive.RenderTarget[0].BlendEnable = TRUE;
	BlendAdditive.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	BlendAdditive.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	BlendAdditive.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	BlendAdditive.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	BlendAdditive.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	BlendAdditive.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	//-----------------------------------------------------------------------------------------------
	D3D12_DEPTH_STENCIL_DESC defaultDepthState = {};

	// Use Reverse Z will increase the precision: Greater Equal
	defaultDepthState.DepthEnable = FALSE;
	defaultDepthState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	defaultDepthState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	defaultDepthState.StencilEnable = FALSE;
	defaultDepthState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	defaultDepthState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	defaultDepthState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	defaultDepthState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	defaultDepthState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	defaultDepthState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	defaultDepthState.BackFace = defaultDepthState.FrontFace;

	DepthStateDisabled = defaultDepthState;

	DepthStateReadOnlyAlways = defaultDepthState;
	DepthStateReadOnlyAlways.DepthEnable = TRUE;
	DepthStateReadOnlyAlways.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthStateReadOnlyAlways.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	DepthStateReadOnlyLessEqual = defaultDepthState;
	DepthStateReadOnlyLessEqual.DepthEnable = TRUE;
	DepthStateReadOnlyLessEqual.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthStateReadOnlyLessEqual.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	DepthStateReadWriteLessEqual = defaultDepthState;
	DepthStateReadWriteLessEqual.DepthEnable = TRUE;
	DepthStateReadWriteLessEqual.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStateReadWriteLessEqual.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

}

void DX12Graphics::DestroyCommonState()
{

}
