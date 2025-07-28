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
