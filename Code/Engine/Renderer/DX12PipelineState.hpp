#pragma once
#include "Engine/Renderer/RendererCommon.hpp"
#include <string>
#include <d3d12.h>

struct ID3D12PipelineState;
class Shader;

class PSO
{
public:
	PSO(const wchar_t* name) : m_name(name) {}

	static void DestroyAll(void);

	void SetRootSignature(ID3D12RootSignature* rootSig)
	{
		m_rootSignature = rootSig;
	}

	ID3D12PipelineState* GetPipelineStateObject(void) const { return m_PSO; }

protected:
	const wchar_t* m_name; // debug
	ID3D12PipelineState* m_PSO = nullptr;

	ID3D12RootSignature* m_rootSignature = nullptr;
};


class GraphicsPSO : public PSO
{
public:
	GraphicsPSO(const wchar_t* name = L"Unnamed Graphics PSO");

	void SetBlendMode(BlendMode blendMode);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);
	void SetInputLayout(InputLayoutMode inputLayoutMode);
	void SetShader(Shader* shader);
	// #ToDO SetRootSignature

	void Finalize();
	//void SetRTVFormats // for deferred rendering render 3 G-Buffer

private:
	void UpdateBlendDesc();
	void UpdateRasterizerDesc();
	void UpdateDepthStencilDesc();


private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc = {};




	// Hash
	std::string		m_shaderName;
	InputLayoutMode m_inputLayoutMode = InputLayoutMode::VERTEX_PCU;
	BlendMode       m_blendMode = BlendMode::ALPHA;
	RasterizerMode  m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	DepthMode       m_depthMode = DepthMode::READ_WRITE_LESS_EQUAL;

};
// #ToDo many states are using default setting, if we use these, change it later


// do not expose the pso to the game code, only expose state, bind state
// bind Shader
// Render->BindState {change currstate}
// when draw,first check desire == curr, then check pso changed? rootsig change?..
// when reset command list  reset to default pso default state
// Render create pso when

// #ToDo ComputePSO
//-----------------------------------------------------------------------------------------------
//	inline void CommandContext::SetPipelineState( const PSO& PSO )
//{
//	ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
//	if (PipelineState == m_CurPipelineState)
//		return;
//
//	m_CommandList->SetPipelineState(PipelineState);
//	m_CurPipelineState = PipelineState;
//	// not switch pso
//	// find pso // or the engine holds it own pso??
//}


// m_CurPipelineState

// If you do not set