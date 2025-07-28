#pragma once
#include "Engine/Renderer/RendererCommon.hpp"


#ifdef ENGINE_RENDER_D3D12

#include <string>
#include <d3d12.h>

struct ID3D12PipelineState;
class Shader;

class PSO
{
public:
	PSO(const wchar_t* name) : m_name(name) {}

	static void DestroyAll(); // call it when shutdown

	void SetRootSignature(ID3D12RootSignature* rootSig)
	{
		m_rootSignature = rootSig;
	}

	ID3D12RootSignature* GetRootSignature() const
	{
		return m_rootSignature; // may be null if you never set
	}

	ID3D12PipelineState* GetPipelineStateObject() const { return m_PSO; }

protected:
	const wchar_t* m_name; // debug
	ID3D12PipelineState* m_PSO = nullptr;

	ID3D12RootSignature* m_rootSignature = nullptr;
};

// We do not hash the root signature for now, different root sigs will use different shaders.
// Fixed SampleMask, StreamOutput(not set), TopologyType(triangle), NumRenderTarget(1), RTVFormat[8], DSVFormat, SampleDesc(1,0)
// TopologyType is triangle, but Topology can be triangle list triangle
class GraphicsPSO : public PSO
{
public:
	GraphicsPSO(const wchar_t* name = L"Unnamed Graphics PSO");


	void SetBlendMode(BlendMode blendMode);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);
	//void SetInputLayout(VertexType inputLayoutMode);
	void SetShader(Shader* shader);
	// Not hashed Not used // for deferred rendering render 3 G-Buffer
	void SetRenderTargetFormats(UINT numRTVs, DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat, UINT msaaCount = 1, UINT msaaQuality = 0);

	void Finalize();

private:
	size_t GetHashKey() const;

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc = {};

	// Hash
	std::string		m_shaderName;
	VertexType		m_inputLayoutMode = VertexType::VERTEX_PCU;
	BlendMode       m_blendMode = BlendMode::ALPHA;
	RasterizerMode  m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	DepthMode       m_depthMode = DepthMode::READ_WRITE_LESS_EQUAL;
};

#endif // ENGINE_RENDER_D3D12
