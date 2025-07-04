#pragma once


#include "Engine/Renderer/Renderer.hpp"
//#include <d3d12.h>
//#include <dxgi1_6.h>
#include <array>

struct IDXGISwapChain4;
struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12RootSignature;
struct ID3D12DescriptorHeap;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList6;
struct IDXGIFactory4;
struct ID3D12Fence;
struct CD3DX12_STATIC_SAMPLER_DESC;

typedef void* HANDLE;













//-----------------------------------------------------------------------------------------------
class DX12Renderer 
{
public:
	DX12Renderer(RendererConfig const& config);
	~DX12Renderer() = default;
	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	void ClearScreen(Rgba8 const& clearColor);
	void BeginCamera(Camera const& camera);
	void EndCamera(Camera const& camera);

public:


protected:
	void CreateDevice();
	void CreateFenceAndHandleAndDescriptorSizes();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void CreateRenderTargetView();
	void CreateDepthStencilBufferAndView();
	void SetDebugNameForBasicDXObjects();

	void FlushCommandQueue();






	void SampleSetViewPortAndScissorRect();

protected:
	RendererConfig m_config; // #ToDo inherit from Renderer Class

protected:
	IDXGIFactory4*		m_dxgiFactory = nullptr;
	IDXGISwapChain4*	m_swapChain	= nullptr;
	ID3D12Device*		m_device = nullptr;

	ID3D12CommandQueue*				m_commandQueue = nullptr;
	ID3D12CommandAllocator*			m_commandAllocator = nullptr; // have 3 allocator for frame buffering
	ID3D12GraphicsCommandList6*		m_commandList = nullptr;


	static constexpr unsigned int SWAP_CHAIN_BUFFER_COUNT = 2; // FRAME_COUNT
	ID3D12Resource* m_swapChainBuffer[SWAP_CHAIN_BUFFER_COUNT] = {}; // m_renderTargets
	ID3D12Resource* m_depthStencilBuffer = nullptr; // Texture in DX11

	ID3D12DescriptorHeap* m_rtvHeap = nullptr;
	ID3D12DescriptorHeap* m_dsvHeap = nullptr;
	
	ID3D12RootSignature* m_rootSignature = nullptr;

	ID3D12PipelineState* m_pipelineState = nullptr;

	unsigned int m_rtvDescriptorSize = 0;
	unsigned int m_dsvDescriptorSize = 0;
	unsigned int m_cbvSrvUavDescriptorSize = 0;

	// Synchronization objects.

	unsigned int m_frameIndex = 0; // currBackBuffer
	HANDLE m_fenceEvent;
	ID3D12Fence* m_fence = nullptr;
	uint64_t m_currentFenceValue = 0; 


protected:

	/*
	inline void CommandContext::SetPipelineState( const PSO& PSO )
{
	ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
	if (PipelineState == m_CurPipelineState)
		return;

	m_CommandList->SetPipelineState(PipelineState);
	m_CurPipelineState = PipelineState;
	// not switch pso
	// find pso // or the engine holds it own pso??
}*/

	// m_CurPipelineState
};

