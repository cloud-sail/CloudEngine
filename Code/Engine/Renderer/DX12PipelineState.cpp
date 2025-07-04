#include "Engine/Renderer/DX12PipelineState.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "ThirdParty/directx/d3dx12.h"
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

GraphicsPSO::GraphicsPSO(const wchar_t* name /*= L"Unnamed Graphics PSO"*/)
	: PSO(name)
{
	m_PSODesc = {};
	m_PSODesc.NodeMask = 1;
	m_PSODesc.SampleMask = 0xFFFFFFFFu;
	m_PSODesc.SampleDesc = {1, 0};
	m_PSODesc.InputLayout.NumElements = 0;
}

void GraphicsPSO::SetBlendMode(BlendMode blendMode)
{
	UNUSED(blendMode);
}

void GraphicsPSO::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	if (rasterizerMode == RasterizerMode::SOLID_CULL_NONE)
	{

	}
	else if (rasterizerMode == RasterizerMode::SOLID_CULL_NONE)
	{

	}
	else if (rasterizerMode == RasterizerMode::SOLID_CULL_NONE)
	{

	}
	else if (rasterizerMode == RasterizerMode::SOLID_CULL_NONE)
	{

	}
	else
	{

	}
}

void GraphicsPSO::SetDepthMode(DepthMode depthMode)
{



	if (depthMode == DepthMode::DISABLED)
	{

	}
	else if (depthMode == DepthMode::READ_ONLY_ALWAYS)
	{

	}
	else if (depthMode == DepthMode::READ_ONLY_LESS_EQUAL)
	{

	}
	else if (depthMode == DepthMode::READ_WRITE_LESS_EQUAL)
	{

	}
}

void GraphicsPSO::SetInputLayout(InputLayoutMode inputLayoutMode)
{
	if (inputLayoutMode == InputLayoutMode::VERTEX_PCU)
	{
		m_PSODesc.InputLayout = { g_InputLayout_PCU, _countof(g_InputLayout_PCU) };
	}
	else if (inputLayoutMode == InputLayoutMode::VERTEX_PCUTBN)
	{
		m_PSODesc.InputLayout = { g_InputLayout_PCUTBN, _countof(g_InputLayout_PCUTBN) };
	}
}

void GraphicsPSO::SetShader(Shader* shader)
{
	m_shaderName = shader->GetName();
	m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(shader->m_VS);
	m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(shader->m_PS);
}
