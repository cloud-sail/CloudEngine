#include "Engine/Renderer/DX12PipelineState.hpp"


#ifdef ENGINE_RENDER_D3D12

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/HashCombine.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/DX12GraphicsCommon.hpp"
#include "ThirdParty/directx/d3dx12.h"

#include <map>

static std::map<size_t, ID3D12PipelineState*> s_graphicPSOHashMap;
static std::map<size_t, ID3D12PipelineState*> s_computePSOHashMap;


STATIC void PSO::DestroyAll()
{
	for (auto& it : s_graphicPSOHashMap)
	{
		DX_SAFE_RELEASE(it.second);
	}
	s_graphicPSOHashMap.clear();

	for (auto& it : s_computePSOHashMap)
	{
		DX_SAFE_RELEASE(it.second);
	}
	s_computePSOHashMap.clear();
}


//-----------------------------------------------------------------------------------------------
// Input Layout Desc
//-----------------------------------------------------------------------------------------------
static const D3D12_INPUT_ELEMENT_DESC g_InputLayout_PCU[] =
{
	{"POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"COLOR"	, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

static const D3D12_INPUT_ELEMENT_DESC g_InputLayout_PCUTBN[] =
{
	{"POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"COLOR"	, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TANGENT"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

};

// Instance Rendering sample, NOT USED
static const D3D12_INPUT_ELEMENT_DESC g_InputLayout_Instancing[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
	{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
};

GraphicsPSO::GraphicsPSO(const wchar_t* name /*= L"Unnamed Graphics PSO"*/)
	: PSO(name)
{
	m_PSODesc = {};
	m_PSODesc.NodeMask = 1;
	m_PSODesc.SampleMask = 0xFFFFFFFFu;
	m_PSODesc.SampleDesc = { 1, 0 };
	m_PSODesc.InputLayout.NumElements = 0;
	m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_PSODesc.NumRenderTargets = 1;
	m_PSODesc.RTVFormats[0] = DX12Graphics::BackBufferFormat;
	m_PSODesc.DSVFormat = DX12Graphics::DepthStencilFormat;
}

void GraphicsPSO::SetBlendMode(BlendMode blendMode)
{
	m_blendMode = blendMode;

	switch (m_blendMode)
	{
	case BlendMode::OPAQUE:
		m_PSODesc.BlendState = DX12Graphics::BlendOpaque;
		break;
	case BlendMode::ALPHA:
		m_PSODesc.BlendState = DX12Graphics::BlendAlpha;
		break;
	case BlendMode::ADDITIVE:
		m_PSODesc.BlendState = DX12Graphics::BlendAdditive;
		break;
	default:
		break;
	}
}

void GraphicsPSO::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	m_rasterizerMode = rasterizerMode;

	switch (m_rasterizerMode)
	{
	case RasterizerMode::SOLID_CULL_NONE:
		m_PSODesc.RasterizerState = DX12Graphics::RasterizerSolidCullNone;
		break;
	case RasterizerMode::SOLID_CULL_BACK:
		m_PSODesc.RasterizerState = DX12Graphics::RasterizerSolidCullBack;
		break;
	case RasterizerMode::WIREFRAME_CULL_NONE:
		m_PSODesc.RasterizerState = DX12Graphics::RasterizerWireframeCullNone;
		break;
	case RasterizerMode::WIREFRAME_CULL_BACK:
		m_PSODesc.RasterizerState = DX12Graphics::RasterizerWireframeCullBack;
		break;
	default:
		ERROR_AND_DIE("Unknown rasterizer mode!");
		break;
	}
}

void GraphicsPSO::SetDepthMode(DepthMode depthMode)
{
	m_depthMode = depthMode;

	switch (m_depthMode)
	{
	case DepthMode::DISABLED:
		m_PSODesc.DepthStencilState = DX12Graphics::DepthStateDisabled;
		break;
	case DepthMode::READ_ONLY_ALWAYS:
		m_PSODesc.DepthStencilState = DX12Graphics::DepthStateReadOnlyAlways;
		break;
	case DepthMode::READ_ONLY_LESS_EQUAL:
		m_PSODesc.DepthStencilState = DX12Graphics::DepthStateReadOnlyLessEqual;
		break;
	case DepthMode::READ_WRITE_LESS_EQUAL:
		m_PSODesc.DepthStencilState = DX12Graphics::DepthStateReadWriteLessEqual;
		break;
	default:
		ERROR_AND_DIE("Unknown depth mode!");
		break;
	}
}

//void GraphicsPSO::SetInputLayout(VertexType inputLayoutMode)
//{
//	m_inputLayoutMode = inputLayoutMode;
//
//	switch (m_inputLayoutMode)
//	{
//	case VertexType::VERTEX_PCU:
//		m_PSODesc.InputLayout = { g_InputLayout_PCU, _countof(g_InputLayout_PCU) };
//		break;
//	case VertexType::VERTEX_PCUTBN:
//		m_PSODesc.InputLayout = { g_InputLayout_PCUTBN, _countof(g_InputLayout_PCUTBN) };
//		break;
//	default:
//		ERROR_AND_DIE("Unknown Input Layout!");
//		break;
//	}
//}

void GraphicsPSO::SetShader(Shader* shader)
{
	m_shaderName = shader->GetName();

	m_PSODesc.VS.pShaderBytecode = (shader->m_vertexShaderByteCode.size() > 0)? shader->m_vertexShaderByteCode.data(): nullptr;
	m_PSODesc.VS.BytecodeLength = shader->m_vertexShaderByteCode.size();

	m_PSODesc.PS.pShaderBytecode = (shader->m_pixelShaderByteCode.size() > 0) ? shader->m_pixelShaderByteCode.data() : nullptr;
	m_PSODesc.PS.BytecodeLength = shader->m_pixelShaderByteCode.size();
	
	m_PSODesc.GS.pShaderBytecode = (shader->m_geometryShaderByteCode.size() > 0) ? shader->m_geometryShaderByteCode.data() : nullptr;
	m_PSODesc.GS.BytecodeLength = shader->m_geometryShaderByteCode.size();


	m_inputLayoutMode = shader->m_inputLayoutMode;

	switch (m_inputLayoutMode)
	{
	case VertexType::VERTEX_NONE:
		m_PSODesc.InputLayout = { nullptr, 0 };
		break;
	case VertexType::VERTEX_PCU:
		m_PSODesc.InputLayout = { g_InputLayout_PCU, _countof(g_InputLayout_PCU) };
		break;
	case VertexType::VERTEX_PCUTBN:
		m_PSODesc.InputLayout = { g_InputLayout_PCUTBN, _countof(g_InputLayout_PCUTBN) };
		break;
	default:
		ERROR_AND_DIE("Unknown Input Layout!");
		break;
	}
}

void GraphicsPSO::SetRenderTargetFormats(UINT numRTVs, DXGI_FORMAT const* rtvFormats, DXGI_FORMAT dsvFormat, UINT msaaCount /*= 1*/, UINT msaaQuality /*= 0*/)
{
	for (UINT i = 0; i < numRTVs; ++i)
	{
		m_PSODesc.RTVFormats[i] = rtvFormats[i];
	}
	for (UINT i = numRTVs; i < m_PSODesc.NumRenderTargets; ++i)
		m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	m_PSODesc.NumRenderTargets = numRTVs;
	m_PSODesc.DSVFormat = dsvFormat;

	// if dsvFormat is UNKNOWN, it means no dsv is used, and DepthStencil Should be set as Disabled
	if (dsvFormat == DXGI_FORMAT_UNKNOWN)
	{
		GUARANTEE_OR_DIE(m_PSODesc.DepthStencilState.DepthEnable == FALSE && m_PSODesc.DepthStencilState.StencilEnable == FALSE, "Please set Depth Mode as Disabled first");
	}

	m_PSODesc.SampleDesc.Count = msaaCount;
	m_PSODesc.SampleDesc.Quality = msaaQuality;
}

void GraphicsPSO::Finalize()
{
	GUARANTEE_OR_DIE(m_rootSignature != nullptr, "Graphics PSO does not have a rootsignature.");
	m_PSODesc.pRootSignature = m_rootSignature; // may need to be hashed

	size_t hashKey = GetHashKey();

	auto found = s_graphicPSOHashMap.find(hashKey);
	if (found != s_graphicPSOHashMap.end())
	{
		m_PSO = found->second;
		return;
	}

	HRESULT hr = DX12Renderer::s_device->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create Graphics PSO!");
	}

	s_graphicPSOHashMap[hashKey] = m_PSO;

	static int i = 0;
	++i;
	std::wstring newName = WStringf(L"%d", i);
	m_PSO->SetName(newName.c_str());
}


size_t GraphicsPSO::GetHashKey() const
{
	size_t seed = 0;
	hash_combine(seed, m_shaderName);
	hash_combine(seed, m_inputLayoutMode);
	hash_combine(seed, m_blendMode);
	hash_combine(seed, m_rasterizerMode);
	hash_combine(seed, m_depthMode);


	// Not important to hash now
	hash_combine(seed, m_PSODesc.NumRenderTargets);
	for (UINT i = 0; i < 8; ++i) 
	{
		if (i < m_PSODesc.NumRenderTargets) 
		{
			hash_combine(seed, m_PSODesc.RTVFormats[i]);
		}
		else 
		{
			hash_combine(seed, DXGI_FORMAT_UNKNOWN);
		}
	}

	hash_combine(seed, m_PSODesc.DSVFormat);
	hash_combine(seed, m_PSODesc.SampleDesc.Count);
	hash_combine(seed, m_PSODesc.SampleDesc.Quality);
	hash_combine(seed, m_PSODesc.PrimitiveTopologyType);

	return seed;
}

ComputePSO::ComputePSO(const wchar_t* name /*= L"Unnamed Compute PSO"*/)
	: PSO(name)
{
	m_PSODesc = {};
	m_PSODesc.NodeMask = 1;
}

void ComputePSO::SetShader(Shader* shader)
{
	m_shaderName = shader->GetName();


	m_PSODesc.CS.pShaderBytecode = (shader->m_computeShaderByteCode.size() > 0) ? shader->m_computeShaderByteCode.data() : nullptr;
	m_PSODesc.CS.BytecodeLength = shader->m_computeShaderByteCode.size();
}

void ComputePSO::Finalize()
{
	GUARANTEE_OR_DIE(m_rootSignature != nullptr, "Compute PSO does not have a rootsignature.");
	m_PSODesc.pRootSignature = m_rootSignature; // may need to be hashed

	size_t hashKey = GetHashKey();

	auto found = s_computePSOHashMap.find(hashKey);
	if (found != s_computePSOHashMap.end())
	{
		m_PSO = found->second;
		return;
	}

	HRESULT hr = DX12Renderer::s_device->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create Compute PSO!");
	}

	s_computePSOHashMap[hashKey] = m_PSO;

	static int i = 0;
	++i;
	std::wstring newName = WStringf(L"%d", i);
	m_PSO->SetName(newName.c_str());
}

size_t ComputePSO::GetHashKey() const
{
	size_t seed = 0;
	hash_combine(seed, m_shaderName);

	return seed;
}

#endif // ENGINE_RENDER_D3D12

